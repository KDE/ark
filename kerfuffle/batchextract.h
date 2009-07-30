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

#ifndef BATCHEXTRACT_H
#define BATCHEXTRACT_H

#include "kerfuffle_export.h"

#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QString>

#include <kcompositejob.h>

class Interface;
class KJobTrackerInterface;
class KUrl;

namespace Kerfuffle
{
class Archive;
class Query;

class KERFUFFLE_EXPORT BatchExtract : public KCompositeJob
{
    Q_OBJECT

public:
    BatchExtract();
    virtual ~BatchExtract();
    void addExtraction(Archive* archive, bool preservePaths = true, QString destinationFolder = QString());
    void start();
    void setAutoSubfolder(bool value);
    bool addInput(const KUrl& url);
    QString destinationFolder();
    bool openDestinationAfterExtraction();
    bool showExtractDialog();
    void setDestinationFolder(QString folder);
    void setOpenDestinationAfterExtraction(bool value);
    void setPreservePaths(bool value);

private slots:
    void forwardProgress(KJob *job, unsigned long percent);
    void slotResult(KJob *job);
    void slotUserQuery(Query *query);

private:
    int m_initialJobCount;
    QMap<KJob*, QPair<QString, QString> > m_fileNames;
    bool m_autoSubfolders;

    QList<Archive*> m_inputs;
    //KJobTrackerInterface *m_tracker;
    QString m_destinationFolder;
    bool m_preservePaths;
    bool m_openDestinationAfterExtraction;
};
}

#endif // BATCHEXTRACT_H
