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

class QLineEdit;

class ArchiveView : public QTreeView
{
    Q_OBJECT

public:
    explicit ArchiveView(QWidget *parent = nullptr);
    void dragEnterEvent(class QDragEnterEvent * event) override;
    void dropEvent(class QDropEvent * event) override;
    void dragMoveEvent(class QDragMoveEvent * event) override;
    void startDrag(Qt::DropActions supportedActions) override;

    /**
     * Expand the first level in the view if there is only one root folder.
     * Typical use case: an archive with source code.
     */
    void expandIfSingleFolder();

    /**
     * Set whether the view should accept drop events.
     */
    void setDropsEnabled(bool enabled);

public Q_SLOTS:
    void renameSelectedEntry();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

Q_SIGNALS:
    void entryChanged(const QString &name);

private:
    void openEntryEditor(const QModelIndex &index);
    void closeEntryEditor();
    QModelIndex m_editorIndex;
    QLineEdit *m_entryEditor = nullptr;
};

#endif /* ARCHIVEVIEW_H */
