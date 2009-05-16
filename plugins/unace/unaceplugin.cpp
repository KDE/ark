/*
 * unace plugin for ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Rafał Rzepecki <divided.mind@gmail.com>
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
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

#include <KDebug>
#include <KLocale>

#include <kerfuffle/archive.h>
#include <kerfuffle/archivefactory.h>

UnAceInterface::UnAceInterface( const QString & filename, QObject *parent )
		: CliInterface ( filename, parent ),
		m_hadHeader(false)
{

}

bool UnAceInterface::readListLine( QString line )
{
	static QRegExp header(
			"^\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+$");

	static QRegExp pattern(
			"^(\\S+\\s\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s*$");
	kDebug() << line << pattern.indexIn(line) << header.indexIn(line);

	const char* const EXPECTED_HEADER[] = { "Date", "Time", "Packed", "Size", "Ratio", "File" };

	if ( !m_hadHeader ) { // header should follow, let's check if it's as expected
		if ( header.indexIn(line) == -1 )
			// this is not the listing yet
			return true;

		for ( int i = 1; i <= 6; i++ )
			if ( header.cap(i) != EXPECTED_HEADER[i - 1] ) {
				error( i18n( "Unexpected output from the unace process." ) );
				return false;
			}
		kDebug() << "found header";
		m_hadHeader = true;
		return true;
	}

	if ( pattern.indexIn(line) == -1 )
		// this is not the listing yet
		return true;

	ArchiveEntry the_entry;
	const QString timestamp = pattern.cap(1);

	the_entry[Timestamp] = QDateTime::fromString( timestamp, "dd.MM.yy HH:mm" );
	the_entry[CompressedSize] = pattern.cap(2).toLongLong();
	the_entry[Size] = pattern.cap(3).toLongLong();
	the_entry[Ratio] = pattern.cap(4).left( pattern.cap(4).size() - 1 ).toInt();
	the_entry[FileName] = the_entry[InternalID] = pattern.cap(5);

	entry( the_entry );
	return true;
}

ParameterList UnAceInterface::parameterList() const
{
	static ParameterList p;
	if (p.isEmpty()) {

		p[CaptureProgress] = false;
		p[ListProgram] = p[ExtractProgram] = p[DeleteProgram] = p[AddProgram] = "unace";

		p[ListArgs] = QStringList() << "l" << "$Archive";
		p[ExtractArgs] = QStringList() << "$PreservePathSwitch" << "$Archive" << "$Files";
		p[PreservePathSwitch] = QStringList() << "x" << "e";
		p[RootNodeSwitch] = QStringList() << "";
		p[PasswordSwitch] = QStringList() << "";

		p[DeleteArgs] = QStringList() << "";
		p[AddArgs] = QStringList() << "";
		p[ExtractionFailedPatterns] = QStringList() << "CRC failed";

		p[FileExistsExpression] = "^\\s+File already exists: (.+)$";
		p[FileExistsInput] = QStringList()
			<< "Y" //overwrite
			<< "N" //skip
			<< "A" //overwrite all
			<< "N" //autoskip
			<< "C" //cancel
			;

	}
	return p;

}

#include "unaceplugin.moc"

KERFUFFLE_PLUGIN_FACTORY( UnAceInterface )
