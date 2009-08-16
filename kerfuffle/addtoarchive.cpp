/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv (at@at) stud.ntnu.no>
 * Copyright (C) 2009 Raphael Kubo da Costa <kubito@gmail.com>
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

#include "addtoarchive.h"
#include "adddialog.h"

#include <QFileInfo>
#include <QDir>
#include <QPointer>

#include <KConfig>
#include <kdebug.h>
#include <kjobtrackerinterface.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <KStandardDirs>

namespace Kerfuffle
{
AddToArchive::AddToArchive(QObject *parent)
        : KJob(parent), m_changeToFirstPath(false)
{
}

AddToArchive::~AddToArchive()
{
}

bool AddToArchive::showAddDialog(void)
{
    QPointer<Kerfuffle::AddDialog> dialog = new Kerfuffle::AddDialog(
        m_inputs, // itemsToAdd
        KUrl(m_firstPath), // startDir
        "", // filter
        NULL, // parent
        NULL); // widget

    bool ret = dialog->exec();

    if (ret) {
        kDebug(1601) << "Returned URL:" << dialog->selectedUrl();
        kDebug(1601) << "Returned mime:" << dialog->currentMimeFilter();
        setFilename(dialog->selectedUrl());
        setMimeType(dialog->currentMimeFilter());
    }

    delete dialog;

    return ret;
}

bool AddToArchive::addInput(const KUrl& url)
{
    m_inputs << url.pathOrUrl(
        QFileInfo(url.pathOrUrl()).isDir() ?
        KUrl::AddTrailingSlash :
        KUrl::RemoveTrailingSlash
    );

    if (m_firstPath.isEmpty()) {
        QString firstEntry = url.pathOrUrl(KUrl::RemoveTrailingSlash);
        m_firstPath = QFileInfo(firstEntry).dir().absolutePath();
    }

    return true;
}

// TODO: If this class should ever be called outside main.cpp,
//       the returns should be preceded by emitResult().
void AddToArchive::start(void)
{
    kDebug(1601);

    Kerfuffle::CompressionOptions options;

    if (!m_inputs.size()) {
        KMessageBox::error(NULL, i18n("No input files were given."));
        return;
    }

    Kerfuffle::Archive *archive;
    if (!m_filename.isEmpty()) {
        archive = Kerfuffle::factory(m_filename, m_mimeType);
        kDebug(1601) << "Set filename to " + m_filename;
    } else {
        if (m_autoFilenameSuffix.isEmpty()) {
            KMessageBox::error(NULL, i18n("You need to either supply a filename for the archive or a suffix (such as rar, tar.gz) with the --autofilename argument."));
            return;
        }

        if (m_firstPath.isEmpty()) {
            kDebug(1601) << "Weird, this should not happen. no firstpath defined. aborting";
            return;
        }

        QString base;
        QFileInfo fi(m_inputs.first());

        base = fi.absoluteFilePath();

        if (base.endsWith('/')) {
            base.chop(1);
        }

        QString finalName = base + '.' + m_autoFilenameSuffix;

        //if file already exists, append a number to the base until it doesn't
        //exist
        int appendNumber = 0;
        while (KStandardDirs::exists(finalName)) {
            ++appendNumber;
            finalName = base + '_' + QString::number(appendNumber) + '.' + m_autoFilenameSuffix;
        }

        kDebug(1601) << "Autoset filename to " + finalName;
        archive = Kerfuffle::factory(finalName, m_mimeType);
    }

    // TODO Post-4.3 string freeze: the check for read-only must cause a separate error
    if (archive == NULL || archive->isReadOnly()) {
        KMessageBox::error(NULL, i18n("Failed to create the new archive. Permissions might not be sufficient."));
        return;
    }

    if (m_changeToFirstPath) {
        if (m_firstPath.isEmpty()) {
            kDebug(1601) << "Weird, this should not happen. no firstpath defined. aborting";
            return;
        }

        QDir stripDir = QDir(m_firstPath);

        for (int i = 0; i < m_inputs.size(); ++i) {
            m_inputs[i] = stripDir.absoluteFilePath(m_inputs.at(i));
        }

        options["GlobalWorkDir"] = stripDir.path();
        kDebug(1601) << "Setting GlobalWorkDir to " << stripDir.path();
    }

    Kerfuffle::AddJob *job =
        archive->addFiles(m_inputs, options);

    KIO::getJobTracker()->registerJob(job);

    connect(job, SIGNAL(result(KJob*)),
            this, SLOT(slotFinished(KJob*)));

    job->start();
}

void AddToArchive::slotFinished(KJob *job)
{
    kDebug(1601);

    if (job->error()) {
        KMessageBox::error(NULL, job->errorText());
    }

    emitResult();
}
}
