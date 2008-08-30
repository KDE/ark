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

	KProcess kp;
	kp << m_exepath << "l" << "-slt" << m_filename;
	kp.setOutputChannelMode(KProcess::MergedChannels);

	kp.start();

	if (!kp.waitForStarted()){
		kDebug( 1601 ) << m_exepath << "did not start";
		return false;
	}


	if (!kp.waitForFinished()) {
		kDebug( 1601 ) << m_exepath << "failed to finish";
		return false;
	}

	int state = 0;
	while (kp.canReadLine())
	{
		processLine(state, QString::fromLocal8Bit(kp.readLine()));
	}

	kDebug( 1601 ) << m_exepath << "finished";
	return true;
}


void p7zipInterface::processLine(int& state, const QString& line)
{
	switch (state)
	{
		case 0: // header
			if (line.startsWith("Listing archive:"))
			{
				kDebug( 1601 ) << "Archive name: " << line.right(line.size() - 16).trimmed() ;
			}
			else if (line.startsWith("----------"))
				state = 1;
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
				m_currentArchiveEntry[ IsDirectory ] = attributes.startsWith('D');
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
				bool isPasswordProtected = method.contains("AES");
				m_currentArchiveEntry[ IsPasswordProtected ] = isPasswordProtected;
			}
			else if (line.startsWith("Block = "))
			{
				// do nothing
			}
			else 	// assume end of file details
			{
				if (m_currentArchiveEntry.contains(Size) && m_currentArchiveEntry.contains(CompressedSize))
				{
					m_currentArchiveEntry[ Ratio ] = m_currentArchiveEntry[ CompressedSize ].toDouble() / m_currentArchiveEntry[Size].toDouble();
				}
				else
				{
					m_currentArchiveEntry[ Ratio ] = 0.0;
				}

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

	kDebug( 1601 ) << files  << destinationDirectory << (preservePaths? " with paths":"");
	
	if (m_exepath.isNull())
	{
		return false;
	}
	
	KProcess kp;
	kp.setOutputChannelMode(KProcess::MergedChannels);
	kp << m_exepath;
	if (preservePaths)
	{
		kp << "x"; 
	} else
	{
		kp << "e";
	}

	if ( !password().isEmpty() ) kp << "-p" + password();
	
	kp << "-o" + destinationDirectory;
	kp << m_filename;

	foreach( const QVariant& file, files )
	{
		kDebug( 1601 ) << file.toString().trimmed();
		kp << file.toString().trimmed();
	}

	QStringList args = kp.program();
	foreach (QString s, args)
		kDebug( 1601 ) << s;
	
	kp.start();

	if (!kp.waitForStarted())
	{
		kDebug( 1601 ) << m_exepath << "did not start";
		return false;
	}
	if (!kp.waitForFinished())
	{
		kDebug( 1601 ) << m_exepath << " did not finish";
		return false;
	}

	while (kp.canReadLine())
		kDebug( 1601 ) << kp.readLine();

	kDebug( 1601 ) << "Finished reading output";
	return true;
}

bool p7zipInterface::addFiles( const QString& path, const QStringList & files )
{
	kDebug( 1601 ) << "Will try to add path " << path << " files " << files << " to " << m_filename << " using " << m_exepath;

	if (m_exepath.isNull())
	{
		return false;
	}

	KProcess kp;
	kp << m_exepath << "a";
	kp << "-bd"; // supress percentage indicator
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
		QStringList lines = QString(kp.readAll()).split("\n");
		foreach(QString line, lines) {
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

