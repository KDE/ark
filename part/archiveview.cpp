/*
    SPDX-FileCopyrightText: 2008-2009 Harald Hvaal <haraldhv (at@at) stud.ntnu.no>
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archiveview.h"
#include "ark_debug.h"

#include <QHeaderView>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QMouseEvent>
#include <QLineEdit>

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
    setFrameShape(QFrame::NoFrame);
    // #368807: drops must be initially disabled, otherwise they will override the MainWindow's ones.
    // They will be enabled in Part::slotLoadingFinished().
    setDropsEnabled(false);
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
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

void ArchiveView::expandIfSingleFolder()
{
    if (model()->rowCount() == 1) {
        expandToDepth(0);
    }
}

void ArchiveView::setDropsEnabled(bool enabled)
{
    setAcceptDrops(enabled);
    setDragDropMode(enabled ? QAbstractItemView::DragDrop : QAbstractItemView::DragOnly);
}

void ArchiveView::dragEnterEvent(QDragEnterEvent * event)
{
    //TODO: if no model, trigger some mechanism to create one automatically!
    qCDebug(ARK) << event;

    if (event->source() == this) {
        //we don't support internal drops yet.
        return;
    }

    QTreeView::dragEnterEvent(event);
}

void ArchiveView::dropEvent(QDropEvent * event)
{
    qCDebug(ARK) << event;

    if (event->source() == this) {
        //we don't support internal drops yet.
        return;
    }

    QTreeView::dropEvent(event);
}

void ArchiveView::dragMoveEvent(QDragMoveEvent * event)
{
    qCDebug(ARK) << event;

    if (event->source() == this) {
        //we don't support internal drops yet.
        return;
    }

    QTreeView::dragMoveEvent(event);
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

bool ArchiveView::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_entryEditor && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            closeEntryEditor();
            return true;
        }
    }
    return false;
}

void ArchiveView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_editorIndex.isValid()) {
        closeEntryEditor();
    } else {
        QTreeView::mouseReleaseEvent(event);
    }
}

void ArchiveView::keyPressEvent(QKeyEvent *event)
{
    if (m_editorIndex.isValid()) {
        switch (event->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter: {
            QLineEdit* editor = static_cast<QLineEdit*>(indexWidget(m_editorIndex));
            Q_EMIT entryChanged(editor->text());
            closeEntryEditor();
            break;
        }
        default:
            QTreeView::keyPressEvent(event);
        }
    } else {
        QTreeView::keyPressEvent(event);
    }
}

void ArchiveView::renameSelectedEntry()
{
    QModelIndex currentIndex = selectionModel()->currentIndex();
    currentIndex = (currentIndex.parent().isValid())
                   ? currentIndex.parent().model()->index(currentIndex.row(), 0, currentIndex.parent())
                   : model()->index(currentIndex.row(), 0);
    openEntryEditor(currentIndex);
}

void ArchiveView::openEntryEditor(const QModelIndex &index)
{
    m_editorIndex = index;
    openPersistentEditor(index);
    m_entryEditor = static_cast<QLineEdit*>(indexWidget(m_editorIndex));
    m_entryEditor->installEventFilter(this);
    m_entryEditor->setText(index.data().toString());
    m_entryEditor->setFocus(Qt::OtherFocusReason);
    m_entryEditor->selectAll();
}

void ArchiveView::closeEntryEditor()
{
    m_entryEditor->removeEventFilter(this);
    closePersistentEditor(m_editorIndex);
    m_editorIndex = QModelIndex();
}

void NoHighlightSelectionDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() && option.state.testFlag(QStyle::State_Selected)) {
        QStyleOptionViewItem myOption = option;
        myOption.state |= QStyle::State_MouseOver;
        myOption.state &= ~QStyle::State_Selected;

        QStyledItemDelegate::paint(painter, myOption, index);
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

#include "moc_archiveview.cpp"
