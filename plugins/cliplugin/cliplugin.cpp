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
#include "kerfuffle/cliinterface.h"
#include "kerfuffle/archivefactory.h"

using namespace Kerfuffle;

class CliPlugin: public CliInterface
{
	public:
		explicit CliPlugin( const QString & filename, QObject *parent = 0 )
			: CliInterface( filename, parent )
		{

		}

		virtual ~CliPlugin()
		{

		}

		virtual ParameterList parameterList() const
		{
			static ParameterList p;
			if (p.isEmpty()) {

				p[ListProgram] = p[ExtractProgram] = p[DeleteProgram] = p[AddProgram] = "rar";

				p[ListArgs] = QStringList() << "v" << "-c-" << "$Archive";
				p[ExtractArgs] = QStringList() << "v" << "-c-" << "$Archive" << "$Files";

			}
			return p;
		}
};

KERFUFFLE_PLUGIN_FACTORY(CliPlugin)

