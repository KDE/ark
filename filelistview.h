/*

  ark -- archiver for the KDE project

  Copyright (C)

  1999: Francois-Xavier Duranceau duranceau@kde.org
  1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
  2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
  2003: Georg Robbers <Georg.Robbers@urz.uni-hd.de>

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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef FILELISTVIEW_H
#define FILELISTVIEW_H

#include <qlistview.h>
#include <qdatetime.h>

#include <klistview.h>

#include "archiveentry.h"

class QString;
class QStringList;
class QRect;
class QPainter;
class QColorGroup;
class QMouseEvent;
class QPoint;

class KListView;

class ArkWidget;
class Archive;

/* Columns */
#define nameColumn           0
#define typeColumn           1
#define sizeColumn           2
#define compressedSizeColumn 997
#define ratioColumn          998
#define timeStampColumn      3
#define crcColumn            999


class ArkListViewItem : public KListViewItem
{
  public:
    ArkListViewItem( const ArchiveEntry & entry, KListView * listView );

    int compare ( QListViewItem * i, int col, bool ascending ) const;
    virtual QString key(int column, bool) const;

    ArchiveEntry entry() const { return m_entry; }

    QString   path() const { return m_entry.path(); }
    QString   mimeType() { return m_entry.mimeType(); }
    Q_UINT64  size() const { return m_entry.size(); }
    Q_UINT64  compressedSize() const { return m_entry.compressedSize(); }
    float     compressionRatio() const { return m_entry.compressionRatio(); }
    QDateTime timeStamp() const { return m_entry.timeStamp(); }
    Q_UINT64  crc() const { return m_entry.crc(); }

  private:
    ArchiveEntry m_entry;
};


class ArkView : public KListView
{
  Q_OBJECT

  public:
    ArkView( QWidget* parent = 0, const char* name = 0 );
    ArkListViewItem *currentItem() { return static_cast< ArkListViewItem * >( KListView::currentItem()); }

    QStringList selectedFilenames() const;
    bool isSelectionEmpty();

    void setArchive( Archive * archive = 0 );

  public slots:
    /**
    * Adds a file and stats to the file listing
    * @param entries A stringlist of the entries for each column of the list.
    */
    void addItem( const ArchiveEntry & entry );

  signals:
    void startDragRequest( const QStringList & fileList );

  protected:
    void contentsMouseReleaseEvent( QMouseEvent *e );
    void contentsMousePressEvent( QMouseEvent *e );
    void contentsMouseMoveEvent( QMouseEvent *e );

  private:
    Archive *m_archive;
    bool m_pressed;
    QPoint presspos;  // this will save the click pos to correctly recognize drag events
};

#endif
