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

#include "batchextract.h"

#include "archive.h"
#include "extractiondialog.h"
#include "jobs.h"
#include "queries.h"

#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QApplication>
#include <QDesktopWidget>
#include <QPointer>

#include <KDebug>
#include <KGlobal>
#include <KLocale>
#include <KMessageBox>
#include <KRun>
#include <KIO/RenameDialog>
#include <kwidgetjobtracker.h>

namespace Kerfuffle
{
BatchExtract::BatchExtract()
        : m_autoSubfolders(false),
        m_preservePaths(true),
        m_openDestinationAfterExtraction(false)
{
    setCapabilities(KJob::Killable);
}

BatchExtract::~BatchExtract()
{
    if (!m_inputs.isEmpty()) {
        KIO::getJobTracker()->unregisterJob(this);
    }
}

void BatchExtract::addExtraction(Kerfuffle::Archive* archive, bool preservePaths, QString destinationFolder)
{
    kDebug(1601);

    QString autoDestination = destinationFolder;

    if (m_autoSubfolders) {
        if (!archive->isSingleFolderArchive()) {
            QDir destinationDir(destinationFolder);
            QString subfolderName = archive->subfolderName();

            if (destinationDir.exists(subfolderName))
                subfolderName = KIO::RenameDialog::suggestName(destinationFolder, subfolderName);

            kDebug(1601) << "Auto-creating subfolder" << subfolderName << "under" << destinationFolder;
            destinationDir.mkdir(subfolderName);

            autoDestination = destinationFolder + '/' + subfolderName;
        }
    }

    Kerfuffle::ExtractionOptions options;
    options["PreservePaths"] = preservePaths;

    Kerfuffle::ExtractJob *job = archive->copyFiles(
                                     QVariantList(), //extract all files
                                     autoDestination, //extract to folder
                                     options
                                 );

    connect(job, SIGNAL(userQuery(Query*)), this, SLOT(slotUserQuery(Query*)));

    kDebug(1601) << QString("Registering job from archive %1, to %2, preservePaths %3").arg(archive->fileName()).arg(autoDestination).arg(preservePaths);

    addSubjob(job);
    m_fileNames[job] = qMakePair(archive->fileName(), destinationFolder);
    connect(job, SIGNAL(percent(KJob*, unsigned long)),
            this, SLOT(forwardProgress(KJob *, unsigned long)));
}

void BatchExtract::slotUserQuery(Query *query)
{
    query->execute();
}

void BatchExtract::setAutoSubfolder(bool value)
{
    m_autoSubfolders = value;
}

void BatchExtract::start()
{
    // If none of the archives could be loaded, there is no subjob to run
    if (m_inputs.isEmpty()) {
        emitResult();
        return;
    }

    foreach(Kerfuffle::Archive *archive, m_inputs) {
        addExtraction(archive, m_preservePaths, destinationFolder());
    }

    KIO::getJobTracker()->registerJob(this);

    emit description(this,
                     i18n("Extracting file..."),
                     qMakePair(i18n("Source archive"), m_fileNames.value(subjobs().at(0)).first),
                     qMakePair(i18n("Destination"), m_fileNames.value(subjobs().at(0)).second)
                    );

    m_initialJobCount = subjobs().size();

    kDebug(1601) << "Starting first job";

    subjobs().at(0)->start();
}

void BatchExtract::slotResult(KJob *job)
{
    kDebug(1601);
    if (job->error()) {
        kDebug(1601) << "There was en error, " << job->errorText();

        setErrorText(job->errorText());
        setError(job->error());

        removeSubjob(job);

        KMessageBox::error(NULL, job->errorText().isEmpty() ?
                           i18n("There was an error during extraction.") : job->errorText()
                          );

        emitResult();

        return;
    } else {
        removeSubjob(job);
    }

    if (!subjobs().size()) {
        if (openDestinationAfterExtraction()) {
            KUrl destination(destinationFolder());
            destination.cleanPath();
            KRun::runUrl(destination, "inode/directory", 0);
        }

        kDebug(1601) << "Finished, emitting the result";
        emitResult();
    } else {
        kDebug(1601) << "Starting the next job";
        emit description(this,
                         i18n("Extracting file..."),
                         qMakePair(i18n("Source archive"), m_fileNames.value(subjobs().at(0)).first),
                         qMakePair(i18n("Destination"), m_fileNames.value(subjobs().at(0)).second)
                        );
        subjobs().at(0)->start();
    }
}

void BatchExtract::forwardProgress(KJob *job, unsigned long percent)
{
    Q_UNUSED(job);
    int jobPart = 100 / m_initialJobCount;
    setPercent(jobPart *(m_initialJobCount - subjobs().size()) + percent / m_initialJobCount);
}

bool BatchExtract::addInput(const KUrl& url)
{
    kDebug(1601);

    Kerfuffle::Archive *archive = Kerfuffle::factory(url.pathOrUrl());

    if ((archive == NULL) || (!QFileInfo(url.pathOrUrl()).exists())) {
        return false;
    }

    m_inputs << archive;
    return true;
}

bool BatchExtract::openDestinationAfterExtraction()
{
    return m_openDestinationAfterExtraction;
}

QString BatchExtract::destinationFolder()
{
    if (m_destinationFolder.isEmpty()) {
        return QDir::currentPath();
    } else {
        return m_destinationFolder;
    }
}

void BatchExtract::setDestinationFolder(QString folder)
{
    if (QFileInfo(folder).isDir()) {
        m_destinationFolder = folder;
    }
}

void BatchExtract::setOpenDestinationAfterExtraction(bool value)
{
    m_openDestinationAfterExtraction = value;
}

void BatchExtract::setPreservePaths(bool value)
{
    m_preservePaths = value;
}

bool BatchExtract::showExtractDialog()
{
    kDebug(1601);

    QPointer<Kerfuffle::ExtractionDialog> dialog = new Kerfuffle::ExtractionDialog(NULL);
    if (m_inputs.size() > 1) {
        dialog->batchModeOption();
    }

    dialog->setAutoSubfolder(m_autoSubfolders);
    dialog->setCurrentUrl(destinationFolder());
    dialog->setPreservePaths(m_preservePaths);

    if (m_inputs.size() == 1) {
        if (m_inputs.at(0)->isSingleFolderArchive()) {
            dialog->setSingleFolderArchive(true);
        }
        dialog->setSubfolder(m_inputs.at(0)->subfolderName());
    }

    if (!dialog->exec()) {
        delete dialog;
        return false;
    }

    setAutoSubfolder(dialog->autoSubfolders());
    setDestinationFolder(dialog->destinationDirectory().pathOrUrl());
    setOpenDestinationAfterExtraction(dialog->openDestinationAfterExtraction());
    setPreservePaths(dialog->preservePaths());

    delete dialog;

    return true;
}
}

#include <batchextract.moc>
