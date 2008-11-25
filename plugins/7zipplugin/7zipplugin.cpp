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

p7zipInterface::p7zipInterface( const QString & filename, QObject *parent )
	: ReadWriteArchiveInterface( filename, parent ),
	m_filename(filename)
{
	kDebug( 1601 ) << "7zipplugin opening " << filename ;
	
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
	if (m_exepath.isNull())
		return false;
		
	#if defined(Q_OS_WIN)
	KProcess kp(QThread::currentThread());
	kp.setOutputChannelMode( KProcess::MergedChannels );
	#else
	KPtyProcess kp(QThread::currentThread());
	kp.setOutputChannelMode( KProcess::SeparateChannels );
	#endif

	m_process = &kp;

	connect( m_process, SIGNAL( started() ), SLOT( started() ) );
	connect( m_process, SIGNAL( readyReadStandardOutput() ), SLOT( listReadStdout() ) );
	connect( m_process, SIGNAL( readyReadStandardError() ), SLOT( readFromStderr() ) );

	qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
	connect( m_process, SIGNAL( finished( int, QProcess::ExitStatus ) ), SLOT( finished( int, QProcess::ExitStatus ) ) );

	QStringList args;
	args << "l" << "-slt" << m_filename;

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
	m_process = 0;
	#endif

	if (ret) {
		kDebug( 1601 ) << m_exepath << "failed to finish";
		return false;
	}

	kDebug( 1601 ) << m_exepath << "finished";

	return true;
}

void p7zipInterface::started()
{
	m_state = 0;
}

void p7zipInterface::listReadStdout()
{
	if ( !m_process )
		return;
		
	m_stdOutData += m_process->readAllStandardOutput();

	// process all lines until the last '\n'
	int indx = m_stdOutData.lastIndexOf('\n');
	QString leftString = QString::fromLocal8Bit(m_stdOutData.left(indx + 1));
	QStringList lines = leftString.split( '\n' );
	foreach(const QString &line, lines)
	{
		listProcessLine(m_state, line);
	}
	m_stdOutData.remove(0, indx + 1);

	//kDebug( 1601 ) << "leftOver" << m_stdOutData;

	if (m_stdOutData.startsWith("Enter password (will not be echoed) :"))
	{
		Kerfuffle::PasswordNeededQuery query(filename());
		emit userQuery(&query);
		query.waitForResponse();

		if (query.responseCancelled()) {
			error(i18n("Password input cancelled by user."));
			m_process->kill();
			return;
		}
		setPassword(query.password());
		writeToProcess(password().toLocal8Bit().append('\r'));
	}
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
			}
			else if (line.contains("Error:"))
			{
				error(line);
			}
			break;
		case 1: // beginning of a file detail
			if (line.startsWith("Path ="))
			{
				QString entryFilename = QDir::fromNativeSeparators(line.mid( 6).trimmed());
				m_currentArchiveEntry[FileName] = entryFilename;
				m_currentArchiveEntry[InternalID] = entryFilename;
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
			else if (line.startsWith("Encrypted = "))
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
	
	if (m_exepath.isNull())
	{
		return false;
	}
	
	QList<QString> overwriteList;
	bool overwriteAll = false;
	if (files.count() == 0)
	{
		overwriteAll = true;
	}
	else
	{
		for (int i = 0; i < files.count(); i++)
		{
			if (overwriteAll)
			{
				overwriteList << files[i].toString();
			}
			else
			{
				QString filepath(destinationDirectory + '/' + files[i].toString());
				kDebug( 1601 ) << "checking" << filepath;
				if (QFile::exists(filepath))
				{
					Kerfuffle::OverwriteQuery query(filepath);
					query.setNoRenameMode(true);
					emit userQuery(&query);
					query.waitForResponse();
					if (query.responseOverwrite())
					{
						overwriteList << files[i].toString();
					}
					else if (query.responseSkip())
					{
						// do not add to overwriteList
					}
					else if (query.responseOverwriteAll())
					{
						overwriteAll = true;
						overwriteList << files[i].toString();
					}
					else if (query.responseCancelled())
					{
						return false;
					}
				}
				else
				{
					overwriteList << files[i].toString();
				}
			}
		}
	}
	
	#if defined(Q_OS_WIN)
	KProcess kp(QThread::currentThread());
	kp.setOutputChannelMode( KProcess::MergedChannels );
	#else
	KPtyProcess kp(QThread::currentThread());
	kp.setOutputChannelMode( KProcess::SeparateChannels );
	#endif
	
	m_process = &kp;

	connect( m_process, SIGNAL( started() ), SLOT( started() ) );
	connect( m_process, SIGNAL( readyReadStandardOutput() ), SLOT( copyReadStdout() ) );
	connect( m_process, SIGNAL( readyReadStandardError() ), SLOT( readFromStderr() ) );

	qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
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
	
	if (overwriteAll || overwriteList.count() > 0)
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
	args << m_filename;

	foreach( const QString& file, overwriteList )
	{
		args << file.trimmed();
	}

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
	m_process = 0;
	#endif

	return !ret;
}

void p7zipInterface::copyReadStdout()
{
	if ( !m_process )
		return;
		
	m_stdOutData += m_process->readAllStandardOutput();

	// process all lines until the last '\n'
	int indx = m_stdOutData.lastIndexOf('\n');
	QString leftString = QString::fromLocal8Bit(m_stdOutData.left(indx + 1));
	QStringList lines = leftString.split( '\n', QString::SkipEmptyParts );
	
	//kDebug( 1601 ) << "lines:" << lines;
	
	for (int i = 0; i < lines.size(); i++)
	{
		const QString &line = lines[i];
		if (line.startsWith("Extracting"))
		{
			// TODO: update progress
		}
	}
	m_stdOutData.remove(0, indx + 1);

	if (m_stdOutData.startsWith("Enter password (will not be echoed) :"))
	{
		Kerfuffle::PasswordNeededQuery query(filename());
		emit userQuery(&query);
		query.waitForResponse();

		if (query.responseCancelled()) {
			error(i18n("Password input cancelled by user."));
			m_process->kill();
			return;
		}
		setPassword(query.password());
		writeToProcess(password().toLocal8Bit().append('\r'));
	}
}

bool p7zipInterface::addFiles( const QStringList & files, const CompressionOptions& options )
{
	kDebug( 1601 ) << "Will try to add files " << files << " to " << m_filename << " using " << m_exepath;

	if (m_exepath.isNull())
	{
		return false;
	}

	KProcess kp;
	kp << m_exepath << "a";
	kp << "-bd"; // suppress percentage indicator
	kp << m_filename;


	foreach( const QString& file, files )
	{
		kDebug( 1601 ) << file;
		kp << file;
	}

	kp.setOutputChannelMode(KProcess::MergedChannels);
	kp.start();

	if (!kp.waitForStarted())
	{
		kDebug( 1601 ) << m_exepath << "did not start";
		return false;
	}

	float fileCount = 0;
	bool hasWarning = false;
	QString warningMessages;
	while (kp.waitForReadyRead()) {
		QStringList lines = QString(kp.readAll()).split('\n');
		foreach(const QString &line, lines) {
			if (line.startsWith("Compressing"))
			{
				fileCount += 1;
				progress(fileCount / files.size());
			}
			else if (line.contains("WARNINGS for files:"))
			{
				hasWarning = true;
			}
			else if (hasWarning)
			{
				warningMessages += line + '\n';
			}
		}
	}

	list();

	if (hasWarning)
		error(warningMessages);

	kDebug( 1601 ) << "Finished adding files";

	return true;
}

bool p7zipInterface::deleteFiles( const QList<QVariant> & files )
{
	kDebug( 1601 ) << "Will try to delete " << files << " from " << m_filename;

	if (m_exepath.isNull())
	{
		return false;
	}

	KProcess kp;
	kp << m_exepath << "d";
	kp << m_filename;


	foreach( const QVariant& file, files )
	{
		kDebug( 1601 ) << file;
		kp << file.toString();
	}

	kp.setOutputChannelMode(KProcess::MergedChannels);
	kp.start();
	if (!kp.waitForStarted()){
		return false;
	}

	if (!kp.waitForFinished()){
		return false;
	}

	// TODO: ensure file is deleted
	if (kp.exitStatus() == QProcess::NormalExit)
	{
		foreach( const QVariant& file, files )
		{
			kDebug( 1601 ) << file;
			entryRemoved(file.toString());
		}
	}

	return true;
}

KERFUFFLE_PLUGIN_FACTORY( p7zipInterface )

