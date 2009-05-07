/*
 * ark -- archiver for the KDE project
 *
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#include "kerfuffle/cliinterface.h"
#include "kerfuffle/archivefactory.h"

#include <QDir>
#include <QString>
#include <QStringList>
#include <KDebug>

using namespace Kerfuffle;

class CliPlugin: public CliInterface
{
	public:
		explicit CliPlugin( const QString & filename, QObject *parent = 0 )
			: CliInterface( filename, parent ),
			m_status(PreHeader)
		{

		}

		virtual ~CliPlugin()
		{

		}

		virtual ParameterList parameterList() const
		{
			static ParameterList p;
			if (p.isEmpty()) {

				p[CaptureProgress] = false;
				p[ListProgram] = "zipinfo";
				p[ExtractProgram] = "unzip";
				p[DeleteProgram] = p[AddProgram] = "zip";

				p[ListArgs] = QStringList() << "-v" << "$Archive";
				p[ExtractArgs] = QStringList() << "-p-" << "$PreservePathSwitch" << "$PasswordSwitch" << "$RootNodeSwitch" << "$Archive" << "$Files";
				p[PreservePathSwitch] = QStringList() << "x" << "e";
				p[RootNodeSwitch] = QStringList() << "-ap$Path";
				p[PasswordSwitch] = QStringList() << "-p$Password";

				p[DeleteArgs] = QStringList() << "d" << "$Archive" << "$Files";

				p[FileExistsExpression] = "^(.+) already exists. Overwrite it";
				p[FileExistsInput] = QStringList()
					<< "Y" //overwrite
					<< "N" //skip
					<< "A" //overwrite all
					<< "E" //autoskip
					<< "Q" //cancel
					;

				p[AddArgs] = QStringList() << "a" << "$Archive" << "$Files";

				p[WrongPasswordPatterns] = QStringList() << "password incorrect";
				p[ExtractionFailedPatterns] = QStringList() << "CRC failed";
			}
			return p;
		}




		bool m_isFirstLine, m_incontent, m_isPasswordProtected;
		QString m_entryFilename, m_internalId;
		ArchiveEntry m_currentEntry;
		
		enum ReadStatus {
			PreHeader = 0,
			Header,
			EntryHeader,
			EntryBody
		};
		
		ReadStatus m_status;

		bool readListLine(QString line)
		{

			const QString m_headerString = "--------";
			QString trimmed = line.trimmed();

			switch (m_status) {
				case PreHeader:
					if (line.startsWith(m_headerString))
						m_status = Header;
					break;
				case Header:
					if (line.startsWith(m_headerString))
						m_status = EntryHeader;
					break;
				case EntryHeader:
					if (!trimmed.isEmpty()) {
						m_currentEntry[FileName] = trimmed;
						m_status = EntryBody;
					}
					break;
				case EntryBody:
					if (line.startsWith(m_headerString)) {
						m_status = EntryHeader;
						entry(m_currentEntry);
						m_currentEntry = ArchiveEntry();
					} else {
						handleDataLine(trimmed);
					}
					break;

			}
			return true;
		}

		void handleDataLine(QString line)
		{
			static QRegExp pattern("^(.+):\\s+(.+)$");

			if (pattern.indexIn(line) == -1)
				return;

			QString field = pattern.cap(1);
			QString value = pattern.cap(2);

			if (line.startsWith("compressed size"))
				m_currentEntry[CompressedSize] = 100;

#if 0
			ArchiveEntry e;
			e[ FileName ] = m_entryFilename;
			e[ InternalID ] = m_internalId;
			e[ Size ] = fileprops[ 0 ];
			e[ CompressedSize] = fileprops[ 1 ];
			e[ Ratio ] = fileprops[ 2 ];
			e[ Timestamp ] = ts;
			e[ IsDirectory ] = isDirectory;
			e[ Permissions ] = fileprops[ 5 ].remove(0,1);
			e[ CRC ] = fileprops[ 6 ];
			e[ Method ] = fileprops[ 7 ];
			e[ Version ] = fileprops[ 8 ];
			e[ IsPasswordProtected] = m_isPasswordProtected;
#endif
		}
};

KERFUFFLE_PLUGIN_FACTORY(CliPlugin)

