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
#include "kdelibs/knewpassworddialog.h"
#include "kdelibs/renamedialog.h"

#include <KLocale>
#include <KPasswordDialog>
#include <kdebug.h>

#include <QApplication>
#include <QWeakPointer>

namespace Kerfuffle
{
Query::Query()
{
    m_responseMutex.lock();
}

QVariant Query::response()
{
    return m_data.value(QLatin1String( "response" ));
}

void Query::waitForResponse()
{
    kDebug(1601);

    //if there is no response set yet, wait
    if (!m_data.contains(QLatin1String("response"))) {
        m_responseCondition.wait(&m_responseMutex);
    }
    m_responseMutex.unlock();
}

void Query::setResponse(QVariant response)
{
    kDebug(1601);

    m_data[QLatin1String( "response" )] = response;
    m_responseCondition.wakeAll();
}

OverwriteQuery::OverwriteQuery(const QString &filename) :
    m_noRenameMode(false),
    m_multiMode(true),
    m_updateExistingMode(false)
{
    m_data[QLatin1String( "filename" )] = filename;
}

void OverwriteQuery::execute()
{
    // If we are being called from the KPart, the cursor is probably Qt::WaitCursor
    // at the moment (#231974)
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

    RenameDialog_Mode mode = (RenameDialog_Mode)(M_OVERWRITE | M_SKIP);
    if (m_noRenameMode) {
        mode = (RenameDialog_Mode)(mode | M_NORENAME);
    }
    if (m_multiMode) {
        mode = (RenameDialog_Mode)(mode | M_MULTI);
    }
    if (m_updateExistingMode) {
        mode = (RenameDialog_Mode)(mode | M_UPDATE_EXISTING);
    }

    KUrl sourceUrl(m_data.value(QLatin1String( "filename" )).toString());
    KUrl destUrl(m_data.value(QLatin1String( "filename" )).toString());
    sourceUrl.cleanPath();
    destUrl.cleanPath();

    QWeakPointer<RenameDialog> dialog = new RenameDialog(
        NULL,
        i18nc("@info", "File already exists"),
        sourceUrl,
        destUrl,
        mode);
    dialog.data()->exec();

    m_data[QLatin1String("newFilename")] = dialog.data()->newDestUrl().pathOrUrl();

    setResponse(dialog.data()->result());

    delete dialog.data();

    QApplication::restoreOverrideCursor();
}

bool OverwriteQuery::responseCancelled()
{
    return m_data.value(QLatin1String( "response" )).toInt() == R_CANCEL;
}
bool OverwriteQuery::responseOverwriteAll()
{
    return m_data.value(QLatin1String( "response" )).toInt() == R_OVERWRITE_ALL;
}
bool OverwriteQuery::responseOverwrite()
{
    return m_data.value(QLatin1String( "response" )).toInt() == R_OVERWRITE;
}

bool OverwriteQuery::responseRename()
{
    return m_data.value(QLatin1String( "response" )).toInt() == R_RENAME;
}

bool OverwriteQuery::responseSkip()
{
    return m_data.value(QLatin1String( "response" )).toInt() == R_SKIP;
}

bool OverwriteQuery::responseAutoSkip()
{
    return m_data.value(QLatin1String( "response" )).toInt() == R_AUTO_SKIP;
}


bool OverwriteQuery::responseUpdateExisting()
{
    return m_data.value(QLatin1String( "response" )).toInt() == R_UPDATE_EXISTING;
}

QString OverwriteQuery::newFilename()
{
    return m_data.value(QLatin1String( "newFilename" )).toString();
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

void OverwriteQuery::setUpdateExistingMode(bool updateExisting)
{
    m_updateExistingMode = updateExisting;
}

bool OverwriteQuery::updateExistingMode()
{
    return m_updateExistingMode;
}

PasswordNeededQuery::PasswordNeededQuery(const QString& archiveFilename, const PasswordFlags flags)
{
    m_data[QLatin1String( "archiveFilename" )] = archiveFilename;
    m_data[QLatin1String( "incorrectTryAgain" )] = flags.testFlag(IncorrectTryAgain);
    m_data[QLatin1String( "askNewPassword" )] = flags.testFlag(AskNewPassword);
}

void PasswordNeededQuery::execute()
{
    // If we are being called from the KPart, the cursor is probably Qt::WaitCursor
    // at the moment (#231974)
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

    bool notCancelled = false;
    QString password;

    if (m_data.value(QLatin1String("askNewPassword")).toBool()) {
        QWeakPointer<KNewPasswordDialog> dlg = new KNewPasswordDialog;
        dlg.data()->setPrompt(i18nc("@info", "Please enter the password to protect <filename>%1</filename>.", m_data.value(QLatin1String( "archiveFilename" )).toString()));
        dlg.data()->setAllowEmptyPasswords(false);
    
        notCancelled = dlg.data()->exec();
        if (dlg) {
            password = dlg.data()->password();
            dlg.data()->deleteLater();
        }
    } else {
        QWeakPointer<KPasswordDialog> dlg = new KPasswordDialog;
        dlg.data()->setPrompt(i18nc("@info", "The archive <filename>%1</filename> is password protected. Please enter the password to extract the file.", m_data.value(QLatin1String( "archiveFilename" )).toString()));
    
        if (m_data.value(QLatin1String("incorrectTryAgain")).toBool()) {
            dlg.data()->showErrorMessage(i18nc("@info", "Incorrect password, please try again."), KPasswordDialog::PasswordError);
        }
    
        notCancelled = dlg.data()->exec();
        if (dlg) {
            password = dlg.data()->password();
            dlg.data()->deleteLater();
        }
    }

    m_data[QLatin1String("password")] = password;
    setResponse(notCancelled && !password.isEmpty());

    QApplication::restoreOverrideCursor();
}

QString PasswordNeededQuery::password()
{
    return m_data.value(QLatin1String( "password" )).toString();
}

bool PasswordNeededQuery::responseCancelled()
{
    return !m_data.value(QLatin1String( "response" )).toBool();
}
}
