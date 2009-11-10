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
#include <QApplication>
#include <QHeaderView>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QMouseEvent>
#include <KDebug>
#include <KGlobalSettings>

ArchiveView::ArchiveView(QWidget *parent)
        : QTreeView(parent)
        , m_mouseButtons(Qt::NoButton)
{
    connect(this, SIGNAL(pressed(const QModelIndex&)),
            SLOT(updateMouseButtons()));
    connect(this, SIGNAL(clicked(const QModelIndex&)),
            SLOT(slotClicked(const QModelIndex&)));
    connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
            SLOT(slotDoubleClicked(const QModelIndex&)));
}

// FIXME: this is a workaround taken from Dolphin until Qt-issue 176832 is resolved
void ArchiveView::updateMouseButtons()
{
    m_mouseButtons = QApplication::mouseButtons();
}

void ArchiveView::slotClicked(const QModelIndex& index)
{
    if (KGlobalSettings::singleClick()) {
        if (m_mouseButtons != Qt::LeftButton) // FIXME: see Qt-issue 176832
            return;

        // If the user is pressing shift or control, more than one item is being selected
        const Qt::KeyboardModifiers modifier = QApplication::keyboardModifiers();
        if ((modifier & Qt::ShiftModifier) || (modifier & Qt::ControlModifier))
            return;

        emit itemTriggered(index);
    }
}

void ArchiveView::slotDoubleClicked(const QModelIndex& index)
{
    if (!KGlobalSettings::singleClick())
        emit itemTriggered(index);
}

void ArchiveView::setModel(QAbstractItemModel *model)
{
    kDebug(1601) ;
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
    kDebug(1601) << "Singling out the current selection...";
    //selectionModel()->setCurrentIndex(currentIndex(), QItemSelectionModel::Clear);
    selectionModel()->setCurrentIndex(currentIndex(), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    QTreeView::startDrag(supportedActions);
}


void ArchiveView::dragEnterEvent(QDragEnterEvent * event)
{
    //TODO: if no model, trigger some mechanism to create one automatically!
    kDebug(1601) << event;

    if (event->source() == this) {
        //we don't support internal drops yet.
        return;
    }

    QTreeView::dragEnterEvent(event);
}

void ArchiveView::dropEvent(QDropEvent * event)
{
    kDebug(1601) << event;

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
    if (event->mimeData()->hasFormat("text/uri-list"))
        event->acceptProposedAction();
}
