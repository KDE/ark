/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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

#include "addtoarchive.h"
#include "archiveentry.h"
#include "archive_kerfuffle.h"
#include "ark_debug.h"
#include "createdialog.h"
#include "jobs.h"

#include <KJobTrackerInterface>
#include <KIO/JobTracker>
#include <KLocalizedString>
#include <KMessageBox>

#include <QFileInfo>
#include <QDir>
#include <QMimeDatabase>
#include <QTimer>
#include <QPointer>

namespace Kerfuffle
{
AddToArchive::AddToArchive(QObject *parent)
        : KJob(parent)
        , m_changeToFirstPath(false)
        , m_enableHeaderEncryption(false)
{
}

AddToArchive::~AddToArchive()
{
}

void AddToArchive::setAutoFilenameSuffix(const QString& suffix)
{
    m_autoFilenameSuffix = suffix;
}

void AddToArchive::setChangeToFirstPath(bool value)
{
    m_changeToFirstPath = value;
}

void AddToArchive::setFilename(const QUrl &path)
{
    m_filename = path.toLocalFile();
}

void AddToArchive::setMimeType(const QString & mimeType)
{
    m_mimeType = mimeType;
}

void AddToArchive::setPassword(const QString &password)
{
    m_password = password;
}

void AddToArchive::setHeaderEncryptionEnabled(bool enabled)
{
    m_enableHeaderEncryption = enabled;
}

bool AddToArchive::showAddDialog()
{
    qCDebug(ARK) << "Opening add dialog";

    if (m_filename.isEmpty()) {
        m_filename = detectBaseName(m_entries);
    }

    QPointer<Kerfuffle::CreateDialog> dialog = new Kerfuffle::CreateDialog(
        nullptr, // parent
        i18n("Compress to Archive"), // caption
        QUrl::fromLocalFile(QFileInfo(m_filename).path())); // startDir

    dialog->setFileName(QFileInfo(m_filename).fileName());

    bool ret = dialog.data()->exec();

    if (ret) {
        qCDebug(ARK) << "CreateDialog returned URL:" << dialog.data()->selectedUrl().toString();
        qCDebug(ARK) << "CreateDialog returned mime:" << dialog.data()->currentMimeType().name();
        setFilename(dialog.data()->selectedUrl());
        setMimeType(dialog.data()->currentMimeType().name());
        setPassword(dialog.data()->password());
        setHeaderEncryptionEnabled(dialog.data()->isHeaderEncryptionEnabled());
        m_options.setCompressionLevel(dialog.data()->compressionLevel());
        m_options.setCompressionMethod(dialog.data()->compressionMethod());
        m_options.setEncryptionMethod(dialog.data()->encryptionMethod());
        m_options.setVolumeSize(dialog.data()->volumeSize());
    }

    delete dialog.data();

    return ret;
}

bool AddToArchive::addInput(const QUrl &url)
{
    Archive::Entry *entry = new Archive::Entry(this);
    entry->setFullPath(url.toLocalFile());
    m_entries << entry;

    if (m_firstPath.isEmpty()) {
        QString firstEntry = url.toLocalFile();
        m_firstPath = QFileInfo(firstEntry).dir().absolutePath();
    }

    return true;
}

void AddToArchive::start()
{
    qCDebug(ARK) << "Starting job";

    QTimer::singleShot(0, this, &AddToArchive::slotStartJob);
}

bool AddToArchive::doKill()
{
    return m_createJob && m_createJob->kill();
}

void AddToArchive::slotStartJob()
{
    if (m_entries.isEmpty()) {
        KMessageBox::error(nullptr, i18n("No input files were given."));
        emitResult();
        return;
    }

    if (m_filename.isEmpty()) {
        if (m_autoFilenameSuffix.isEmpty()) {
            KMessageBox::error(nullptr, xi18n("You need to either supply a filename for the archive or a suffix (such as rar, tar.gz) with the <command>--autofilename</command> argument."));
            emitResult();
            return;
        }

        if (m_firstPath.isEmpty()) {
            qCWarning(ARK) << "Weird, this should not happen. no firstpath defined. aborting";
            emitResult();
            return;
        }

        detectFileName();
    }

    if (m_changeToFirstPath) {
        if (m_firstPath.isEmpty()) {
            qCWarning(ARK) << "Weird, this should not happen. no firstpath defined. aborting";
            emitResult();
            return;
        }

        const QDir stripDir(m_firstPath);

        for (Archive::Entry *entry : qAsConst(m_entries)) {
            entry->setFullPath(stripDir.absoluteFilePath(entry->fullPath()));
        }

        qCDebug(ARK) << "Setting GlobalWorkDir to " << stripDir.path();
        m_options.setGlobalWorkDir(stripDir.path());
    }

    m_createJob = Archive::create(m_filename, m_mimeType, m_entries, m_options, this);

    if (!m_password.isEmpty()) {
        m_createJob->enableEncryption(m_password, m_enableHeaderEncryption);
    }

    KIO::getJobTracker()->registerJob(m_createJob);
    connect(m_createJob, &KJob::result, this, &AddToArchive::slotFinished);
    m_createJob->start();
}

void AddToArchive::detectFileName()
{
    const QString base = detectBaseName(m_entries);
    const QString suffix = !m_autoFilenameSuffix.isEmpty() ? QLatin1Char( '.' ) + m_autoFilenameSuffix : QString();

    QString finalName = base + suffix;

    //if file already exists, append a number to the base until it doesn't
    //exist
    int appendNumber = 0;
    while (QFileInfo::exists(finalName)) {
        ++appendNumber;
        finalName = base + QLatin1Char( '_' ) + QString::number(appendNumber) + suffix;
    }

    qCDebug(ARK) << "Autoset filename to" << finalName;
    m_filename = finalName;
}

void AddToArchive::slotFinished(KJob *job)
{
    qCDebug(ARK) << "job finished";

    if (job->error() && !job->errorString().isEmpty()) {
        KMessageBox::error(nullptr, job->errorString());
    }

    emitResult();
}

QString AddToArchive::detectBaseName(const QVector<Archive::Entry*> &entries) const
{
    QFileInfo fileInfo = QFileInfo(entries.first()->fullPath());
    QDir parentDir = fileInfo.dir();
    QString base = parentDir.absolutePath() + QLatin1Char('/');

    if (entries.size() > 1) {
        if (!parentDir.isRoot()) {
            // Use directory name for the new archive.
            base += parentDir.dirName();
        }
    } else {
        // Strip filename of its extension, but only if present (see #362690).
        if (!QMimeDatabase().mimeTypeForFile(fileInfo.fileName(), QMimeDatabase::MatchExtension).isDefault()) {
            base += fileInfo.completeBaseName();
        } else {
            base += fileInfo.fileName();
        }
    }

    // Special case for compressed tar archives.
    if (base.right(4).toUpper() == QLatin1String(".TAR")) {
        base.chop(4);
    }

    if (base.endsWith(QLatin1Char('/'))) {
        base.chop(1);
    }

    return base;
}

}
