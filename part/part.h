/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (C) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
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

#include <KParts/Part>
#include <KParts/StatusBarExtension>
#include <QModelIndex>
#include "interface.h"

class ArchiveModel;
class InfoPanel;

class QTreeView;
class QAction;
class KAction;

class KAboutData;
class KTempDir;
class KJob;
class KAbstractWidgetJobTracker;

namespace Ark
{

class Part: public KParts::ReadWritePart, public Interface
{
    Q_OBJECT
    Q_INTERFACES(Interface)
public:
    Part(QWidget *parentWidget, QObject *parent, const QStringList &);
    ~Part();
    static KAboutData* createAboutData();

    virtual bool openFile();
    virtual bool saveFile();

    QStringList supportedMimeTypes() const;
    QStringList supportedWriteMimeTypes() const;

    bool isBusy() const {
        return m_busy;
    }

public slots:
    void extractSelectedFilesTo(QString localPath);

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
    void updateActions();
    void selectionChanged();
    void adjustColumns();
    void setBusyGui();
    void setReadyGui();
    void setFileNameFromArchive();

signals:
    void busy();
    void ready();

private:
    void setupView();
    void setupActions();
    bool isSingleFolderArchive();
    QString detectSubfolder();
    bool isPreviewable(const QModelIndex & index);
    QList<QVariant> selectedFiles();
    QList<QVariant> selectedFilesWithChildren();
    void registerJob(KJob *job);

    ArchiveModel         *m_model;
    QTreeView            *m_view;
    QAction              *m_previewAction;
    KAction              *m_extractFilesAction;
    QAction              *m_addFilesAction;
    QAction              *m_addDirAction;
    QAction              *m_deleteFilesAction;
    InfoPanel            *m_infoPanel;
    KTempDir             *m_previewDir;
    bool                  m_busy;
    QString               m_destinationDirectory;

    KAbstractWidgetJobTracker  *m_jobTracker;
    KParts::StatusBarExtension *m_statusBarExtension;
};

} // namespace Ark

#endif // PART_H
