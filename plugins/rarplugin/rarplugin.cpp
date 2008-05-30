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

#include <QFile>
#include <QDateTime>
#include <KProcess>
#include <KStandardDirs>
#include <KDebug>
#include <KLocale>

RARInterface::RARInterface( const QString & filename, QObject *parent )
	: ReadWriteArchiveInterface( filename, parent )
{
	kDebug( 1601 ) << "Rar plugin opening " << filename ;
	m_filename=filename;

	m_rarpath = KStandardDirs::findExe( "rar" );
	m_unrarpath = KStandardDirs::findExe( "unrar" );
 	bool have_rar = !m_rarpath.isNull();
 	bool have_unrar = !m_unrarpath.isNull();
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
	KProcess kp;
	if (!m_unrarpath.isNull()) kp << m_unrarpath << "v" << "-c-" << m_filename;
	else if (!m_rarpath.isNull()) kp << m_rarpath << "v" << "-c-" << m_filename;
	else return false;
	kp.setOutputChannelMode(KProcess::MergedChannels);
	kp.start();
	if (!kp.waitForStarted()){
		kDebug( 1601 ) << "Rar did not start";
		return false;
	}
	if (!kp.waitForFinished()) {
		kDebug( 1601 ) << "Rar did not finish";
		return false;
	}
	while (kp.canReadLine()){
		processListLine( kp.readLine());
	}
	kDebug( 1601 ) << "Finished reading rar output";
	return true;
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
		m_entryFilename = line;
		m_entryFilename.chop(1); // handle newline
		m_entryFilename.remove( 0, 1 ); // and the dumb spaces in front
		m_isFirstLine = false;
		return;
	}

	QStringList fileprops = line.split(' ', QString::SkipEmptyParts);
	kDebug( 1601 ) << m_entryFilename << " : " << fileprops ;
	ArchiveEntry e;
	e[ FileName ] = m_entryFilename;
	e[ InternalID ] = m_entryFilename.toUtf8();
	e[ Size ] = fileprops[ 0 ];
	e[ CompressedSize] = fileprops[ 1 ];
	e[ Ratio ] = fileprops[ 2 ];
	QDateTime ts (QDate::fromString(fileprops[ 3 ], "dd-mm-yy"),
		QTime::fromString(fileprops[ 4 ], "hh:mm"));
	e[ Timestamp ] = ts;
	e[ IsDirectory ] = fileprops[ 5 ].startsWith('d');
	e[ Permissions ] = fileprops[ 5 ].remove(0,1);
	e[ CRC ] = fileprops[ 6 ];
	e[ Method ] = fileprops[ 7 ];
	fileprops[ 8 ].chop(1); // handle newline
	e[ Version ] = fileprops[ 8 ];
	kDebug( 1601 ) << "Added entry: " << e ;
	entry(e);
	m_isFirstLine = true;
	return;

}

bool RARInterface::copyFiles( const QList<QVariant> & files, const QString & destinationDirectory, bool preservePaths )
{
	kDebug( 1601 ) << files  << destinationDirectory << (preservePaths? " with paths":"");
	KProcess kp;
	kp.setOutputChannelMode(KProcess::MergedChannels);
	if (!m_unrarpath.isNull()) kp << m_unrarpath;
	else if (!m_rarpath.isNull()) kp << m_rarpath;
	else return false;
	if (preservePaths) {
		kp << "x"; 
	} else {
		kp << "e";
	}
	kp << m_filename;
	foreach( const QVariant& file, files )
	{
		kDebug( 1601 ) << file.toString();
		kp << file.toString();
	}
	kp << destinationDirectory;
	kp.start();
	if (!kp.waitForStarted()){
		kDebug( 1601 ) << "Rar did not start";
		return false;
	}
	if (!kp.waitForFinished()) {
		kDebug( 1601 ) << "Rar did not finish";
		return false;
	}
	kDebug( 1601 ) << "Finished reading rar output";
	return true;
}

bool RARInterface::addFiles( const QStringList & files )
{
	kDebug( 1601 ) << "Will try to add " << files << " to " << m_filename;
  return false;
}

bool RARInterface::deleteFiles( const QList<QVariant> & files )
{
	kDebug( 1601 ) << "Will try to delete " << files << " from " << m_filename;
  return false;
}

KERFUFFLE_PLUGIN_FACTORY( RARInterface )

