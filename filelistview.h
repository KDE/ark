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
class QRegExp;

class KListView;

class ArkWidget;
class Archive;

/* Columns */
#define nameColumn           0
#define typeColumn           1
#define sizeColumn           2
#define compressedSizeColumn 3
#define ratioColumn          4
#define timeStampColumn      5
#define crcColumn            6


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

    /**
     * Selects all items matching a regular expression, and only them.
     * @param regExp The regular expression that describes which items should be selected.
     */
    void select( const QRegExp& regExp );

    QStringList selectedFilenames() const;

    /**
     * Is the selection empty, or is there any item selected?
     * @return true if no item is selected.
     */
    bool isSelectionEmpty();

    /**
     * Sets the archive this view should show.
     * @param archive Pointer to the archive that should be shown. A value of 0 means that this view should not be attached to an archive.
     */
    void setArchive( Archive * archive = 0 );

  private slots:
    /**
     * Adds an entry to the listing
     * @param entry The entry to be added
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
