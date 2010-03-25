/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
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
    kDebug();

    //if there is no response set yet, wait
    if (!m_data.contains("response"))
        m_responseCondition.wait(&m_responseMutex);
    m_responseMutex.unlock();
}

void Query::setResponse(QVariant response)
{
    kDebug();

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
    // If we are being called from the KPart, the cursor is probably Qt::WaitCursor
    // at the moment (#231974)
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

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

    QApplication::restoreOverrideCursor();
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
    // If we are being called from the KPart, the cursor is probably Qt::WaitCursor
    // at the moment (#231974)
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

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

    QApplication::restoreOverrideCursor();

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
