/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv (at@at) stud.ntnu.no>
 * Copyright (c) 2016 Vladyslav Batyrenko <mvlabat@gmail.com>
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
#include <QtWidgets/QLineEdit>

class ArchiveView : public QTreeView
{
    Q_OBJECT

public:
    explicit ArchiveView(QWidget *parent = 0);
    virtual void dragEnterEvent(class QDragEnterEvent * event) Q_DECL_OVERRIDE;
    virtual void dropEvent(class QDropEvent * event) Q_DECL_OVERRIDE;
    virtual void dragMoveEvent(class QDragMoveEvent * event) Q_DECL_OVERRIDE;
    virtual void startDrag(Qt::DropActions supportedActions) Q_DECL_OVERRIDE;

    /**
     * Set whether the view should accept drop events.
     */
    void setDropsEnabled(bool enabled);

    void openEntryEditor(const QModelIndex &index);

protected:
    virtual bool eventFilter(QObject *object, QEvent *event) Q_DECL_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;

signals:
    void entryChanged(const QString &name);

private:
    void closeEntryEditor();
    QModelIndex m_editorIndex;
    QLineEdit *m_entryEditor = Q_NULLPTR;
};

#endif /* ARCHIVEVIEW_H */
