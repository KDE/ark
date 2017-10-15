/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009-2010 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "batchextract.h"
#include "ark_debug.h"
#include "extractiondialog.h"
#include "jobs.h"
#include "queries.h"

#include <KIO/JobTracker>
#include <KLocalizedString>
#include <KMessageBox>
#include <KRun>
#include <KWidgetJobTracker>

#include <QDir>
#include <QFileInfo>
#include <QPointer>
#include <QTimer>

BatchExtract::BatchExtract(QObject* parent)
    : KCompositeJob(parent),
      m_autoSubfolder(false),
      m_preservePaths(true),
      m_openDestinationAfterExtraction(false)
{
    setCapabilities(KJob::Killable);

    connect(this, &KJob::result, this, &BatchExtract::showFailedFiles);
}

BatchExtract::~BatchExtract()
{
}

void BatchExtract::addExtraction(const QUrl& url)
{
    QString destination = destinationFolder();

    auto job = Kerfuffle::Archive::batchExtract(url.toLocalFile(), destination, autoSubfolder(), preservePaths());

    qCDebug(ARK) << QString(QStringLiteral("Registering job from archive %1, to %2, preservePaths %3")).arg(url.toLocalFile(), destination, QString::number(preservePaths()));

    addSubjob(job);

    m_fileNames[job] = qMakePair(url.toLocalFile(), destination);

    connect(job, SIGNAL(percent(KJob*,ulong)),
            this, SLOT(forwardProgress(KJob*,ulong)));
    connect(job, &Kerfuffle::BatchExtractJob::userQuery,
            this, &BatchExtract::slotUserQuery);
}

bool BatchExtract::doKill()
{
    if (subjobs().isEmpty()) {
        return false;
    }

    return subjobs().first()->kill();
}

void BatchExtract::slotUserQuery(Kerfuffle::Query *query)
{
    query->execute();
}

bool BatchExtract::autoSubfolder() const
{
    return m_autoSubfolder;
}

void BatchExtract::setAutoSubfolder(bool value)
{
    m_autoSubfolder = value;
}

void BatchExtract::start()
{
    QTimer::singleShot(0, this, &BatchExtract::slotStartJob);
}

void BatchExtract::slotStartJob()
{
    if (m_inputs.isEmpty()) {
        emitResult();
        return;
    }

    foreach (const auto& url, m_inputs) {
        addExtraction(url);
    }

    KIO::getJobTracker()->registerJob(this);

    emit description(this,
                     i18n("Extracting Files"),
                     qMakePair(i18n("Source archive"), m_fileNames.value(subjobs().at(0)).first),
                     qMakePair(i18n("Destination"), m_fileNames.value(subjobs().at(0)).second)
                    );

    m_initialJobCount = subjobs().size();

    qCDebug(ARK) << "Starting first job";

    subjobs().at(0)->start();
}

void BatchExtract::showFailedFiles()
{
    if (!m_failedFiles.isEmpty()) {
        KMessageBox::informationList(nullptr, i18n("The following files could not be extracted:"), m_failedFiles);
    }
}

void BatchExtract::slotResult(KJob *job)
{
    // TODO: The user must be informed about which file caused the error, and that the other files
    //       in the queue will not be extracted.
    if (job->error()) {
        qCDebug(ARK) << "There was en error:" << job->error() << ", errorText:" << job->errorString();

        setErrorText(job->errorString());
        setError(job->error());

        removeSubjob(job);

        if (job->error() != KJob::KilledJobError) {
            KMessageBox::error(nullptr, job->errorString().isEmpty() ?
                                     i18n("There was an error during extraction.") : job->errorString());
        }

        emitResult();
        return;
    }

    removeSubjob(job);

    if (!hasSubjobs()) {
        if (openDestinationAfterExtraction()) {
            QUrl destination(destinationFolder());
            destination.setPath(QDir::cleanPath(destination.path()));
            KRun::runUrl(destination, QStringLiteral("inode/directory"), nullptr, KRun::RunExecutables, QString(), QByteArray());
        }

        qCDebug(ARK) << "Finished, emitting the result";
        emitResult();
    } else {
        qCDebug(ARK) << "Starting the next job";
        emit description(this,
                         i18n("Extracting Files"),
                         qMakePair(i18n("Source archive"), m_fileNames.value(subjobs().at(0)).first),
                         qMakePair(i18n("Destination"), m_fileNames.value(subjobs().at(0)).second)
                        );
        subjobs().at(0)->start();
    }
}

void BatchExtract::forwardProgress(KJob *job, unsigned long percent)
{
    Q_UNUSED(job)
    auto jobPart = static_cast<ulong>(100 / m_initialJobCount);
    auto remainingJobs = static_cast<ulong>(m_initialJobCount - subjobs().size());
    setPercent(jobPart * remainingJobs + percent / static_cast<ulong>(m_initialJobCount));
}

void BatchExtract::addInput(const QUrl& url)
{
    qCDebug(ARK) << "Adding archive" << url.toLocalFile();

    if (!QFileInfo::exists(url.toLocalFile())) {
        m_failedFiles.append(url.fileName());
        return;
    }

    m_inputs.append(url);
}

bool BatchExtract::openDestinationAfterExtraction() const
{
    return m_openDestinationAfterExtraction;
}

bool BatchExtract::preservePaths() const
{
    return m_preservePaths;
}

QString BatchExtract::destinationFolder() const
{
    if (m_destinationFolder.isEmpty()) {
        return QDir::currentPath();
    } else {
        return m_destinationFolder;
    }
}

void BatchExtract::setDestinationFolder(const QString& folder)
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
    QPointer<Kerfuffle::ExtractionDialog> dialog =
        new Kerfuffle::ExtractionDialog;

    if (m_inputs.size() > 1) {
        dialog.data()->batchModeOption();
    }

    dialog.data()->setModal(true);
    dialog.data()->setAutoSubfolder(autoSubfolder());
    dialog.data()->setCurrentUrl(QUrl::fromUserInput(destinationFolder(), QString(), QUrl::AssumeLocalFile));
    dialog.data()->setPreservePaths(preservePaths());

    // Only one archive, we need a LoadJob to get the single-folder and subfolder properties.
    // TODO: find a better way (e.g. let the dialog handle everything), otherwise we list
    // the archive twice (once here and once in the following BatchExtractJob).
    Kerfuffle::LoadJob *loadJob = nullptr;
    if (m_inputs.size() == 1) {
        loadJob = Kerfuffle::Archive::load(m_inputs.at(0).toLocalFile(), this);
        // We need to access the job after result has been emitted, if the user rejects the dialog.
        loadJob->setAutoDelete(false);

        connect(loadJob, &KJob::result, this, [=](KJob *job) {
            if (job->error()) {
                return;
            }

            auto archive = qobject_cast<Kerfuffle::LoadJob*>(job)->archive();
            dialog->setExtractToSubfolder(archive->hasMultipleTopLevelEntries());
            dialog->setSubfolder(archive->subfolderName());
        });

        connect(loadJob, &KJob::result, dialog.data(), &Kerfuffle::ExtractionDialog::setReadyGui);
        dialog->setBusyGui();
        // NOTE: we exploit the dialog->exec() below to run this job.
        loadJob->start();
    }

    if (!dialog.data()->exec()) {
        if (loadJob) {
            loadJob->kill();
            loadJob->deleteLater();
        }
        delete dialog.data();
        return false;
    }

    setAutoSubfolder(dialog.data()->autoSubfolders());
    setDestinationFolder(dialog.data()->destinationDirectory().toDisplayString(QUrl::PreferLocalFile));
    setOpenDestinationAfterExtraction(dialog.data()->openDestinationAfterExtraction());
    setPreservePaths(dialog.data()->preservePaths());

    delete dialog.data();

    return true;
}

