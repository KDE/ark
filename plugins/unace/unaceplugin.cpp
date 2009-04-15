/*
 * unace plugin for ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Rafał Rzepecki <divided.mind@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "unaceplugin.h"

#include <QEventLoop>
#include <QThread>

#include <KDebug>
#include <KLocale>
#include <KProcess>

#include <kerfuffle/archive.h>
#include <kerfuffle/archivefactory.h>

UnAceInterface::UnAceInterface( const QString & filename, QObject *parent )
		: ReadOnlyArchiveInterface( filename, parent )
{}

UnAceInterface::~UnAceInterface()
{}

bool UnAceInterface::list()
{
	KProcess unace;
	QStringList args;
	args << "l" << filename();
	unace.setProgram( "unace", args );
	unace.setOutputChannelMode( KProcess::OnlyStdoutChannel );

	m_hadHeader = false;
	unace.start();
	bool started = unace.waitForStarted();
	while ( unace.state() == KProcess::Running ) {
		unace.waitForReadyRead();
		while ( unace.canReadLine() ) {
			char buf[1024];
			qint64 lineLength = unace.readLine( buf, sizeof(buf) );

			if ( buf[lineLength - 1] == '\n' )
				lineLength--;

			const QByteArray bytes( buf, lineLength );
			if ( !processListLine( bytes ) )
				return false;
		}
	}

  if ( !started )
    error( i18n( "Unable to launch <tt>unace</tt>. Make sure you have that tool installed and available." ) );

	return started && (unace.exitStatus() == QProcess::NormalExit) && (unace.exitCode() == 0);
}

bool UnAceInterface::processListLine( const QByteArray &bytes )
{
	const QList<QByteArray> fields( bytes.split('\xb3') );
	if ( fields.size() != 6 )
		// this is not the listing yet
		return true;

	const char* const EXPECTED_HEADER[] = { "Date    ", "Time ", "Packed     ", "Size     ", "Ratio", "File            " };

	if ( !m_hadHeader ) { // header should follow, let's check if it's as expected
		for ( int i = 0; i < 6; i++ )
			if ( fields[i] != EXPECTED_HEADER[i] ) {
				error( i18n( "Unexpected output from the unace process." ) );
				return false;
			}
		m_hadHeader = true;
		return true;
	}

	ArchiveEntry the_entry;
	const QString timestamp = QString::fromAscii( fields[0] ) + ' ' +	QString::fromAscii(fields[1]);

	the_entry[Timestamp] = QDateTime::fromString( timestamp, "dd.MM.yy HH:mm" );
	the_entry[CompressedSize] = QString::fromAscii( fields[2] ).toLongLong();
	the_entry[Size] = QString::fromAscii( fields[3] ).toLongLong();
	the_entry[Ratio] = QString::fromAscii( fields[4] ).left( fields[4].size() - 1 ).toInt();
	the_entry[FileName] = QString::fromUtf8( fields[5] ).mid( 1 );

	entry( the_entry );
	return true;
}

bool UnAceInterface::copyFiles( const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options )
{
	if ( files.size() > 0 ) { // TODO
		error( i18n( "Only extracting full archive is currently supported." ) );
		return false;
	}

	if ( !options.contains( "PreservePaths" ) || !options["PreservePaths"].toBool() ) { // TODO
		error( i18n( "Only extracting while preserving paths is currently supported." ) );
		return false;
	}

	if ( options.contains( "RootNode" ) ) { // TODO
		error( i18n( "Extracting with root node other than default is not currently supported." ) );
		return false;
	}

	KProcess unace;
	QStringList args;
	args << "x" << filename();
	unace.setProgram( "unace", args );
	unace.setOutputChannelMode( KProcess::SeparateChannels );
	unace.setWorkingDirectory( destinationDirectory );

	unace.start();
	bool started = unace.waitForStarted();
	if ( !started ) {
		error( i18n( "Unable to launch <tt>unace</tt>. Make sure you have that tool installed and available." ) );
		return false;
	}

	unace.waitForFinished();

	if ( unace.exitStatus() != QProcess::NormalExit ) {
		error( i18n( "<tt>unace</tt> crashed unexpectedly." ) );
		return false;
	}

	if ( unace.exitCode() != 0 ) {
		error( i18n( "<tt>unace</tt> has encountered error when extracting:<pre>%1</pre>", QString::fromUtf8( unace.readAllStandardError() ) ) );
		return false;
	}

	return true;
}

#include "unaceplugin.moc"

KERFUFFLE_PLUGIN_FACTORY( UnAceInterface )
