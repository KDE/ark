/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv (at@at) stud.ntnu.no>
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

#ifndef ARCHIVEVIEW_H
#define ARCHIVEVIEW_H

#include <QTreeView>

class ArchiveView : public QTreeView
{
    Q_OBJECT

public:
    ArchiveView(QWidget *parent = 0);
    virtual void dragEnterEvent(class QDragEnterEvent * event);
    virtual void dropEvent(class QDropEvent * event);
    virtual void dragMoveEvent(class QDragMoveEvent * event);
    virtual void startDrag(Qt::DropActions supportedActions);

    void setModel(QAbstractItemModel *model);

protected slots:
    void slotClicked(const QModelIndex & index);
    void slotDoubleClicked(const QModelIndex & index);

private slots:
    void updateMouseButtons();

signals:
    void itemTriggered(const QModelIndex & index);

private:
    Qt::MouseButtons m_mouseButtons; // FIXME: workaround until QTBUG-1067 is resolved
};

#endif /* ARCHIVEVIEW_H */
