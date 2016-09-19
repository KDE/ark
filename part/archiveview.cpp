/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008-2009 Harald Hvaal <haraldhv (at@at) stud.ntnu.no>
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

#include "archiveview.h"
#include "ark_debug.h"

#include <QDebug>

#include <QMimeData>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QMouseEvent>

ArchiveView::ArchiveView(QWidget *parent)
    : QTreeView(parent)
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAlternatingRowColors(true);
    setAnimated(true);
    setAllColumnsShowFocus(true);
    setSortingEnabled(true);
    setDragEnabled(true);
    setDropIndicatorShown(true);
    // #368807: drops must be initially disabled, otherwise they will override the MainWindow's ones.
    // They will be enabled in Part::slotLoadingFinished().
    setDropsEnabled(false);
}

void ArchiveView::startDrag(Qt::DropActions supportedActions)
{
    //only start the drag if it's over the filename column. this allows dragging selection in
    //tree/detail view
    if (currentIndex().column() != 0) {
        return;
    }

    QTreeView::startDrag(supportedActions);
}

void ArchiveView::setDropsEnabled(bool enabled)
{
    setAcceptDrops(enabled);
    setDragDropMode(enabled ? QAbstractItemView::DragDrop : QAbstractItemView::NoDragDrop);
}

void ArchiveView::dragEnterEvent(QDragEnterEvent * event)
{
    //TODO: if no model, trigger some mechanism to create one automatically!
    qCDebug(ARK) << "dragEnterEvent" << event;

    if (event->source() == this) {
        //we don't support internal drops yet.
        return;
    }

    QTreeView::dragEnterEvent(event);
}

void ArchiveView::dropEvent(QDropEvent * event)
{
    qCDebug(ARK) << "dropEvent" << event;

    if (event->source() == this) {
        //we don't support internal drops yet.
        return;
    }

    QTreeView::dropEvent(event);
}

void ArchiveView::dragMoveEvent(QDragMoveEvent * event)
{
    qCDebug(ARK) << "dragMoveEvent" << event;

    if (event->source() == this) {
        //we don't support internal drops yet.
        return;
    }

    QTreeView::dragMoveEvent(event);
    if (event->mimeData()->hasFormat(QStringLiteral("text/uri-list"))) {
        event->acceptProposedAction();
    }
}
