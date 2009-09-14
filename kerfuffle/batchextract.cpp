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

#include <KDebug>
#include <KGlobal>
#include <KLocale>
#include <KMessageBox>
#include <KRun>
#include <KIO/RenameDialog>
#include <kwidgetjobtracker.h>

#include <QDir>
#include <QFileInfo>
#include <QPointer>
#include <QTimer>

namespace Kerfuffle
{
BatchExtract::BatchExtract()
        : m_autoSubfolder(false),
        m_preservePaths(true),
        m_openDestinationAfterExtraction(false)
{
    setCapabilities(KJob::Killable);

    connect(this, SIGNAL(result(KJob*)), SLOT(showFailedFiles()));
}

BatchExtract::~BatchExtract()
{
    if (!m_inputs.isEmpty()) {
        KIO::getJobTracker()->unregisterJob(this);
    }
}

void BatchExtract::addExtraction(Kerfuffle::Archive* archive)
{
    QString destination = destinationFolder();

    if ((autoSubfolder()) && (!archive->isSingleFolderArchive())) {
        QDir d(destination);
        QString subfolderName = archive->subfolderName();

        if (d.exists(subfolderName))
            subfolderName = KIO::RenameDialog::suggestName(destination, subfolderName);

        d.mkdir(subfolderName);

        destination += '/' + subfolderName;
    }

    Kerfuffle::ExtractionOptions options;
    options["PreservePaths"] = preservePaths();

    Kerfuffle::ExtractJob *job = archive->copyFiles(QVariantList(), destination, options);

    kDebug() << QString("Registering job from archive %1, to %2, preservePaths %3").arg(archive->fileName()).arg(destination).arg(preservePaths());

    addSubjob(job);

    m_fileNames[job] = qMakePair(archive->fileName(), destination);

    connect(job, SIGNAL(percent(KJob*, unsigned long)),
            this, SLOT(forwardProgress(KJob *, unsigned long)));
    connect(job, SIGNAL(userQuery(Query*)), this, SLOT(slotUserQuery(Query*)));
}

void BatchExtract::slotUserQuery(Query *query)
{
    query->execute();
}

bool BatchExtract::autoSubfolder()
{
    return m_autoSubfolder;
}

void BatchExtract::setAutoSubfolder(bool value)
{
    m_autoSubfolder = value;
}

void BatchExtract::start()
{
    QTimer::singleShot(0, this, SLOT(slotStartJob()));
}

void BatchExtract::slotStartJob()
{
    // If none of the archives could be loaded, there is no subjob to run
    if (m_inputs.isEmpty()) {
        emitResult();
        return;
    }

    foreach(Kerfuffle::Archive *archive, m_inputs) {
        addExtraction(archive);
    }

    KIO::getJobTracker()->registerJob(this);

    emit description(this,
                     i18n("Extracting file..."),
                     qMakePair(i18n("Source archive"), m_fileNames.value(subjobs().at(0)).first),
                     qMakePair(i18n("Destination"), m_fileNames.value(subjobs().at(0)).second)
                    );

    m_initialJobCount = subjobs().size();

    kDebug() << "Starting first job";

    subjobs().at(0)->start();
}

void BatchExtract::showFailedFiles()
{
    if (!m_failedFiles.isEmpty()) {
        KMessageBox::informationList(0, i18n("The following files could not be extracted:"), m_failedFiles);
    }
}

void BatchExtract::slotResult(KJob *job)
{
    kDebug();
    if (job->error()) {
        kDebug() << "There was en error, " << job->errorText();

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

    if (!hasSubjobs()) {
        if (openDestinationAfterExtraction()) {
            KUrl destination(destinationFolder());
            destination.cleanPath();
            KRun::runUrl(destination, "inode/directory", 0);
        }

        kDebug() << "Finished, emitting the result";
        emitResult();
    } else {
        kDebug() << "Starting the next job";
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
    Kerfuffle::Archive *archive = Kerfuffle::factory(url.pathOrUrl());

    if ((archive == NULL) || (!QFileInfo(url.pathOrUrl()).exists())) {
        m_failedFiles.append(url.fileName());
        return false;
    }

    m_inputs.append(archive);

    return true;
}

bool BatchExtract::openDestinationAfterExtraction()
{
    return m_openDestinationAfterExtraction;
}

bool BatchExtract::preservePaths()
{
    return m_preservePaths;
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
    QPointer<Kerfuffle::ExtractionDialog> dialog = new Kerfuffle::ExtractionDialog(NULL);

    if (m_inputs.size() > 1) {
        dialog->batchModeOption();
    }

    dialog->setAutoSubfolder(autoSubfolder());
    dialog->setCurrentUrl(destinationFolder());
    dialog->setPreservePaths(preservePaths());

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
