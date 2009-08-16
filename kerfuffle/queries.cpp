/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008-2009 Harald Hvaal <haraldhv (at@at) stud.ntnu.no>
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

#include "queries.h"

#include <KLocale>
#include <KPasswordDialog>
#include <kdebug.h>
#include <kio/renamedialog.h>

#include <QApplication>
#include <QMessageBox>
#include <QPointer>

namespace Kerfuffle
{
Query::Query()
{
    m_responseMutex.lock();
}

QVariant Query::response()
{
    return m_data.value("response");
}

void Query::waitForResponse()
{
    kDebug(1601);

    //if there is no response set yet, wait
    if (!m_data.contains("response"))
        m_responseCondition.wait(&m_responseMutex);
    m_responseMutex.unlock();
}

void Query::setResponse(QVariant response)
{
    kDebug(1601);

    m_data["response"] = response;
    m_responseCondition.wakeAll();
}

OverwriteQuery::OverwriteQuery(QString filename) :
        m_noRenameMode(false),
        m_multiMode(true)
{
    m_data["filename"] = filename;
}

void OverwriteQuery::execute()
{
    KIO::RenameDialog_Mode mode = (KIO::RenameDialog_Mode)(KIO::M_OVERWRITE | KIO::M_SKIP);
    if (m_noRenameMode) {
        mode = (KIO::RenameDialog_Mode)(mode | KIO::M_NORENAME);
    }
    if (m_multiMode) {
        mode = (KIO::RenameDialog_Mode)(mode | KIO::M_MULTI);
    }

    KUrl sourceUrl(m_data.value("filename").toString());
    KUrl destUrl(m_data.value("filename").toString());
    sourceUrl.cleanPath();
    destUrl.cleanPath();

    QPointer<KIO::RenameDialog> dialog = new KIO::RenameDialog(
        NULL,
        i18n("File already exists"),
        sourceUrl,
        destUrl,
        mode);
    dialog->exec();

    m_data["newFilename"] = dialog->newDestUrl().pathOrUrl();

    setResponse(dialog->result());

    delete dialog;
}

bool OverwriteQuery::responseCancelled()
{
    return m_data.value("response").toInt() == KIO::R_CANCEL;
}
bool OverwriteQuery::responseOverwriteAll()
{
    return m_data.value("response").toInt() == KIO::R_OVERWRITE_ALL;
}
bool OverwriteQuery::responseOverwrite()
{
    return m_data.value("response").toInt() == KIO::R_OVERWRITE;
}

bool OverwriteQuery::responseRename()
{
    return m_data.value("response").toInt() == KIO::R_RENAME;
}

bool OverwriteQuery::responseSkip()
{
    return m_data.value("response").toInt() == KIO::R_SKIP;
}

bool OverwriteQuery::responseAutoSkip()
{
    return m_data.value("response").toInt() == KIO::R_AUTO_SKIP;
}

QString OverwriteQuery::newFilename()
{
    return m_data.value("newFilename").toString();
}

void OverwriteQuery::setNoRenameMode(bool enableNoRenameMode)
{
    m_noRenameMode = enableNoRenameMode;
}

bool OverwriteQuery::noRenameMode()
{
    return m_noRenameMode;
}

void OverwriteQuery::setMultiMode(bool enableMultiMode)
{
    m_multiMode = enableMultiMode;
}

bool OverwriteQuery::multiMode()
{
    return m_multiMode;
}

PasswordNeededQuery::PasswordNeededQuery(QString archiveFilename, bool incorrectTryAgain)
{
    m_data["archiveFilename"] = archiveFilename;
    m_data["incorrectTryAgain"] = incorrectTryAgain;
}

void PasswordNeededQuery::execute()
{
    QPointer<KPasswordDialog> dlg = new KPasswordDialog(NULL);
    dlg->setPrompt(i18n("The archive '%1' is password protected. Please enter the password to extract the file.", m_data.value("archiveFilename").toString()));

    if (m_data.value("incorrectTryAgain").toBool()) {
        dlg->showErrorMessage(i18n("Incorrect password, please try again."), KPasswordDialog::PasswordError);
    }

    if (!dlg->exec()) {
        setResponse(false);
    } else {
        m_data["password"] = dlg->password();
        setResponse(true);
    }

    delete dlg;
}

QString PasswordNeededQuery::password()
{
    return m_data.value("password").toString();
}

bool PasswordNeededQuery::responseCancelled()
{
    return !m_data.value("response").toBool();
}
}
