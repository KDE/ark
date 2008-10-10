/*
 * 7zipplugin -- plugin for KDE ark
 *
 * Copyright (C) 2008 Chen Yew Ming <fusion82@gmail.com>
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
#ifndef SEVENZIPPLUGIN_H
#define SEVENZIPPLUGIN_H

#include "kerfuffle/archiveinterface.h"
class QByteArray;
class KProcess;

using namespace Kerfuffle;


class p7zipInterface: public ReadWriteArchiveInterface
{
	Q_OBJECT
	public:
		explicit p7zipInterface( const QString & filename, QObject *parent = 0 );
		~p7zipInterface();

		bool list();
		bool copyFiles( const QList<QVariant> & files, const QString & destinationDirectory, Archive::CopyFlags flags );

		bool addFiles( const QStringList & files, const CompressionOptions& options );
		bool deleteFiles( const QList<QVariant> & files );

	private:
		void processLine(int& state, const QString& line);
		QString m_filename;
		QString m_exepath;
		ArchiveEntry m_currentArchiveEntry;
};

#endif // SEVENZIPPLUGIN_H
