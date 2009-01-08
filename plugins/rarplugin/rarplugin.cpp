/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Claudio Bantaloukas <rockdreamer@gmail.com>
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
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
#include "rarplugin.h"
#include "kerfuffle/archivefactory.h"
#include "kerfuffle/queries.h"

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

const QByteArray RAR_PASSWORD_PROMPT_STR("Enter password (will not be echoed)");
const QByteArray RAR_OVERWRITE_PROMPT_STR("already exists. Overwrite it ?");
const QByteArray RAR_ENTER_NEW_NAME_PROMPT_STR("Enter new name:");

RARInterface::RARInterface( const QString & filename, QObject *parent )
	: ReadWriteArchiveInterface( filename, parent ),
	m_process(NULL)
{
	kDebug( 1601 ) << "Rar plugin opening " << filename ;

	m_unrarpath = KStandardDirs::findExe( "unrar" );
	if (m_unrarpath.isNull())
	{
		m_unrarpath = KStandardDirs::findExe( "unrar-free" );
	}
	bool have_unrar = !m_unrarpath.isNull();

	m_rarpath = KStandardDirs::findExe( "rar" );
	bool have_rar = !m_rarpath.isNull();
	if (!have_rar && !have_unrar) {
		error(i18n( "Neither rar or unrar are available in your PATH." ));
		return;
	}
	if (!have_rar){
		// set read-only mode
	}

	m_headerString = "-----------------------------------------";
	m_isFirstLine = true;
	m_incontent = false;
}

RARInterface::~RARInterface()
{
}

bool RARInterface::list()
{
	kDebug( 1601 );

	if (!QFile::exists(filename()))
		return true;

	if (!createRarProcess())
	{
		return false;
	}

	connect( m_process, SIGNAL( started() ), SLOT( started() ) );
	connect( m_process, SIGNAL( readyReadStandardOutput() ), SLOT( listReadStdout() ) );
	connect( m_process, SIGNAL( readyReadStandardError() ), SLOT( readFromStderr() ) );
	connect( m_process, SIGNAL( finished( int, QProcess::ExitStatus ) ), SLOT( finished( int, QProcess::ExitStatus ) ) );

	QString exePath;
	QStringList args;

	if (!m_rarpath.isNull()) exePath = m_rarpath;
	else if (!m_unrarpath.isNull()) exePath = m_unrarpath;
	else return false;

	args << "v" << "-c-" << filename();

	m_archiveContents.clear();
	return executeRarProcess(exePath, args);
}

void RARInterface::listReadStdout()
{
	if ( !m_process )
		return;

	m_stdOutData += m_process->readAllStandardOutput();

	// process all lines until the last '\n'
	int indx = m_stdOutData.lastIndexOf('\n');
	QString leftString = QString::fromLocal8Bit(m_stdOutData.left(indx + 1));
	const QStringList lines = leftString.split( '\n', QString::SkipEmptyParts );
	foreach(const QString &line, lines)
	{kDebug( 1601 ) << "line" << line;
		processListLine(line);
	}
	m_stdOutData.remove(0, indx + 1);

	if (!m_stdOutData.isEmpty())
		kDebug( 1601 ) << "leftOver" << m_stdOutData;
}

void RARInterface::processListLine(const QString& line)
{
	// skip the heading
	if (!m_incontent){
		if (line.startsWith(m_headerString) )
			m_incontent = true;
		return;
	}
	// catch final line
	if (line.startsWith(m_headerString) ) {
		m_incontent = false;
		return;
	}

	// rar gives one line for the filename and a line after it with some file properties
	if ( m_isFirstLine ) {
		m_internalId = line.trimmed();
		//m_entryFilename.chop(1); // handle newline
		if (!m_internalId.isEmpty() && m_internalId.at(0) == '*')
		{
			m_isPasswordProtected = true;
			m_internalId.remove( 0, 1 ); // and the spaces in front
		}
		else
			m_isPasswordProtected = false;

		m_isFirstLine = false;
		return;
	}

	QStringList fileprops = line.split(' ', QString::SkipEmptyParts);
	m_internalId = QDir::fromNativeSeparators(m_internalId);
	bool isDirectory = (bool)(fileprops[ 5 ].contains('d', Qt::CaseInsensitive));

	m_entryFilename = m_internalId;
	if (isDirectory && !m_internalId.endsWith('/'))
	{
		m_entryFilename += '/';
	}

	kDebug( 1601 ) << m_entryFilename << " : " << fileprops ;
	ArchiveEntry e;
	e[ FileName ] = m_entryFilename;
	e[ InternalID ] = m_internalId;
	e[ Size ] = fileprops[ 0 ];
	e[ CompressedSize] = fileprops[ 1 ];
	e[ Ratio ] = fileprops[ 2 ];
	QDateTime ts (QDate::fromString(fileprops[ 3 ], "dd-mm-yy"),
		QTime::fromString(fileprops[ 4 ], "hh:mm"));
	e[ Timestamp ] = ts;
	e[ IsDirectory ] = isDirectory;
	e[ Permissions ] = fileprops[ 5 ].remove(0,1);
	e[ CRC ] = fileprops[ 6 ];
	e[ Method ] = fileprops[ 7 ];
	e[ Version ] = fileprops[ 8 ];
	e[ IsPasswordProtected] = m_isPasswordProtected;
	kDebug( 1601 ) << "Added entry: " << e ;
	entry(e);
	m_isFirstLine = true;

	m_archiveContents << m_entryFilename;
	return;

}

bool RARInterface::copyFiles( const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options )
{
	const bool preservePaths = options.value("PreservePaths").toBool();

	kDebug( 1601 ) << files  << destinationDirectory << (preservePaths? " with paths":"");

	QDir::setCurrent(destinationDirectory);

	QString rootNode;
	if (options.contains("RootNode"))
	{
		rootNode = options.value("RootNode").toString();
		kDebug(1601) << "Set root node " << rootNode;
	}

	//if we get a hint about this being a password protected archive, ask about
	//the password in advance.
	if (options.value("PasswordProtectedHint").toBool()) {
		kDebug( 1601 ) << "Password hint enabled, querying user";

		Kerfuffle::PasswordNeededQuery query(filename());
		emit userQuery(&query);
		query.waitForResponse();

		if (query.responseCancelled()) {
			return true;
		}
		setPassword(query.password());
	}

	QStringList overwriteList;
	const QList<QVariant>* filesList = (files.count() == 0)? &m_archiveContents : &files;
	QStringList overwriteAllDirectories;
	QStringList autoSkipDirectories;
	QStringList skipList;

	for (int i = 0; i < filesList->count(); i++)
	{
		QString filepath(destinationDirectory + '/' + filesList->at(i).toString());
		QFileInfo currentFileInfo(filepath);

		if ( overwriteAllDirectories.contains(currentFileInfo.canonicalPath()) ||
			overwriteAllDirectories.contains(currentFileInfo.canonicalFilePath()))
		{
			overwriteList << filesList->at(i).toString();
		}
		else if (autoSkipDirectories.contains(currentFileInfo.canonicalPath())
				|| autoSkipDirectories.contains(currentFileInfo.canonicalFilePath()))
		{
			skipList << currentFileInfo.canonicalFilePath();
		}
		else if (currentFileInfo.exists())
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
				skipList << currentFileInfo.canonicalFilePath();
			}
			else if (query.responseAutoSkip())
			{
				if (currentFileInfo.isDir())
				{
					autoSkipDirectories << currentFileInfo.canonicalFilePath();
				}
				else
				{
					autoSkipDirectories << currentFileInfo.canonicalPath();
				}
				kDebug( 1601 ) << "adding auto skip" << autoSkipDirectories.at(autoSkipDirectories.size()-1);
				skipList << currentFileInfo.canonicalFilePath();
			}
			else if (query.responseOverwriteAll())
			{
				kDebug( 1601 ) << "adding overwrite all" << currentFileInfo.canonicalPath();
				overwriteAllDirectories << currentFileInfo.canonicalPath();
				overwriteList << filesList->at(i).toString();
			}
			else if (query.responseCancelled())
			{
				return true;
			}
		}
		else
		{
			overwriteList << filesList->at(i).toString();
		}
	}
	
	if (overwriteList.isEmpty() && !skipList.isEmpty())
	{
		// all files skipped
		return true;
	}

	if (!createRarProcess())
	{
		return false;
	}

	connect( m_process, SIGNAL( started() ), SLOT( started() ) );
	connect( m_process, SIGNAL( readyReadStandardOutput() ), SLOT( copyReadStdout() ) );
	connect( m_process, SIGNAL( readyReadStandardError() ), SLOT( readFromStderr() ) );
	connect( m_process, SIGNAL( finished( int, QProcess::ExitStatus ) ), SLOT( finished( int, QProcess::ExitStatus ) ) );

	QString exePath;
	QStringList args;

	if (!m_rarpath.isNull()) exePath = m_rarpath;
	else if (!m_unrarpath.isNull()) exePath = m_unrarpath;
	else return false;

	if (preservePaths) {
		args << "x";
	} else {
		args << "e";
	}

	args << "-y";   // yes to all overwrite queries

	//args << "-p-"; // do not query for password
	if ( !password().isEmpty() ) args << "-p" + password();

	if (!rootNode.isEmpty())
		args << "-ap" + rootNode;

	args << filename();
	foreach( const QVariant& file, overwriteList )
	{
		QString filename = file.toString();
		if (!filename.endsWith('/'))
		{
			kDebug( 1601 ) << file.toString();
			args << file.toString();
		}
	}
	//args << destinationDirectory;

	return executeRarProcess(exePath, args);
}

void RARInterface::copyReadStdout()
{
	if ( !m_process )
		return;

	m_stdOutData += m_process->readAllStandardOutput();

	// process all lines until the last '\n'
	int indx = m_stdOutData.lastIndexOf('\n');
	QString leftString = QString::fromLocal8Bit(m_stdOutData.left(indx + 1));
	const QStringList lines = leftString.split( '\n', QString::SkipEmptyParts );
	foreach(const QString &line, lines)
	{
		kDebug( 1601 ) << "line" << line;
		//read the percentage
		int pos = line.indexOf('%');
		if (pos != -1 && pos > 1) {
			int percentage = line.mid(pos - 2, 2).toInt();
			progress(float(percentage) / 100);
		}
	}
	m_stdOutData.remove(0, indx + 1);
}

bool RARInterface::addFiles( const QStringList & files, const CompressionOptions& options )
{
	kDebug( 1601 ) << "Will try to add " << files << " to " << filename() << " using " << m_rarpath;

	QString workPath = options.value("GlobalWorkDir").toString();
	if (!workPath.isEmpty()) {
		QDir::setCurrent(workPath);
	}

	if (!createRarProcess())
	{
		return false;
	}

	connect( m_process, SIGNAL( started() ), SLOT( started() ) );
	connect( m_process, SIGNAL( readyReadStandardOutput() ), SLOT( addReadStdout() ) );
	connect( m_process, SIGNAL( readyReadStandardError() ), SLOT( readFromStderr() ) );
	connect( m_process, SIGNAL( finished( int, QProcess::ExitStatus ) ), SLOT( finished( int, QProcess::ExitStatus ) ) );

	QString exePath;
	QStringList args;

	if (!m_rarpath.isNull()) exePath = m_rarpath;
	else return false;

	 args << "a" << "-c-" << filename();
	foreach( const QString& file, files )
	{
		if (!workPath.isEmpty()) {
			args << QDir::current().relativeFilePath(file);
		}
		else
			args << file;
		kDebug( 1601 ) << file;
	}

	bool result = executeRarProcess(exePath, args);

	if (result) list();

	kDebug( 1601 ) << "Finished adding files";

	return result;
}

void RARInterface::addReadStdout()
{
	if ( !m_process )
		return;

	m_stdOutData += m_process->readAllStandardOutput();

	// process all lines until the last '\n'
	int indx = m_stdOutData.lastIndexOf('\n');
	QString leftString = QString::fromLocal8Bit(m_stdOutData.left(indx + 1));
	const QStringList lines = leftString.split( '\n', QString::SkipEmptyParts );
	foreach(const QString &line, lines)
	{
		int pos = line.indexOf('%');
		if (pos < 2 || pos == -1) continue;
		int percentage = line.mid(pos - 2, 2).toInt();
		progress(float(percentage) / 100);
	}
	m_stdOutData.remove(0, indx + 1);
}

bool RARInterface::deleteFiles( const QList<QVariant> & files )
{
	kDebug( 1601 ) << "Will try to delete " << files << " from " << filename();

	if (!QFile::exists(filename()))
		return true;

	if (!createRarProcess())
	{
		return false;
	}

	connect( m_process, SIGNAL( started() ), SLOT( started() ) );
	connect( m_process, SIGNAL( readyReadStandardOutput() ), SLOT( deleteReadStdout() ) );
	connect( m_process, SIGNAL( readyReadStandardError() ), SLOT( readFromStderr() ) );
	connect( m_process, SIGNAL( finished( int, QProcess::ExitStatus ) ), SLOT( finished( int, QProcess::ExitStatus ) ) );

	QString exePath;
	QStringList args;

	if (!m_rarpath.isNull()) exePath = m_rarpath;
	else return false;

	args << "d" << filename();
	foreach( const QVariant& file, files )
	{
		kDebug( 1601 ) << file;
		args << file.toString();
	}

	bool result = executeRarProcess(exePath, args);

	if (result)
	{
		// TODO: ensure files are deleted from archive
		foreach( const QVariant& file, files )
		{
			kDebug( 1601 ) << file;
			entryRemoved(file.toString());
		}
	}

	return result;
}

void RARInterface::deleteReadStdout()
{
	if ( !m_process )
		return;

	m_stdOutData += m_process->readAllStandardOutput();

	// process all lines until the last '\n'
	int indx = m_stdOutData.lastIndexOf('\n');
	QString leftString = QString::fromLocal8Bit(m_stdOutData.left(indx + 1));
	const QStringList lines = leftString.split( '\n', QString::SkipEmptyParts );
	foreach(const QString &line, lines)
	{
		kDebug( 1601 ) << line;
		int pos = line.indexOf('%');
		if (pos < 2 || pos == -1) continue;
		int percentage = line.mid(pos - 2, 2).toInt();
		progress(float(percentage) / 100);
	}
	m_stdOutData.remove(0, indx + 1);
}


bool RARInterface::createRarProcess()
{
	if (m_process)
		return false;

	m_process = new KPtyProcess;
	m_process->setOutputChannelMode( KProcess::SeparateChannels );

	if (QMetaType::type("QProcess::ExitStatus") == 0)
		qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
	return true;
}

bool RARInterface::executeRarProcess(const QString& rarPath, const QStringList & args)
{
	if (rarPath.isEmpty())
	{
		return false;
	}

	kDebug( 1601 ) << rarPath << args;

	m_process->setProgram( rarPath, args );
	m_process->setNextOpenMode( QIODevice::ReadWrite | QIODevice::Unbuffered );
	m_process->start();
	QEventLoop loop;
	m_loop = &loop;
	bool ret = loop.exec( QEventLoop::WaitForMoreEvents );
	m_loop = 0;

	delete m_process;
	m_process = NULL;
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
}

void RARInterface::writeToProcess( const QByteArray &data )
{
	if ( !m_process || data.isNull() )
		return;

	//m_process->write( data );
	kDebug( 1601 ) << data;
	m_process->pty()->write( data );
}

void RARInterface::started()
{
	//m_state = 0;
	m_errorMessages.clear();
	m_userCancelled = false;
}

void RARInterface::finished( int exitCode, QProcess::ExitStatus exitStatus)
{
	if ( !m_process )
		return;

	progress(1.0);

	if ( m_loop )
	{
		m_loop->exit( exitStatus == QProcess::CrashExit ? 1 : 0 );
	}
}

void RARInterface::readFromStderr()
{
	if ( !m_process )
		return;

	QByteArray stdErrData = m_process->readAllStandardError();

	kDebug( 1601 ) << "ERROR" << stdErrData.size() << stdErrData;

	if ( !stdErrData.isEmpty() )
	{
		if (handlePasswordPrompt(stdErrData))
			return;
		//else if (handleOverwritePrompt(stdErrData))
		//	return;
		else
		{
			m_errorMessages << QString::fromLocal8Bit(stdErrData);
		}
	}
}

bool RARInterface::handlePasswordPrompt(const QByteArray &message)
{
	if (message.contains(RAR_PASSWORD_PROMPT_STR))
	{
		Kerfuffle::PasswordNeededQuery query(filename());
		emit userQuery(&query);
		query.waitForResponse();

		if (query.responseCancelled()) {
			m_userCancelled = true;
			m_process->kill();
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

// currently, this does not work
bool RARInterface::handleOverwritePrompt(const QByteArray &message)
{
	QString line = QString::fromLocal8Bit(message);
	if (line.contains(RAR_OVERWRITE_PROMPT_STR)) {
		QString filename = line.left(line.indexOf(RAR_OVERWRITE_PROMPT_STR)).trimmed();
		Kerfuffle::OverwriteQuery query(filename);
		emit userQuery(&query);
		query.waitForResponse();

		if (query.responseCancelled())
			writeToProcess(QByteArray("Q\r"));
		else if (query.responseOverwriteAll())
			writeToProcess(QByteArray("A\r"));
		else if (query.responseOverwrite())
			writeToProcess(QByteArray("Y\r"));
		else if (query.responseRename())
		{
			writeToProcess(QByteArray("R\r"));
			m_newFilename = query.newFilename();
		}

		return true;
	}
	else if (line.contains(RAR_ENTER_NEW_NAME_PROMPT_STR) && !m_newFilename.isEmpty())
	{
		writeToProcess(m_newFilename.toLocal8Bit() + '\n');
		m_newFilename.clear();
		return true;
	}
	else
	{
		return false;
	}
}
KERFUFFLE_PLUGIN_FACTORY( RARInterface )

