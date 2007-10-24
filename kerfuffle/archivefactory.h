/*
 * ark -- archiver for the KDE project
 *
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
#ifndef ARCHIVEFACTORY_H
#define ARCHIVEFACTORY_H

#include "kerfuffle_export.h"
#include "archive.h"
#include "archivebase.h"
#include <QString>
#include <QObject>


namespace Kerfuffle
{
	class ArchiveFactory
	{
		public:
			ArchiveFactory() {}
			virtual ~ArchiveFactory() {}
			virtual Kerfuffle::Archive* createArchive( const QString& filename, QObject *parent ) = 0;
	};

	template<class T> class ArchiveInterfaceFactory: public ArchiveFactory
	{
		public:
			Kerfuffle::Archive* createArchive( const QString& filename, QObject *parent = 0 )
			{
				return new ArchiveBase( new T( filename, parent ) );
			}
	};
} // namespace Kerfuffle

#define KERFUFFLE_PLUGIN_FACTORY( classname ) \
	extern "C" { KDE_EXPORT Kerfuffle::ArchiveFactory *pluginFactory() { return new Kerfuffle::ArchiveInterfaceFactory<classname>(); } }

#endif // ARCHIVEFACTORY_H
