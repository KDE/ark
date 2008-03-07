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
#include <KMessageBox>
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
	if (!have_rar||!have_unrar) {
		KMessageBox::error( 0, i18n( "Neither rar or unrar are available in your PATH." ) );
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
	kp << m_unrarpath << "v" << "-c-" << m_filename;
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
		processLine( kp.readLine());
	}
	kDebug( 1601 ) << "Finished reading rar output";
	return true;
}

void RARInterface::processLine(const QString& line)
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
		m_entryFilename = line ;
		m_entryFilename.remove( 0, 1 );
		m_isFirstLine = false;
		return;
	}
	kDebug( 1601 ) << "Rar output: " << line ;

	QStringList fileprops = line.split(' ');
	ArchiveEntry e;
	e[ FileName ] = m_entryFilename;
	e[ Size ] = fileprops[ 0 ];
	e[ CompressedSize] = fileprops[ 1 ];
	e[ Ratio ] = fileprops[ 2 ];
	QDateTime ts (QDate::fromString(fileprops[ 3 ], "dd-mm-yy"),
		QTime::fromString(fileprops[ 4 ], "hh:mm"));
	e[ Timestamp] = ts;
	e[ Permissions ] = fileprops[ 5 ];
	e[ CRC ] = fileprops[ 6 ];
	e[ Method ] = fileprops[ 7 ];
	e[ Version ] = fileprops[ 8 ];
	kDebug( 1601 ) << "Rar parsed: " << e ;
	entry(e);
	m_isFirstLine = true;
	return;

}

bool RARInterface::copyFiles( const QList<QVariant> & files, const QString & destinationDirectory, bool preservePaths )
{
	foreach( const QVariant& file, files )
	{
		int rc;

		kDebug( 1601 ) << "Trying to extract " << file.toByteArray() ;
		//rc = bk_extract( &m_volInfo, file.toByteArray(), QFile::encodeName( destinationDirectory ), true, 0 );
		//if ( rc <= 0 )
		//{
		//	error( QString( "Could not extract '%1'" ).arg( file.toString() ) );
		//	return false;
		//}
	}
	return true;
}

bool RARInterface::addFiles( const QStringList & files )
{
  return false;
}

bool RARInterface::deleteFiles( const QList<QVariant> & files )
{
  return false;
}

KERFUFFLE_PLUGIN_FACTORY( RARInterface )

