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

#include "kerfuffle/archive.h"
#include "kerfuffle/extractiondialog.h"
#include "kerfuffle/jobs.h"
#include "kerfuffle/queries.h"

#include <KDebug>
#include <KGlobal>
#include <KLocale>
#include <KMessageBox>
#include <KRun>
#include <KIO/RenameDialog>
#include <kwidgetjobtracker.h>

#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QWeakPointer>

BatchExtract::BatchExtract()
    : KCompositeJob(0),
      m_autoSubfolder(false),
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
        const QDir d(destination);
        QString subfolderName = archive->subfolderName();

        if (d.exists(subfolderName)) {
            subfolderName = KIO::RenameDialog::suggestName(destination, subfolderName);
        }

        d.mkdir(subfolderName);

        destination += QLatin1Char( '/' ) + subfolderName;
    }

    Kerfuffle::ExtractionOptions options;
    options[QLatin1String( "PreservePaths" )] = preservePaths();

    Kerfuffle::ExtractJob *job = archive->copyFiles(QVariantList(), destination, options);

    kDebug() << QString(QLatin1String( "Registering job from archive %1, to %2, preservePaths %3" )).arg(archive->fileName()).arg(destination).arg(preservePaths());

    addSubjob(job);

    m_fileNames[job] = qMakePair(archive->fileName(), destination);

    connect(job, SIGNAL(percent(KJob*,ulong)),
            this, SLOT(forwardProgress(KJob*,ulong)));
    connect(job, SIGNAL(userQuery(Kerfuffle::Query*)),
            this, SLOT(slotUserQuery(Kerfuffle::Query*)));
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

    // TODO: The user must be informed about which file caused the error, and that the other files
    //       in the queue will not be extracted.
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
            KRun::runUrl(destination, QLatin1String( "inode/directory" ), 0);
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
    Q_UNUSED(job)
    int jobPart = 100 / m_initialJobCount;
    setPercent(jobPart *(m_initialJobCount - subjobs().size()) + percent / m_initialJobCount);
}

bool BatchExtract::addInput(const KUrl& url)
{
    Kerfuffle::Archive *archive = Kerfuffle::Archive::create(url.pathOrUrl(), this);

    if ((archive == NULL) || (!QFileInfo(url.pathOrUrl()).exists())) {
        m_failedFiles.append(url.fileName());
        return false;
    }

    m_inputs.append(archive);

    return true;
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
    QWeakPointer<Kerfuffle::ExtractionDialog> dialog =
        new Kerfuffle::ExtractionDialog;

    if (m_inputs.size() > 1) {
        dialog.data()->batchModeOption();
    }

    dialog.data()->setAutoSubfolder(autoSubfolder());
    dialog.data()->setCurrentUrl(destinationFolder());
    dialog.data()->setPreservePaths(preservePaths());

    if (m_inputs.size() == 1) {
        if (m_inputs.at(0)->isSingleFolderArchive()) {
            dialog.data()->setSingleFolderArchive(true);
        }
        dialog.data()->setSubfolder(m_inputs.at(0)->subfolderName());
    }

    if (!dialog.data()->exec()) {
        delete dialog.data();
        return false;
    }

    setAutoSubfolder(dialog.data()->autoSubfolders());
    setDestinationFolder(dialog.data()->destinationDirectory().pathOrUrl());
    setOpenDestinationAfterExtraction(dialog.data()->openDestinationAfterExtraction());
    setPreservePaths(dialog.data()->preservePaths());

    delete dialog.data();

    return true;
}

#include <batchextract.moc>
