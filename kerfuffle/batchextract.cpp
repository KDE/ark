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
#include <KIO/RenameDialog>
#include <kwidgetjobtracker.h>

#include <QDir>
#include <QFileInfo>
#include <QPointer>

namespace Kerfuffle
{
BatchExtract::BatchExtract()
        : m_autoSubfolder(false),
        m_preservePaths(true)
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

void BatchExtract::addExtraction(Kerfuffle::Archive* archive, QString destinationFolder)
{
    QString autoDestination = destinationFolder;

    if (autoSubfolder()) {
        if (!archive->isSingleFolderArchive()) {
            QDir destinationDir(destinationFolder);
            QString subfolderName = archive->subfolderName();

            if (destinationDir.exists(subfolderName))
                subfolderName = KIO::RenameDialog::suggestName(destinationFolder, subfolderName);

            kDebug() << "Auto-creating subfolder" << subfolderName << "under" << destinationFolder;
            destinationDir.mkdir(subfolderName);

            autoDestination = destinationFolder + '/' + subfolderName;
        }
    }

    Kerfuffle::ExtractionOptions options;
    options["PreservePaths"] = preservePaths();

    Kerfuffle::ExtractJob *job = archive->copyFiles(
                                     QVariantList(), //extract all files
                                     autoDestination, //extract to folder
                                     options
                                 );

    connect(job, SIGNAL(userQuery(Query*)), this, SLOT(slotUserQuery(Query*)));

    kDebug() << QString("Registering job from archive %1, to %2, preservePaths %3").arg(archive->fileName()).arg(autoDestination).arg(preservePaths());

    addSubjob(job);
    m_fileNames[job] = qMakePair(archive->fileName(), destinationFolder);
    connect(job, SIGNAL(percent(KJob*, unsigned long)),
            this, SLOT(forwardProgress(KJob *, unsigned long)));
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
    // If none of the archives could be loaded, there is no subjob to run
    if (m_inputs.isEmpty()) {
        emitResult();
        return;
    }

    if (!subfolder().isEmpty()) {
        kDebug() << "Creating subfolder" << subfolder();
        QDir dest(destinationFolder());
        dest.mkpath(subfolder());
        m_destinationFolder += '/' + subfolder();
    }

    foreach(Kerfuffle::Archive *archive, m_inputs) {
        QString finalDestination;
        if (destinationFolder().isEmpty()) {
            finalDestination = QDir::currentPath();
        } else {
            finalDestination = destinationFolder();
        }

        addExtraction(archive, finalDestination);
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
    Kerfuffle::Archive *archive = Kerfuffle::factory(url.path());

    if ((archive == NULL) || (!QFileInfo(url.path()).exists())) {
        m_failedFiles.append(url.fileName());
        return false;
    }

    m_inputs.append(archive);

    return true;
}

bool BatchExtract::preservePaths()
{
    return m_preservePaths;
}

QString BatchExtract::destinationFolder()
{
    return m_destinationFolder;
}

void BatchExtract::setDestinationFolder(QString folder)
{
    if (QFileInfo(folder).isDir()) {
        m_destinationFolder = folder;
    }
}

void BatchExtract::setPreservePaths(bool value)
{
    m_preservePaths = value;
}

QString BatchExtract::subfolder()
{
    return m_subfolder;
}

void BatchExtract::setSubfolder(QString subfolder)
{
    m_subfolder = subfolder;
}

bool BatchExtract::showExtractDialog()
{
    QPointer<Kerfuffle::ExtractionDialog> dialog = new Kerfuffle::ExtractionDialog(NULL);
    if (m_inputs.size() > 1) {
        dialog->batchModeOption();
    }

    if (destinationFolder().isEmpty()) {
        dialog->setCurrentUrl(QDir::currentPath());
    } else {
        dialog->setCurrentUrl(destinationFolder());
    }

    dialog->setAutoSubfolder(autoSubfolder());
    dialog->setPreservePaths(preservePaths());

    if (subfolder().isEmpty() && m_inputs.size() == 1) {
        if (m_inputs.at(0)->isSingleFolderArchive()) {
            dialog->setSingleFolderArchive(true);
        }
        dialog->setSubfolder(m_inputs.at(0)->subfolderName());
    } else {
        dialog->setSubfolder(subfolder());
    }

    if (!dialog->exec()) {
        return false;
    }

    setDestinationFolder(dialog->destinationDirectory().path());

    if (dialog->extractToSubfolder()) {
        setSubfolder(dialog->subfolder());
    }

    setAutoSubfolder(dialog->autoSubfolders());
    setPreservePaths(dialog->preservePaths());

    return true;
}
}

#include <batchextract.moc>
