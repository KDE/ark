/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal
 * <haraldhv atatatat stud.ntnu.no>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "cliinterface.h"

#include <QFile>
#include <QDir>
#include <QDateTime>
#include <KProcess>
#include <KStandardDirs>
#include <KDebug>
#include <KLocale>
#include <QEventLoop>
#include <QThread>
#include <KProcess>
#include <kptyprocess.h>
#include <kptydevice.h>

namespace Kerfuffle
{

	CliInterface::CliInterface( const QString& filename, QObject *parent)
		: ReadWriteArchiveInterface(filename, parent),
		m_process(NULL),
		m_loop(NULL)
	{

	}

	void CliInterface::cacheParameterList()
	{
		m_param = parameterList();
		Q_ASSERT(m_param.contains(ExtractProgram));
		Q_ASSERT(m_param.contains(ListProgram));
		Q_ASSERT(m_param.contains(PreservePathSwitch));
		Q_ASSERT(m_param.contains(RootNodeSwitch));
	}

	CliInterface::~CliInterface()
	{

	}

	bool CliInterface::list()
	{
		cacheParameterList();
		m_mode = List;

		bool ret = findProgramInPath(m_param.value(ListProgram).toString());
		if (!ret) {
			error("TODO could not find program");
			return false;
		}

		ret = createProcess();
		if (!ret) {
			error("TODO could not find program");
			return false;
		}

		QStringList args = m_param.value(ListArgs).toStringList();
		substituteListVariables(args);

		executeProcess(m_program, args);

		return true;
	}

	bool CliInterface::copyFiles( const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options )
	{
		kDebug( 1601) ;
		cacheParameterList();

		m_mode = Copy;


		bool ret = findProgramInPath(m_param.value(ExtractProgram).toString());
		if (!ret) {
			error("TODO could not find program");
			return false;
		}

		ret = createProcess();
		if (!ret) {
			error("TODO could not find program");
			return false;
		}

		//start preparing the argument list
		QStringList args = m_param.value(ExtractArgs).toStringList();

		//now replace the various elements in the list
		for (int i = 0; i < args.size(); ++i) {
			QString argument = args.at(i);
			kDebug(1601) << "Processing argument " << argument;

			if (argument == "$Archive") {
				args[i] = filename();
			}

			if (argument == "$PreservePathSwitch") {

				QStringList replacementFlags = m_param.value(PreservePathSwitch).toStringList();
				Q_ASSERT(replacementFlags.size() == 2);

				bool preservePaths = options.value("PreservePaths").toBool();
				QString theReplacement;
				if (preservePaths)
					theReplacement = replacementFlags.at(0);
				else
					theReplacement = replacementFlags.at(1);

				if (theReplacement.isEmpty()) {
					args.removeAt(i);
					--i; //decrement to compensate for the variable we removed
				}
				else
					//but in this case we don't have to decrement, we just
					//replace it
					args[i] = theReplacement;
			}

			if (argument == "$RootNodeSwitch") {

				//if the RootNodeSwitch argument has been added, we at least
				//assume that the format of the switch has been added as well
				Q_ASSERT(m_param.contains(RootNodeSwitch));

				//we will decrement i afterwards
				args.removeAt(i);

				QString rootNode;
				if (options.contains("RootNode"))
				{
					rootNode = options.value("RootNode").toString();
					kDebug(1601) << "Set root node " << rootNode;
				}

				if (!rootNode.isEmpty()) {
					QStringList theSwitch = m_param.value(RootNodeSwitch).toStringList();
					for (int j = 0; j < theSwitch.size(); ++j) {
						//get the argument part
						QString newArg = theSwitch.at(j);

						//substitute the $Path
						newArg.replace("$Path", rootNode);

						//put it in the arg list
						args.insert(i+j, newArg);
						++i;

					}
				}
				--i; //decrement to compensate for the variable we replaced

			}


			if (argument == "$Files") {
				args.removeAt(i);
				for (int j = 0; j < files.count(); ++j) {
					args.insert(i + j, files.at(j).toString());
					++i;
				}
				--i;
			}
		}

		QDir::setCurrent(destinationDirectory);

		executeProcess(m_program, args);

		return true;
	}


	bool CliInterface::addFiles( const QStringList & files, const CompressionOptions& options )
	{
		cacheParameterList();

		m_mode = Add;

		return false;
	}

	bool CliInterface::deleteFiles( const QList<QVariant> & files )
	{
		cacheParameterList();
		m_mode = Delete;

		return false;
	}

	bool CliInterface::createProcess()
	{
		if (m_process)
			return false;

		m_process = new KPtyProcess;
		m_process->setOutputChannelMode( KProcess::SeparateChannels );

		connect( m_process, SIGNAL( started() ), SLOT( started() ) );
		connect( m_process, SIGNAL( readyReadStandardOutput() ), SLOT( readStdout() ) );
		connect( m_process, SIGNAL( readyReadStandardError() ), SLOT( readFromStderr() ) );
		connect( m_process, SIGNAL( finished( int, QProcess::ExitStatus ) ), SLOT( finished( int, QProcess::ExitStatus ) ) );

		if (QMetaType::type("QProcess::ExitStatus") == 0)
			qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
		return true;
	}

	bool CliInterface::executeProcess(const QString& path, const QStringList & args)
	{
		kDebug( 1601 ) << "Executing " << path << args;
		Q_ASSERT(!path.isEmpty());

		m_process->setProgram( path, args );
		m_process->setNextOpenMode( QIODevice::ReadWrite | QIODevice::Unbuffered );
		m_process->start();
		QEventLoop loop;
		m_loop = &loop;
		bool ret = loop.exec( QEventLoop::WaitForMoreEvents );
		m_loop = 0;

		delete m_process;
		m_process = NULL;
		/*
		   if (!m_errorMessages.isEmpty())
		   {
		   error(m_errorMessages.join("\n"));
		   return false;
		   }
		   else if (ret && !m_userCancelled) {
		   error(i18n("Unknown error when extracting files"));
		   return false;
		   }
		   else
		   {
		   return true;
		   }
		   */

		return true;
	}


	void CliInterface::started()
	{
		//m_state = 0;
		m_errorMessages.clear();
		m_userCancelled = false;
	}

	void CliInterface::finished( int exitCode, QProcess::ExitStatus exitStatus)
	{
		if ( !m_process )
			return;

		progress(1.0);

		if ( m_loop )
		{
			m_loop->exit( exitStatus == QProcess::CrashExit ? 1 : 0 );
		}
	}

	void CliInterface::readFromStderr()
	{
		if ( !m_process )
			return;

		QByteArray stdErrData = m_process->readAllStandardError();

		kDebug( 1601 ) << "ERROR" << stdErrData.size() << stdErrData;

		if ( !stdErrData.isEmpty() )
		{
			//if (handlePasswordPrompt(stdErrData))
				//return;
			//else if (handleOverwritePrompt(stdErrData))
			//	return;
			//else
			{
				m_errorMessages << QString::fromLocal8Bit(stdErrData);
			}
		}
	}

	void CliInterface::readStdout()
	{
		if ( !m_process )
			return;

		m_stdOutData += m_process->readAllStandardOutput();

		// process all lines until the last '\n' or backspace
		int indx = m_stdOutData.lastIndexOf('\010');
		if (indx == -1) indx = m_stdOutData.lastIndexOf('\n');
		if (indx == -1) return;

		QString leftString = QString::fromLocal8Bit(m_stdOutData.left(indx + 1));
		const QStringList lines = leftString.split( QRegExp("[\\n\\010]"), QString::SkipEmptyParts );
		foreach(const QString &line, lines) {

			if ((m_mode == Copy || m_mode == Add) && m_param.contains(CaptureProgress) && m_param.value(CaptureProgress).toBool())
			{
				//read the percentage
				int pos = line.indexOf('%');
				if (pos != -1 && pos > 1) {
					int percentage = line.mid(pos - 2, 2).toInt();
					progress(float(percentage) / 100);
					continue;
				}
			}

			readListLine(line);

		}

		m_stdOutData.remove(0, indx + 1);
	}


	bool CliInterface::findProgramInPath(const QString& program)
	{
		m_program = KStandardDirs::findExe( program );
		return !m_program.isEmpty();
	}


	void CliInterface::substituteListVariables(QStringList& params)
	{
		for (int i = 0; i < params.size(); ++i) {
			QString parameter = params.at(i);

			if (parameter == "$Archive") {
				params[i] = filename();
			}

		}
	}

}

#include "cliinterface.moc"
