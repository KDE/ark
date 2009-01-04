/*
* 7zipplugin -- plugin for KDE ark
*
* Copyright (C) 2008 Chen Yew Ming <fusion82@gmail.com>
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


#include "7zipplugin.h"
#include "kerfuffle/archivefactory.h"

#include <QFile>
#include <QDir>
#include <QDateTime>
#include <KProcess>
#include <KStandardDirs>
#include <KDebug>
#include <KLocale>
#include <QEventLoop>
#include <QThread>

#if !defined(Q_OS_WIN)
#include <kptyprocess.h>
#include <kptydevice.h>
#endif

const QByteArray P7ZIP_PASSWORD_PROMPT_STR("Enter password (will not be echoed) :");
const QString NO_7ZIPPLUGIN_NO_ERROR("7ZIPPLUGIN_NO_ERROR");

p7zipInterface::p7zipInterface( const QString & filename, QObject *parent ) :
	ReadWriteArchiveInterface( filename, parent ),
	m_process(NULL)
{
	kDebug( 1601 ) << "7zipplugin opening " << filename;

	m_exepath = KStandardDirs::findExe( "7z" );
	if (m_exepath.isNull())
	{
		m_exepath = KStandardDirs::findExe( "7za" );
	}
	if (m_exepath.isNull())
	{
		m_exepath = KStandardDirs::findExe( "7zr" );
	}

	if (m_exepath.isNull()) {
		error(i18n( "Unable to find 7zr, 7za or 7z" ));
		return;
	}
}

p7zipInterface::~p7zipInterface()
{
}

bool p7zipInterface::list()
{
	kDebug( 1601 );

	if (!QFile::exists(filename()))
		return true;

	if (!create7zipProcess())
	{
		return false;
	}

	connect( m_process, SIGNAL( started() ), SLOT( started() ) );
	connect( m_process, SIGNAL( readyReadStandardOutput() ), SLOT( listReadStdout() ) );
	connect( m_process, SIGNAL( readyReadStandardError() ), SLOT( readFromStderr() ) );
	connect( m_process, SIGNAL( finished( int, QProcess::ExitStatus ) ), SLOT( finished( int, QProcess::ExitStatus ) ) );

	QStringList args;
	args << "l" << "-slt" << filename();

	return execute7zipProcess(args);
}

void p7zipInterface::started()
{
	m_state = 0;
	m_errorMessages.clear();
}

void p7zipInterface::listReadStdout()
{
	if ( !m_process )
		return;

	m_stdOutData += m_process->readAllStandardOutput();

	// process all lines until the last '\n'
	int indx = m_stdOutData.lastIndexOf('\n');
	QString leftString = QString::fromLocal8Bit(m_stdOutData.left(indx + 1));
	const QStringList lines = leftString.split( '\n' );
	foreach(const QString &line, lines)
	{
		listProcessLine(m_state, line);
	}
	m_stdOutData.remove(0, indx + 1);

	//kDebug( 1601 ) << "leftOver" << m_stdOutData;

	handlePasswordPrompt(m_stdOutData);
}

void p7zipInterface::readFromStderr()
{
	if ( !m_process )
		return;

	m_stdErrData += m_process->readAllStandardError();

	kDebug( 1601 ) << "ERROR:" << m_stdErrData;

	if ( !m_stdErrData.isEmpty() )
	{
		m_process->kill();
		return;
	}
}

void p7zipInterface::finished( int exitCode, QProcess::ExitStatus exitStatus)
{
	if ( !m_process )
		return;

	progress(1.0);

	if ( m_loop )
	{
		m_loop->exit( exitStatus == QProcess::CrashExit ? 1 : 0 );
	}
}

void p7zipInterface::writeToProcess( const QByteArray &data )
{
	if ( !m_process || data.isNull() )
		return;

#if defined(Q_OS_WIN)
	m_process->write( data );
#else
	m_process->pty()->write( data );
#endif
}

void p7zipInterface::listProcessLine(int& state, const QString& line)
{
	switch (state)
	{
		case 0: // header
			if (line.startsWith("Listing archive:"))
			{
				kDebug( 1601 ) << "Archive name: " << line.right(line.size() - 16).trimmed() ;
			}
			else if (line.startsWith("----------"))
			{
				state = 1;
				m_archiveContents.clear();
			}
			else if (line.contains("Error:"))
			{
				m_errorMessages << line.mid(6);
			}
			break;
		case 1: // beginning of a file detail
			if (line.startsWith("Path ="))
			{
				m_currentArchiveEntry.clear();
				QString entryFilename = QDir::fromNativeSeparators(line.mid( 6).trimmed());
				m_currentArchiveEntry[FileName] = entryFilename;
				m_currentArchiveEntry[InternalID] = entryFilename;
				m_archiveContents << entryFilename;
				state = 2;
			}
			break;

		case 2: // file details
			if (line.startsWith("Size = "))
			{
				m_currentArchiveEntry[ Size ] = line.mid( 7).trimmed();
			}
			else if (line.startsWith("Packed Size = "))
			{
				m_currentArchiveEntry[ CompressedSize ] = line.mid( 14).trimmed();
			}
			else if (line.startsWith("Modified = "))
			{
				QDateTime ts = QDateTime::fromString(line.mid(11).trimmed(), "yyyy-MM-dd hh:mm:ss");
				m_currentArchiveEntry[ Timestamp ] = ts;
			}
			else if (line.startsWith("Attributes = "))
			{
				QString attributes = line.mid(13).trimmed();

				bool isDirectory = attributes.startsWith('D');
				m_currentArchiveEntry[ IsDirectory ] = isDirectory;
				if (isDirectory)
				{
					QString directoryName = m_currentArchiveEntry[FileName].toString();
					if (!directoryName.endsWith('/'))
					{
						m_currentArchiveEntry[FileName] = m_currentArchiveEntry[InternalID] = directoryName + '/';
					}
				}

				m_currentArchiveEntry[ Permissions ] = attributes.mid(1);
			}
			else if (line.startsWith("CRC = "))
			{
				m_currentArchiveEntry[ CRC ] = line.mid(6).trimmed();
			}
			else if (line.startsWith("Method = "))
			{
				QString method = line.mid(9).trimmed();
				m_currentArchiveEntry[ Method ] = method;
			}
			else if (line.startsWith("Encrypted = ") && line.size() >= 13)
			{
				bool isPasswordProtected = (line.at(12) == '+');
				m_currentArchiveEntry[ IsPasswordProtected ] = isPasswordProtected;
			}
			else if (line.startsWith("Block = "))
			{
				// do nothing
			}
			else if (line.isEmpty()) // assume end of file details
			{
				if (m_currentArchiveEntry.contains(FileName))
				{
					entry(m_currentArchiveEntry);
				}

				state = 1;
			}
			break;

		default:
			break;
	}
}

bool p7zipInterface::copyFiles( const QList<QVariant> & files, const QString & destinationDirectory, Archive::CopyFlags flags )
{
	const bool preservePaths = flags & Archive::PreservePaths;

	kDebug( 1601 ) << "extract" << files  << "to" << destinationDirectory << (preservePaths? " with paths":"");

	QStringList overwriteList;
	const QList<QVariant>* filesList = (files.count() == 0)? &m_archiveContents : &files;
	bool overwriteAll = false;

	for (int i = 0; i < filesList->count(); i++)
	{
		if (overwriteAll)
		{
			overwriteList << filesList->at(i).toString();
		}
		else
		{
			QString filepath(destinationDirectory + '/' + filesList->at(i).toString());
			if (QFile::exists(filepath))
			{
				Kerfuffle::OverwriteQuery query(filepath);
				query.setNoRenameMode(true);
				emit userQuery(&query);
				query.waitForResponse();
				if (query.responseOverwrite())
				{
					overwriteList << filesList->at(i).toString();
				}
				else if (query.responseSkip())
				{
					// do not add to overwriteList
				}
				else if (query.responseOverwriteAll())
				{
					overwriteAll = true;
					overwriteList << filesList->at(i).toString();
				}
				else if (query.responseCancelled())
				{
					return false;
				}
			}
			else
			{
				overwriteList << filesList->at(i).toString();
			}
		}
	}

	if (!create7zipProcess())
	{
		return false;
	}

	connect( m_process, SIGNAL( started() ), SLOT( started() ) );
	connect( m_process, SIGNAL( readyReadStandardOutput() ), SLOT( copyReadStdout() ) );
	connect( m_process, SIGNAL( readyReadStandardError() ), SLOT( readFromStderr() ) );
	connect( m_process, SIGNAL( finished( int, QProcess::ExitStatus ) ), SLOT( finished( int, QProcess::ExitStatus ) ) );

	QStringList args;
	if (preservePaths)
	{
		args << "x";
	}
	else
	{
		args << "e";
	}

	if (overwriteList.count() > 0)
	{
		args << "-y";
	}
	else
	{
		// nothing to extract, just return
		return true;
	}

	args << "-bd";

	if ( !password().isEmpty() ) args << "-p" + password();

	args << "-o" + destinationDirectory;
	args << filename();

	foreach( const QString& file, overwriteList )
	{
		args << file.trimmed();
	}

	m_progressFilesCount = 0;
	m_totalFilesCount = overwriteList.count();

	return execute7zipProcess(args);
}

void p7zipInterface::copyReadStdout()
{
	if ( !m_process )
		return;

	m_stdOutData += m_process->readAllStandardOutput();

	// process all lines until the last '\n'
	int indx = m_stdOutData.lastIndexOf('\n');
	QString leftString = QString::fromLocal8Bit(m_stdOutData.left(indx + 1));
	const QStringList lines = leftString.split( '\n', QString::SkipEmptyParts );

	//kDebug( 1601 ) << "lines:" << lines;

	for (int i = 0; i < lines.size(); i++)
	{
		const QString &line = lines[i];
		if (line.startsWith("Extracting"))
		{
			m_progressFilesCount++;
			if (m_progressFilesCount > m_totalFilesCount)
				progress(0.9);
			else
				progress((double)m_progressFilesCount / m_totalFilesCount);

			// check for error
			int errorIndex = line.lastIndexOf("     ");
			if (errorIndex > 0)
			{
				QString errorMessage = line.mid(errorIndex + 5);
				if (!errorMessage.isEmpty())
				{
					m_errorMessages << line.mid(11);
				}
			}
		}
	}
	m_stdOutData.remove(0, indx + 1);

	handlePasswordPrompt(m_stdOutData);
}

bool p7zipInterface::addFiles( const QStringList & files, const CompressionOptions& options )
{
	kDebug( 1601 ) << files << "options:" << options;

	if (!create7zipProcess())
	{
		return false;
	}

	connect( m_process, SIGNAL( started() ), SLOT( started() ) );
	connect( m_process, SIGNAL( readyReadStandardOutput() ), SLOT( addReadStdout() ) );
	connect( m_process, SIGNAL( readyReadStandardError() ), SLOT( readFromStderr() ) );
	connect( m_process, SIGNAL( finished( int, QProcess::ExitStatus ) ), SLOT( finished( int, QProcess::ExitStatus ) ) );

	QStringList args;
	args << "a" << "-bd" << filename();
	foreach( const QString& file, files )
	{
		args << file;
	}
	m_progressFilesCount = 0;
	m_totalFilesCount = files.count();

	bool returnSuccess = execute7zipProcess(args);

	list();
	return returnSuccess;
}

void p7zipInterface::addReadStdout()
{
	if ( !m_process )
		return;

	m_stdOutData += m_process->readAllStandardOutput();

	// process all lines until the last '\n'
	int indx = m_stdOutData.lastIndexOf('\n');
	QString leftString = QString::fromLocal8Bit(m_stdOutData.left(indx + 1));
	const QStringList lines = leftString.split( '\n', QString::SkipEmptyParts );

	foreach (const QString &line, lines)
	{
		if (line.startsWith("Compressing"))
		{
			m_progressFilesCount++;
			progress((double)m_progressFilesCount / m_totalFilesCount);
		}
		else if (line.contains("WARNINGS for files:"))
		{
			m_state = 1;
		}
		else if (m_state == 1)
		{
			m_errorMessages << line;
		}
	}
	m_stdOutData.remove(0, indx + 1);

	handlePasswordPrompt(m_stdOutData);
}


bool p7zipInterface::deleteFiles( const QList<QVariant> & files )
{
	kDebug( 1601 ) << files;

	if (!create7zipProcess())
	{
		return false;
	}

	connect( m_process, SIGNAL( started() ), SLOT( started() ) );
	connect( m_process, SIGNAL( readyReadStandardOutput() ), SLOT( deleteReadStdout() ) );
	connect( m_process, SIGNAL( readyReadStandardError() ), SLOT( readFromStderr() ) );
	connect( m_process, SIGNAL( finished( int, QProcess::ExitStatus ) ), SLOT( finished( int, QProcess::ExitStatus ) ) );

	QStringList args;
	args << "d" << "-bd" << filename();
	foreach( const QVariant& file, files )
	{
		args << file.toString();
	}
	m_progressFilesCount = 0;
	m_totalFilesCount = files.count();

	bool returnSuccess = execute7zipProcess(args);

	// TODO: ensure file is deleted, perhaps list again?
	if (returnSuccess)
	{
		foreach( const QVariant& file, files )
		{
			entryRemoved(file.toString());
		}
	}

	kDebug( 1601 ) << m_errorMessages;
	return returnSuccess;
}

void p7zipInterface::deleteReadStdout()
{
	if ( !m_process )
		return;

	m_stdOutData += m_process->readAllStandardOutput();

	// process all lines until the last '\n'
	int indx = m_stdOutData.lastIndexOf('\n');
	QString leftString = QString::fromLocal8Bit(m_stdOutData.left(indx + 1));
	const QStringList lines = leftString.split( '\n', QString::SkipEmptyParts );

	foreach (const QString& line, lines)
	{
		kDebug( 1601 ) << line;
		if (line.contains("error", Qt::CaseInsensitive))
		{
			m_errorMessages << line;
			m_state = 1;
		}
		else if (m_state == 1)
		{
			m_errorMessages << line;
		}
	}
	m_stdOutData.remove(0, indx + 1);

	handlePasswordPrompt(m_stdOutData);
}

bool p7zipInterface::create7zipProcess()
{
	if (m_exepath.isEmpty())
		return false;
	if (m_process)
		return false;

	#if defined(Q_OS_WIN)
	m_process = new KProcess; //QThread::currentThread());
	m_process->setOutputChannelMode( KProcess::MergedChannels );
	#else
	m_process = new KPtyProcess; //QThread::currentThread());
	m_process->setOutputChannelMode( KProcess::SeparateChannels );
	#endif

	if (QMetaType::type("QProcess::ExitStatus") == 0)
		qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
	return true;
}

bool p7zipInterface::execute7zipProcess(const QStringList & args)
{
	#if defined(Q_OS_WIN)
	m_process->start( m_exepath, args, QIODevice::ReadWrite | QIODevice::Unbuffered );
	bool ret = m_process->waitForFinished( -1 ) ? 0 : 1;
	#else
	m_process->setProgram( m_exepath, args );
	m_process->setNextOpenMode( QIODevice::ReadWrite | QIODevice::Unbuffered );
	m_process->start();
	QEventLoop loop;
	m_loop = &loop;
	bool ret = loop.exec( QEventLoop::WaitForMoreEvents );
	m_loop = 0;
	#endif

	delete m_process;
	m_process = NULL;

	if (!m_errorMessages.empty() && !m_errorMessages.contains(NO_7ZIPPLUGIN_NO_ERROR))
	{
		error(m_errorMessages.join("\n"));
		return false;
	}
	else if (ret) {
		error(i18n("Unknown error when extracting files"));
		return false;
	}
	else
	{
	  return true;
	}
}

bool p7zipInterface::handlePasswordPrompt(QByteArray &message)
{
	if (message.contains(P7ZIP_PASSWORD_PROMPT_STR))
	{
		// remove the prompt as it has been handled
		message.replace(P7ZIP_PASSWORD_PROMPT_STR, "");

		Kerfuffle::PasswordNeededQuery query(filename());
		emit userQuery(&query);
		query.waitForResponse();

		if (query.responseCancelled()) {
			m_process->kill();
			m_errorMessages << NO_7ZIPPLUGIN_NO_ERROR;
		}
		else
		{
			setPassword(query.password());
			writeToProcess(password().toLocal8Bit().append('\r'));
		}
		return true;
	}
	else
	{
		return false;
	}
}

KERFUFFLE_PLUGIN_FACTORY( p7zipInterface )

