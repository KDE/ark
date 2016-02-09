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
#include "ark_debug.h"
#include "adddialog.h"
#include "archive_kerfuffle.h"
#include "jobs.h"

#include <KConfig>
#include <kjobtrackerinterface.h>
#include <kmessagebox.h>
#include <KLocalizedString>
#include <kio/job.h>

#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QPointer>

namespace Kerfuffle
{
AddToArchive::AddToArchive(QObject *parent)
        : KJob(parent), m_changeToFirstPath(false)
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
    m_filename = path.toDisplayString(QUrl::PreferLocalFile);
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

    QPointer<Kerfuffle::AddDialog> dialog = new Kerfuffle::AddDialog(
        Q_NULLPTR, // parent
        i18n("Compress to Archive"), // caption
        QUrl::fromLocalFile(m_firstPath), // startDir
        m_inputs); // itemsToAdd

    dialog.data()->show();
    dialog.data()->restoreWindowSize();
    bool ret = dialog.data()->exec();

    if (ret) {
        qCDebug(ARK) << "AddDialog returned URL:" << dialog.data()->selectedUrls().at(0).toString();
        qCDebug(ARK) << "AddDialog returned mime:" << dialog.data()->currentMimeFilter();
        setFilename(dialog.data()->selectedUrls().at(0));
        setMimeType(dialog.data()->currentMimeFilter());
        setPassword(dialog.data()->password());
        setHeaderEncryptionEnabled(dialog.data()->isHeaderEncryptionChecked());
    }

    delete dialog.data();

    return ret;
}

bool AddToArchive::addInput(const QUrl &url)
{
    m_inputs << url.toDisplayString(QUrl::PreferLocalFile);

    if (m_firstPath.isEmpty()) {
        QString firstEntry = url.toDisplayString(QUrl::PreferLocalFile);
        m_firstPath = QFileInfo(firstEntry).dir().absolutePath();
    }

    return true;
}

void AddToArchive::start()
{
    qCDebug(ARK) << "Starting job";

    QTimer::singleShot(0, this, &AddToArchive::slotStartJob);
}

void AddToArchive::slotStartJob()
{
    Kerfuffle::CompressionOptions options;

    if (!m_inputs.size()) {
        KMessageBox::error(NULL, i18n("No input files were given."));
        emitResult();
        return;
    }

    Kerfuffle::Archive *archive;
    if (!m_filename.isEmpty()) {
        archive = Kerfuffle::Archive::create(m_filename, m_mimeType, this);
        qCDebug(ARK) << "Set filename to " << m_filename;
    } else {
        if (m_autoFilenameSuffix.isEmpty()) {
            KMessageBox::error(Q_NULLPTR, xi18n("You need to either supply a filename for the archive or a suffix (such as rar, tar.gz) with the <command>--autofilename</command> argument."));
            emitResult();
            return;
        }

        if (m_firstPath.isEmpty()) {
            qCWarning(ARK) << "Weird, this should not happen. no firstpath defined. aborting";
            emitResult();
            return;
        }

        QString base = QFileInfo(m_inputs.first()).absoluteFilePath();
        if (base.endsWith(QLatin1Char('/'))) {
            base.chop(1);
        }

        QString finalName = base + QLatin1Char( '.' ) + m_autoFilenameSuffix;

        //if file already exists, append a number to the base until it doesn't
        //exist
        int appendNumber = 0;
        while (QFileInfo(finalName).exists()) {
            ++appendNumber;
            finalName = base + QLatin1Char( '_' ) + QString::number(appendNumber) + QLatin1Char( '.' ) + m_autoFilenameSuffix;
        }

        qCDebug(ARK) << "Autoset filename to "<< finalName;
        archive = Kerfuffle::Archive::create(finalName, m_mimeType, this);
    }

    Q_ASSERT(archive);

    if (!archive->isValid()) {
        KMessageBox::error(Q_NULLPTR, i18n("Failed to create the new archive. Permissions might not be sufficient."));
        emitResult();
        return;
    } else if (archive->isReadOnly()) {
        KMessageBox::error(Q_NULLPTR, i18n("It is not possible to create archives of this type."));
        emitResult();
        return;
    }

    if (!m_password.isEmpty()) {
        archive->setPassword(m_password);
        archive->enableHeaderEncryption(m_enableHeaderEncryption);
    }

    if (m_changeToFirstPath) {
        if (m_firstPath.isEmpty()) {
            qCWarning(ARK) << "Weird, this should not happen. no firstpath defined. aborting";
            emitResult();
            return;
        }

        const QDir stripDir(m_firstPath);

        for (int i = 0; i < m_inputs.size(); ++i) {
            m_inputs[i] = stripDir.absoluteFilePath(m_inputs.at(i));
        }


        options[QStringLiteral( "GlobalWorkDir" )] = stripDir.path();
        qCDebug(ARK) << "Setting GlobalWorkDir to " << stripDir.path();
    }

    Kerfuffle::AddJob *job =
        archive->addFiles(m_inputs, options);

    KIO::getJobTracker()->registerJob(job);

    connect(job, &Kerfuffle::AddJob::result, this, &AddToArchive::slotFinished);

    job->start();
}

void AddToArchive::slotFinished(KJob *job)
{
    qCDebug(ARK) << "AddToArchive job finished";

    if (job->error() && !job->errorText().isEmpty()) {
        KMessageBox::error(Q_NULLPTR, job->errorText());
    }

    emitResult();
}
}
