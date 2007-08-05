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
#ifndef BKPLUGIN_H
#define BKPLUGIN_H

#include "kerfuffle/archiveinterface.h"
#include "bk.h"

using namespace Kerfuffle;

class BKInterface: public ReadWriteArchiveInterface
{
	Q_OBJECT
	public:
		explicit BKInterface( const QString & filename, QObject *parent = 0 );
		~BKInterface();

		bool list();
		bool copyFiles( const QList<QVariant> & files, const QString & destinationDirectory, bool preservePaths );

		bool addFiles( const QStringList & files );
		bool deleteFiles( const QList<QVariant> & files );

	private:
		bool browse( BkFileBase* base, const QString& prefix = QString() );

		VolInfo m_volInfo;
};

#endif // BKPLUGIN_H
