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
			m_status(Header)
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

				p[ListArgs] = QStringList() << "-l" << "$Archive";
				p[ExtractArgs] = QStringList() << "$PreservePathSwitch" << "$PasswordSwitch" << "$RootNodeSwitch" << "$Archive" << "$Files";
				p[PreservePathSwitch] = QStringList() << "" << "-j";
				p[RootNodeSwitch] = QStringList() << "-ap$Path";
				p[PasswordSwitch] = QStringList() << "-P$Password";

				p[DeleteArgs] = QStringList() << "-d" << "$Archive" << "$Files";

				p[FileExistsExpression] = "^replace (.+)?";
				p[FileExistsInput] = QStringList()
					<< "y" //overwrite
					<< "n" //skip
					<< "A" //overwrite all
					<< "N" //autoskip
					<< "N" //cancel
					;

				p[AddArgs] = QStringList() << "-r" << "$Archive" << "$Files";

				p[WrongPasswordPatterns] = QStringList() << "incorrect password";
				//p[ExtractionFailedPatterns] = QStringList() << "CRC failed";
			}
			return p;
		}




		QString m_entryFilename, m_internalId;
		ArchiveEntry m_currentEntry;
		
		enum ReadStatus {
			Header = 0,
			Entry
		};
		
		ReadStatus m_status;

		bool readListLine(QString line)
		{
			static QRegExp entryPattern(
					"^(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(.+)$");

			int i;
			switch (m_status) {
				case Header:
					m_status = Entry;
					break;
				case Entry:
					i = entryPattern.indexIn(line);
					if (i != -1) {
						ArchiveEntry e;
						e[Permissions] = entryPattern.cap(1);
						e[Owner] = entryPattern.cap(3);
						e[Size] = entryPattern.cap(4).toInt();
						QString status = entryPattern.cap(5);
						if (status[0].isUpper())
							e[IsPasswordProtected] = true;
						e[CompressedSize] = entryPattern.cap(6).toInt();
						e[FileName] = e[InternalID] = entryPattern.cap(10);
						entry(e);
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

