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
#include "ark_debug.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KPasswordDialog>
#include <kio/renamedialog.h>

#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QMessageBox>
#include <QPointer>
#include <QUrl>

namespace Kerfuffle
{
Query::Query()
{
    m_responseMutex.lock();
}

QVariant Query::response() const
{
    return m_data.value(QStringLiteral( "response" ));
}

void Query::waitForResponse()
{
    //if there is no response set yet, wait
    if (!m_data.contains(QStringLiteral("response"))) {
        m_responseCondition.wait(&m_responseMutex);
    }
    m_responseMutex.unlock();
}

void Query::setResponse(const QVariant &response)
{
    m_data[QStringLiteral( "response" )] = response;
    m_responseCondition.wakeAll();
}

OverwriteQuery::OverwriteQuery(const QString &filename) :
        m_noRenameMode(false),
        m_multiMode(true)
{
    m_data[QStringLiteral( "filename" )] = filename;
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

    QUrl sourceUrl(QDir::cleanPath(m_data.value(QStringLiteral( "filename" )).toString()));
    QUrl destUrl(QDir::cleanPath(m_data.value(QStringLiteral( "filename" )).toString()));

    QPointer<KIO::RenameDialog> dialog = new KIO::RenameDialog(
        Q_NULLPTR,
        i18n("File already exists"),
        sourceUrl,
        destUrl,
        mode);
    dialog.data()->exec();

    m_data[QStringLiteral("newFilename")] = dialog.data()->newDestUrl().toDisplayString(QUrl::PreferLocalFile);

    setResponse(dialog.data()->result());

    delete dialog.data();

    QApplication::restoreOverrideCursor();
}

bool OverwriteQuery::responseCancelled()
{
    return m_data.value(QStringLiteral( "response" )).toInt() == KIO::R_CANCEL;
}
bool OverwriteQuery::responseOverwriteAll()
{
    return m_data.value(QStringLiteral( "response" )).toInt() == KIO::R_OVERWRITE_ALL;
}
bool OverwriteQuery::responseOverwrite()
{
    return m_data.value(QStringLiteral( "response" )).toInt() == KIO::R_OVERWRITE;
}

bool OverwriteQuery::responseRename()
{
    return m_data.value(QStringLiteral( "response" )).toInt() == KIO::R_RENAME;
}

bool OverwriteQuery::responseSkip()
{
    return m_data.value(QStringLiteral( "response" )).toInt() == KIO::R_SKIP;
}

bool OverwriteQuery::responseAutoSkip()
{
    return m_data.value(QStringLiteral( "response" )).toInt() == KIO::R_AUTO_SKIP;
}

QString OverwriteQuery::newFilename()
{
    return m_data.value(QStringLiteral( "newFilename" )).toString();
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

PasswordNeededQuery::PasswordNeededQuery(const QString& archiveFilename, bool incorrectTryAgain)
{
    m_data[QStringLiteral( "archiveFilename" )] = archiveFilename;
    m_data[QStringLiteral( "incorrectTryAgain" )] = incorrectTryAgain;
}

void PasswordNeededQuery::execute()
{
    qCDebug(ARK) << "Executing password prompt";

    // If we are being called from the KPart, the cursor is probably Qt::WaitCursor
    // at the moment (#231974)
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

    QPointer<KPasswordDialog> dlg = new KPasswordDialog;
    dlg.data()->setPrompt(xi18nc("@info", "The archive <filename>%1</filename> is password protected. Please enter the password to extract the file.",
                                 m_data.value(QStringLiteral("archiveFilename")).toString()));

    if (m_data.value(QStringLiteral("incorrectTryAgain")).toBool()) {
        dlg.data()->showErrorMessage(i18n("Incorrect password, please try again."), KPasswordDialog::PasswordError);
    }

    const bool notCancelled = dlg.data()->exec();
    const QString password = dlg.data()->password();

    m_data[QStringLiteral("password")] = password;
    setResponse(notCancelled && !password.isEmpty());

    QApplication::restoreOverrideCursor();

    delete dlg.data();
}

QString PasswordNeededQuery::password()
{
    return m_data.value(QStringLiteral( "password" )).toString();
}

bool PasswordNeededQuery::responseCancelled()
{
    return !m_data.value(QStringLiteral( "response" )).toBool();
}

LoadCorruptQuery::LoadCorruptQuery(const QString& archiveFilename)
{
    m_data[QStringLiteral("archiveFilename")] = archiveFilename;
}

void LoadCorruptQuery::execute()
{
    qCDebug(ARK) << "Executing LoadCorrupt prompt";
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

    setResponse(KMessageBox::warningYesNo(Q_NULLPTR,
                                          xi18nc("@info", "Corrupt archive detected. Do you want Ark to attempt "
                                                 "loading the archive?<nl/><nl/>Some files may be missing "
                                                 "or damaged, and the archive will be opened read-only."),
                                          i18nc("@title:window", "Corrupt archive")));
    QApplication::restoreOverrideCursor();
}

bool LoadCorruptQuery::responseYes() {
    return (m_data.value(QStringLiteral("response")).toInt() == KMessageBox::Yes);
}

ContinueExtractionQuery::ContinueExtractionQuery(const QString& error, const QString& archiveEntry)
{
    m_data[QStringLiteral("error")] = error;
    m_data[QStringLiteral("archiveEntry")] = archiveEntry;
}

void ContinueExtractionQuery::execute()
{
    qCDebug(ARK) << "Executing ContinueExtraction prompt";
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

    QMessageBox box(QMessageBox::Warning,
                    i18n("Error during extraction"),
                    xi18n("Extraction of the entry:<nl/>"
                          "    <filename>%1</filename><nl/>"
                          "failed with the error message:<nl/>    %2<nl/><nl/>"
                          "Do you want to continue extraction?<nl/>", m_data.value(QStringLiteral("archiveEntry")).toString(),
                          m_data.value(QStringLiteral("error")).toString()),
                    QMessageBox::Yes|QMessageBox::Cancel);
    m_chkDontAskAgain = new QCheckBox(i18n("Don't ask again."));
    box.setCheckBox(m_chkDontAskAgain);
    setResponse(box.exec());
    QApplication::restoreOverrideCursor();
}

bool ContinueExtractionQuery::responseCancelled() {
    return (m_data.value(QStringLiteral("response")).toInt() == QMessageBox::Cancel);
}

bool ContinueExtractionQuery::dontAskAgain() {
    return m_chkDontAskAgain->isChecked();
}

}
