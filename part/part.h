/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (C) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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
#ifndef PART_H
#define PART_H

#include "interface.h"

#include <KParts/Part>
#include <KParts/StatusBarExtension>
#include <KTempDir>

#include <QModelIndex>

class ArchiveModel;
class InfoPanel;

class KAbstractWidgetJobTracker;
class KAboutData;
class KAction;
class KJob;

class QAction;
class QSplitter;
class QTreeView;

namespace Ark
{

class Part: public KParts::ReadWritePart, public Interface
{
    Q_OBJECT
    Q_INTERFACES(Interface)
public:
    Part(QWidget *parentWidget, QObject *parent, const QVariantList &);
    ~Part();
    static KAboutData* createAboutData();

    virtual bool openFile();
    virtual bool saveFile();

    bool isBusy() const;

public slots:
    void extractSelectedFilesTo(const QString& localPath);

private slots:
    void slotLoadingStarted();
    void slotLoadingFinished(KJob *job);
    void slotPreview();
    void slotPreview(const QModelIndex & index);
    void slotPreviewExtracted(KJob*);
    void slotError(const QString& errorMessage, const QString& details);
    void slotExtractFiles();
    void slotExtractionDone(KJob*);
    void slotQuickExtractFiles(QAction*);
    void slotAddFiles();
    void slotAddFiles(const QStringList& files, const QString& path = QString());
    void slotAddDir();
    void slotAddFilesDone(KJob*);
    void slotDeleteFiles();
    void slotDeleteFilesDone(KJob*);
    void saveSplitterSizes();
    void slotToggleInfoPanel(bool);
    void slotSaveAs();
    void updateActions();
    void selectionChanged();
    void adjustColumns();
    void setBusyGui();
    void setReadyGui();
    void setFileNameFromArchive();

signals:
    void busy();
    void ready();
    void quit();

private:
    void setupView();
    void setupActions();
    bool isSingleFolderArchive() const;
    QString detectSubfolder() const;
    bool isPreviewable(const QModelIndex& index) const;
    QList<QVariant> selectedFiles();
    QList<QVariant> selectedFilesWithChildren();
    void registerJob(KJob *job);

    ArchiveModel         *m_model;
    QTreeView            *m_view;
    KAction              *m_previewAction;
    KAction              *m_extractFilesAction;
    KAction              *m_addFilesAction;
    KAction              *m_addDirAction;
    KAction              *m_deleteFilesAction;
    KAction              *m_saveAsAction;
    InfoPanel            *m_infoPanel;
    QSplitter            *m_splitter;
    KTempDir              m_previewDir;
    bool                  m_busy;

    KAbstractWidgetJobTracker  *m_jobTracker;
    KParts::StatusBarExtension *m_statusBarExtension;
};

} // namespace Ark

#endif // PART_H
