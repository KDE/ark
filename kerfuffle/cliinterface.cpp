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
#include <QTimer>

#include <QApplication>

namespace Kerfuffle
{

	CliInterface::CliInterface( const QString& filename, QObject *parent)
		: ReadWriteArchiveInterface(filename, parent),
		m_process(NULL),
		m_loop(NULL),
		m_isPasswordProtected(false)
	{
		//because this interface uses the event loop
		setWaitForFinishedSignal(true);
	}

	void CliInterface::cacheParameterList()
	{
		m_param = parameterList();
		Q_ASSERT(m_param.contains(ExtractProgram));
		Q_ASSERT(m_param.contains(ListProgram));
		Q_ASSERT(m_param.contains(PreservePathSwitch));
		Q_ASSERT(m_param.contains(RootNodeSwitch));
		Q_ASSERT(m_param.contains(FileExistsExpression));
		Q_ASSERT(m_param.contains(FileExistsInput));
	}

	CliInterface::~CliInterface()
	{
	}

	bool CliInterface::list()
	{
		cacheParameterList();
		m_mode = List;

		bool ret = findProgramAndCreateProcess(m_param.value(ListProgram).toString());
		if (!ret) {
			failOperation();
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


		bool ret = findProgramAndCreateProcess(m_param.value(ExtractProgram).toString());
		if (!ret) {
			failOperation();
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

			if (argument == "$PasswordSwitch") {

				//if the PasswordSwitch argument has been added, we at least
				//assume that the format of the switch has been added as well
				Q_ASSERT(m_param.contains(PasswordSwitch));

				//we will decrement i afterwards
				args.removeAt(i);

				//if we get a hint about this being a password protected archive, ask about
				//the password in advance.
				if (options.value("PasswordProtectedHint").toBool() && password().isEmpty()) {
					kDebug( 1601 ) << "Password hint enabled, querying user";

					Kerfuffle::PasswordNeededQuery query(filename());
					userQuery(&query);
					query.waitForResponse();

					if (query.responseCancelled()) {
						failOperation();
						return false;
					}
					setPassword(query.password());
				}

				QString pass = password();

				if (!pass.isEmpty()) {
					QStringList theSwitch = m_param.value(PasswordSwitch).toStringList();
					for (int j = 0; j < theSwitch.size(); ++j) {
						//get the argument part
						QString newArg = theSwitch.at(j);

						//substitute the $Path
						newArg.replace("$Password", pass);

						//put it in the arg list
						args.insert(i+j, newArg);
						++i;

					}
				}
				--i; //decrement to compensate for the variable we replaced

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

		bool ret = findProgramAndCreateProcess(m_param.value(ListProgram).toString());
		if (!ret) {
			failOperation();
			return false;
		}

		QString globalWorkdir = options.value("GlobalWorkDir").toString();
		if (!globalWorkdir.isEmpty()) {
			kDebug( 1601 ) << "GlobalWorkDir is set, changing dir to " << globalWorkdir;
			QDir::setCurrent(globalWorkdir);
		}

		//start preparing the argument list
		QStringList args = m_param.value(AddArgs).toStringList();

		//now replace the various elements in the list
		for (int i = 0; i < args.size(); ++i) {
			QString argument = args.at(i);
			kDebug(1601) << "Processing argument " << argument;

			if (argument == "$Archive") {
				args[i] = filename();
			}

			if (argument == "$Files") {
				args.removeAt(i);
				for (int j = 0; j < files.count(); ++j) {

					QString relativeName = QDir::current().relativeFilePath(files.at(j));

					args.insert(i + j, relativeName);
					++i;
				}
				--i;
			}

		}

		executeProcess(m_program, args);

		return true;
	}

	bool CliInterface::deleteFiles( const QList<QVariant> & files )
	{
		cacheParameterList();
		m_mode = Delete;

		bool ret = findProgramAndCreateProcess(m_param.value(DeleteProgram).toString());
		if (!ret) {
			failOperation();
			return false;
		}

		//start preparing the argument list
		QStringList args = m_param.value(DeleteArgs).toStringList();

		//now replace the various elements in the list
		for (int i = 0; i < args.size(); ++i) {
			QString argument = args.at(i);
			kDebug(1601) << "Processing argument " << argument;

			if (argument == "$Archive") {
				args[i] = filename();
			}

			if (argument == "$Files") {
				args.removeAt(i);
				for (int j = 0; j < files.count(); ++j) {

					//QString relativeName = QDir::current().relativeFilePath(files.at(j));

					args.insert(i + j, files.at(j).toString());
					++i;
				}
				--i;
			}

		}

		m_removedFiles = files;

		executeProcess(m_program, args);

		return true;
	}

	bool CliInterface::isPasswordProtected()
	{
		return m_isPasswordProtected;
	}

	void CliInterface::setPasswordProtected(bool isPasswordProtected)
	{
		m_isPasswordProtected = isPasswordProtected;
	}

	bool CliInterface::createProcess()
	{
		kDebug(1601);

		m_process = new KProcess();
		m_process->setOutputChannelMode( KProcess::MergedChannels );

		connect( m_process, SIGNAL( started() ), SLOT( started() ), Qt::DirectConnection  );
		connect( m_process, SIGNAL( readyReadStandardOutput() ), SLOT( readStdout() ), Qt::DirectConnection );
		connect( m_process, SIGNAL( finished( int, QProcess::ExitStatus ) ), SLOT( processFinished( int, QProcess::ExitStatus ) ), Qt::DirectConnection );

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
		m_userCancelled = false;
	}

	void CliInterface::processFinished( int exitCode, QProcess::ExitStatus exitStatus )
	{
		Q_UNUSED( exitCode );
		Q_UNUSED( exitStatus );

		kDebug(1601);

		//if the m_process pointer is gone, then there is nothing to worry
		//about here
		if (!m_process)
			return;

		if (m_mode == Delete)
		{
			foreach(const QVariant& v, m_removedFiles) {
				entryRemoved(v.toString());
			}

		}

		delete m_process;
		m_process = NULL;
		progress(1.0);
		finished(true);
	}

	void CliInterface::failOperation()
	{
		kDebug(1601);
		KProcess *p = m_process;
		m_process = NULL;
		p->terminate();
		finished(false);
	}

	void CliInterface::readStdout()
	{
		//a quick note for any new hackers: Yes, this function is not
		//very pretty, but it has become like this for reasons. You are
		//very welcome to find a better implementation for this, but
		//please consider the following points:
		//
		//- standard output comes in unpredictable chunks, this is why
		//you can never know if the last part of the output is a complete line or not
		//- because of eventloop magic and the process running in its
		//own thread, the readStdOut might be called at any time, even
		//while it's busy processing the last call (this last case
		//happens when another eventloop is created somewhere (eg
		//Query::execute) and the readyReadStandardOutput signal is
		//picked up there)
		//- console applications are not really consistent about what
		//characters they send out (newline, backspace, carriage return,
		//etc), so keep in mind that this function is supposed to handle
		//all those special cases and be the lowest common denominator

		if ( !m_process ) {
			return;
		}

		//if the process is still not finished (m_process is appearantly not
		//set to NULL if here), then the operation should definitely not be in
		//the main thread as this would freeze everything. assert this.
		Q_ASSERT(QThread::currentThread() != QApplication::instance()->thread());

		//if another function is already accessing this function, then we
		//assume that it will finish processing also the lines we just added.
		static QMutex thisFuncMutex;
		bool tryLock = thisFuncMutex.tryLock();

		if (!tryLock)  {
			return;
		}

		QByteArray dd = m_process->readAllStandardOutput();

		//for simplicity, we replace all carriage return characters to newlines
		dd.replace('\015', '\n');

		//same thing with backspaces. 
		//TODO: whether this is a safe assumption or not needs to be
		//determined
		dd.replace('\010', '\n');

		m_stdOutData += dd;

		//if there is no newline, we leave the data like this for now.
		if (!m_stdOutData.contains('\n')) {
			//kDebug(1601) << "No new line, we leave it like this for now";
			thisFuncMutex.unlock();
			return;
		}

		QList<QByteArray> list = m_stdOutData.split('\n');

		//because the last line might be incomplete we leave it for now
		QByteArray lastLine = list.takeLast();
		m_stdOutData = lastLine;

		foreach( const QByteArray& line, list) {

			if (line.isEmpty()) continue;

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

			if (m_mode == Copy) {

				
				if (checkForErrorMessage(line, WrongPasswordPatterns)) {
					kDebug(1601) << "Wrong password!";
					error(i18n("Incorrect password."));
					failOperation();
					return;
				}

				if (checkForErrorMessage(line, ExtractionFailedPatterns)) {
					kDebug(1601) << "Error in extraction!!";
					error(i18n("Extraction failed because of an unexpected error."));
					failOperation();
					return;
				}

				if (checkForFileExistsMessage(line))
					continue;

			}

			if (m_mode == List) {
				readListLine(line);
				continue;
			}

		}

		thisFuncMutex.unlock();
		readStdout();
	}


	bool CliInterface::findProgramAndCreateProcess(const QString& program)
	{
		m_program = KStandardDirs::findExe( program );
		bool ret = !m_program.isEmpty();
		if (!ret) {
			error(i18n("Failed to locate program '%1' in PATH.", program));
			return false;
		}

		ret = createProcess();
		if (!ret) {
			error(i18n("Found program '%1', but failed to initalise the process.", program));
			return false;
		}

		return true;

	}

	bool CliInterface::checkForFileExistsMessage(const QString& line)
	{
		if (m_existsPattern.isEmpty()) {
			m_existsPattern.setPattern(m_param.value(FileExistsExpression).toString());
		}
		if (m_existsPattern.indexIn(line) != -1) {
			kDebug(1601) << "Detected file existing!! Filename " << m_existsPattern.cap(1);

			Kerfuffle::OverwriteQuery query(QDir::current().path() + '/' + m_existsPattern.cap(1));
			query.setNoRenameMode(true);
			userQuery(&query);
			kDebug(1601) << "Waiting response";
			query.waitForResponse();

			kDebug(1601) << "Finished response";

			QString responseToProcess;
			QStringList choices = m_param.value(FileExistsInput).toStringList();

			if (query.responseOverwrite())
				responseToProcess = choices.at(0);
			else if (query.responseSkip())
				responseToProcess = choices.at(1);
			else if (query.responseOverwriteAll())
				responseToProcess = choices.at(2);
			else if (query.responseCancelled())
				responseToProcess = choices.at(3);
			else if (query.responseAutoSkip())
				responseToProcess = choices.at(4);

			Q_ASSERT(!responseToProcess.isEmpty());

			responseToProcess += '\n';

			kDebug(1601) << "Writing " << responseToProcess;

			m_process->write(responseToProcess.toLocal8Bit());

			return true;
		}

		return false;
	}

	bool CliInterface::checkForErrorMessage(const QString& line, int parameterIndex)
	{
		static QHash<int, QList<QRegExp> > patternCache;
		QList<QRegExp> patterns;

		kDebug() << line;

		if (patternCache.contains(parameterIndex)) {
			patterns = patternCache.value(parameterIndex);
		}
		else {
			if (!m_param.contains(parameterIndex))
				return false;

			foreach(const QString& rawPattern, m_param.value(parameterIndex).toStringList()) {
				patterns << QRegExp(rawPattern);
			}
			patternCache[parameterIndex] = patterns;
		}

		foreach(const QRegExp& pattern, patterns) {
			if (pattern.indexIn(line) != -1) {
				return true;
			}
		}
		return false;
	}

	bool CliInterface::doKill()
	{
		if (m_process) {
			m_process->terminate();
			if (!m_process->waitForFinished())
				m_process->kill();
			m_process->waitForFinished();
			return true;
		}
		return false;
	}

	bool CliInterface::doSuspend()
	{
		return false;
	}

	bool CliInterface::doResume()
	{
		return false;
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
