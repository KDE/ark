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
#ifndef RARPLUGIN_H
#define RARPLUGIN_H

#include "kerfuffle/archiveinterface.h"

using namespace Kerfuffle;

class RARInterface: public ReadWriteArchiveInterface
{
	Q_OBJECT
	public:
		explicit RARInterface( const QString & filename, QObject *parent = 0 );
		~RARInterface();

		bool list();
		bool copyFiles( const QList<QVariant> & files, const QString & destinationDirectory, bool preservePaths );

		bool addFiles( const QStringList & files );
		bool deleteFiles( const QList<QVariant> & files );

	private:
		void processListLine( const QString &line);
		QString m_headerString, m_entryFilename;
		bool m_isFirstLine, m_incontent;
		QString m_filename, m_rarpath, m_unrarpath;
};

#endif // RARPLUGIN_H
