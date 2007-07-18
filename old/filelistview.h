/*

  ark -- archiver for the KDE project

  Copyright (C) 1999 Francois-Xavier Duranceau <duranceau@kde.org>
  Copyright (C) 1999-2000 Corel Corporation (author: Emily Ezust <emilye@corel.com>)
  Copyright (C) 2001 Corel Corporation (author: Michael Jarrett <michaelj@corel.com>)
  Copyright (C) 2003 Georg Robbers <Georg.Robbers@urz.uni-hd.de>
  Copyright (C) 2005 Henrique Pinto <henrique.pinto@kdemail.net>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#ifndef FILELISTVIEW_H
#define FILELISTVIEW_H

#include <QDateTime>
#include <QPair>
#include <QTreeWidget>
#include <QHash>

#include <kio/global.h>

#include "kerfuffle/arch.h"

class QString;
class QStringList;
class QMouseEvent;
class QPoint;

enum columnName { sizeCol = 1 , packedStrCol, ratioStrCol, timeStampStrCol, otherCol };

typedef QList< QPair< QString, Qt::AlignmentFlag > > ColumnList;

class FileListView: public QTreeWidget
{
	Q_OBJECT
	public:
		FileListView( QWidget *parent = 0 );

		/**
		 * Returns the full names of the selected files.
		 */
		QStringList selectedFilenames();

		/**
		 * Return the full names of all files.
		 */
		QStringList fileNames();

		/**
		 * Returns true if no file is selected
		 */
		bool isSelectionEmpty();

		/**
		 * Returns the file item, or 0 if not found.
		 * @param filename The filename in question to reference in the archive
		 * @return The requested file's FileLVI
		 */
		ArchiveEntry item( const QString& filename );

		ArchiveEntry currentEntry() const;

		/**
		 * Returns the number of files in the archive.
		 */
		int totalFiles();

		/**
		 * Returns the number of selected files.
		 */
		int selectedFilesCount();

		/**
		 * Return the sum of the sizes of all files in the archive.
		 */
		KIO::filesize_t totalSize();

		/**
		 * Return the sum of the sizes of the selected files.
		 */
		KIO::filesize_t selectedSize();

	public slots:
		/**
		 * Adds a file and stats to the file listing
		 * @param entries A stringlist of the entries for each column of the list.
		 */
		void addItem( const ArchiveEntry & entry );

	private:
		void setHeaders();
		QTreeWidgetItem* findParent( const QString& fullname );
		QStringList childrenOf( QTreeWidgetItem* parent );

		QMap<int, columnName> m_columnMap;
		bool m_pressed;
		QPoint m_presspos;  // this will save the click pos to correctly recognize drag events
		QHash<QTreeWidgetItem*, ArchiveEntry> m_entryMap;
};

#endif
// kate: space-indent off; tab-width 4;
