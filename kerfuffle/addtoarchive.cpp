/*
    SPDX-FileCopyrightText: 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2009 Raphael Kubo da Costa <rakuco@FreeBSD.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "addtoarchive.h"
#include "archiveentry.h"
#include "ark_debug.h"
#include "createdialog.h"
#include "jobs.h"

#include <KIO/JobTracker>
#include <KJobTrackerInterface>
#include <KLocalizedString>
#include <KMessageBox>

#include <KFileUtils>
#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QPointer>
#include <QTimer>

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

void AddToArchive::setAutoFilenameSuffix(const QString &suffix)
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

void AddToArchive::setMimeType(const QString &mimeType)
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

bool AddToArchive::showAddDialog(QWidget *parentWidget)
{
    qCDebug(ARK_LOG) << "Opening add dialog";

    if (m_filename.isEmpty()) {
        m_filename = getFileNameForEntries(m_entries, QString());
    }

    QPointer<Kerfuffle::CreateDialog> dialog = new Kerfuffle::CreateDialog(parentWidget, // parent
                                                                           i18n("Compress to Archive"), // caption
                                                                           QUrl::fromLocalFile(QFileInfo(m_filename).path())); // startDir

    dialog->setFileName(QFileInfo(m_filename).fileName());

    bool ret = dialog.data()->exec();

    if (ret) {
        qCDebug(ARK_LOG) << "CreateDialog returned URL:" << dialog.data()->selectedUrl().toString();
        qCDebug(ARK_LOG) << "CreateDialog returned mime:" << dialog.data()->currentMimeType().name();
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
    qCDebug(ARK_LOG) << "Starting job";

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
            KMessageBox::error(nullptr,
                               xi18n("You need to either supply a filename for the archive or a suffix (such as rar, tar.gz) with the "
                                     "<command>--autofilename</command> argument."));
            emitResult();
            return;
        }

        if (m_firstPath.isEmpty()) {
            qCWarning(ARK_LOG) << "Weird, this should not happen. no firstpath defined. aborting";
            emitResult();
            return;
        }

        detectFileName();
    }

    if (m_changeToFirstPath) {
        if (m_firstPath.isEmpty()) {
            qCWarning(ARK_LOG) << "Weird, this should not happen. no firstpath defined. aborting";
            emitResult();
            return;
        }

        const QDir stripDir(m_firstPath);

        for (Archive::Entry *entry : std::as_const(m_entries)) {
            entry->setFullPath(stripDir.absoluteFilePath(entry->fullPath()));
        }

        qCDebug(ARK_LOG) << "Setting GlobalWorkDir to " << stripDir.path();
        m_options.setGlobalWorkDir(stripDir.path());
    }

    m_createJob = Archive::create(m_filename, m_mimeType, m_entries, m_options, this);

    m_createJob->setProperty("immediateProgressReporting", m_immediateProgressReporting);
    m_createJob->setProperty("destUrl", QUrl::fromLocalFile(m_filename));

    if (!m_password.isEmpty()) {
        m_createJob->enableEncryption(m_password, m_enableHeaderEncryption);
    }

    KIO::getJobTracker()->registerJob(m_createJob);
    connect(m_createJob, &KJob::result, this, &AddToArchive::slotFinished);
    m_createJob->start();
}

void AddToArchive::detectFileName()
{
    const QString suffix = !m_autoFilenameSuffix.isEmpty() ? m_autoFilenameSuffix : QString();
    const QString finalName = getFileNameForEntries(m_entries, suffix);

    qCDebug(ARK_LOG) << "Autoset filename to" << finalName;
    m_filename = finalName;
}

void AddToArchive::slotFinished(KJob *job)
{
    qCDebug(ARK_LOG) << "job finished";

    if (job->error() && !job->errorString().isEmpty()) {
        KMessageBox::error(nullptr, job->errorString());
    }

    emitResult();
}

QString findCommonPrefixForUrls(const QList<QUrl> &list)
{
    Q_ASSERT(!list.isEmpty());
    QString prefix = list.front().fileName();
    for (QList<QUrl>::const_iterator it = list.begin(); it != list.end(); ++it) {
        QString fileName = it->fileName();
        // Strip filename of its extension, but only if present (see #362690).
        // Use loops to handle cases like `*.tar.gz`.
        while (!QMimeDatabase().mimeTypeForFile(fileName, QMimeDatabase::MatchExtension).isDefault()) {
            const QString strippedName = QFileInfo(fileName).completeBaseName();
            if (strippedName == fileName) {
                break;
            }
            fileName = strippedName;
        }

        if (prefix.length() > fileName.length()) {
            prefix.truncate(fileName.length());
        }

        for (int i = 0; i < prefix.length(); ++i) {
            if (prefix.at(i) != fileName.at(i)) {
                prefix.truncate(i);
                break;
            }
        }
    }

    return prefix;
}

QString AddToArchive::getFileNameForUrls(const QList<QUrl> &urls, const QString &suffix)
{
    Q_ASSERT(!urls.isEmpty());

    const QFileInfo fileInfo = QFileInfo(urls.constFirst().toLocalFile());
    QString base = findCommonPrefixForUrls(urls);
    if (urls.size() > 1 && base.length() < 5) {
        base = i18nc("Default name of a newly-created multi-file archive", "Archive");
    }

    const QString path = fileInfo.absolutePath() + QStringLiteral("/");

    if (suffix.isEmpty()) {
        return path + base;
    }
    QString finalName = base + QLatin1Char('.') + suffix;
    if (QFileInfo::exists(path + finalName)) {
        finalName = KFileUtils::suggestName(QUrl::fromLocalFile(path), finalName);
    }
    return path + finalName;
}

QString AddToArchive::getFileNameForEntries(const QList<Archive::Entry *> &entries, const QString &suffix)
{
    QList<QUrl> urls;
    for (const auto &entry : entries) {
        urls.append(QUrl::fromLocalFile(entry->fullPath()));
    }
    return getFileNameForUrls(urls, suffix);
}

void AddToArchive::setImmediateProgressReporting(bool immediateProgressReporting)
{
    m_immediateProgressReporting = immediateProgressReporting;
}
}

#include "moc_addtoarchive.cpp"
