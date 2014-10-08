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

#include <KDebug>
#include <KGlobalSettings>

#include <QMimeData>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QMouseEvent>

ArchiveView::ArchiveView(QWidget *parent)
        : QTreeView(parent)
        , m_mouseButtons(Qt::NoButton)
{
    connect(this, &ArchiveView::pressed, this, &ArchiveView::updateMouseButtons);
    connect(this, &ArchiveView::clicked, this, &ArchiveView::slotClicked);
    connect(this, &ArchiveView::doubleClicked, this, &ArchiveView::slotDoubleClicked);
}

// FIXME: this is a workaround taken from Dolphin until QTBUG-1067 is resolved
void ArchiveView::updateMouseButtons()
{
    m_mouseButtons = QApplication::mouseButtons();
}

void ArchiveView::slotClicked(const QModelIndex& index)
{
    if (style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick)) {
        if (m_mouseButtons != Qt::LeftButton) { // FIXME: see QTBUG-1067
            return;
        }

        // If the user is pressing shift or control, more than one item is being selected
        const Qt::KeyboardModifiers modifier = QApplication::keyboardModifiers();
        if ((modifier & Qt::ShiftModifier) || (modifier & Qt::ControlModifier)) {
            return;
        }

        emit itemTriggered(index);
    }
}

void ArchiveView::slotDoubleClicked(const QModelIndex& index)
{
    if (!style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick)) {
        emit itemTriggered(index);
    }
}

void ArchiveView::setModel(QAbstractItemModel *model)
{
    kDebug();
    QTreeView::setModel(model);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAlternatingRowColors(true);
    setAnimated(true);
    setAllColumnsShowFocus(true);
    setSortingEnabled(true);

    //drag and drop
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DragDrop);
}

void ArchiveView::startDrag(Qt::DropActions supportedActions)
{
    //only start the drag if it's over the filename column. this allows dragging selection in
    //tree/detail view
    if (currentIndex().column() != 0) {
        return;
    }

    kDebug() << "Singling out the current selection...";
    selectionModel()->setCurrentIndex(currentIndex(), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    QTreeView::startDrag(supportedActions);
}


void ArchiveView::dragEnterEvent(QDragEnterEvent * event)
{
    //TODO: if no model, trigger some mechanism to create one automatically!
    kDebug() << event;

    if (event->source() == this) {
        //we don't support internal drops yet.
        return;
    }

    QTreeView::dragEnterEvent(event);
}

void ArchiveView::dropEvent(QDropEvent * event)
{
    kDebug() << event;

    if (event->source() == this) {
        //we don't support internal drops yet.
        return;
    }

    QTreeView::dropEvent(event);
}

void ArchiveView::dragMoveEvent(QDragMoveEvent * event)
{
    if (event->source() == this) {
        //we don't support internal drops yet.
        return;
    }

    QTreeView::dragMoveEvent(event);
    if (event->mimeData()->hasFormat(QLatin1String("text/uri-list"))) {
        event->acceptProposedAction();
    }
}
