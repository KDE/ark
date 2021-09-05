/*
    SPDX-FileCopyrightText: 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "queries.h"
#include "ark_debug.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KPasswordDialog>
#include <KIO/RenameDialog>

#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QPointer>
#include <QUrl>

namespace Kerfuffle
{
Query::Query()
{
}

QVariant Query::response() const
{
    return m_data.value(QStringLiteral( "response" ));
}

void Query::waitForResponse()
{
    QMutexLocker locker(&m_responseMutex);
    //if there is no response set yet, wait
    if (!m_data.contains(QLatin1String("response"))) {
        m_responseCondition.wait(&m_responseMutex);
    }
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
    m_data[QStringLiteral("filename")] = filename;
}

void OverwriteQuery::execute()
{
    // If we are being called from the KPart, the cursor is probably Qt::WaitCursor
    // at the moment (#231974)
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

    KIO::RenameDialog_Options options = KIO::RenameDialog_Overwrite | KIO::RenameDialog_Skip;
    if (m_noRenameMode) {
        options = options | KIO::RenameDialog_NoRename;
    }
    if (m_multiMode) {
        options = options | KIO::RenameDialog_MultipleItems;
    }

    QUrl sourceUrl = QUrl::fromLocalFile(QDir::cleanPath(m_data.value(QStringLiteral("filename")).toString()));
    QUrl destUrl = QUrl::fromLocalFile(QDir::cleanPath(m_data.value(QStringLiteral("filename")).toString()));

    QPointer<KIO::RenameDialog> dialog = new KIO::RenameDialog(
        nullptr,
        i18nc("@title:window", "File Already Exists"),
        sourceUrl,
        destUrl,
        options);
    dialog.data()->exec();

    m_data[QStringLiteral("newFilename")] = dialog.data()->newDestUrl().toDisplayString(QUrl::PreferLocalFile);

    setResponse(dialog.data()->result());

    delete dialog.data();

    QApplication::restoreOverrideCursor();
}

bool OverwriteQuery::responseCancelled()
{
    return m_data.value(QStringLiteral( "response" )).toInt() == KIO::Result_Cancel;
}
bool OverwriteQuery::responseOverwriteAll()
{
    return m_data.value(QStringLiteral( "response" )).toInt() == KIO::Result_OverwriteAll;
}
bool OverwriteQuery::responseOverwrite()
{
    return m_data.value(QStringLiteral( "response" )).toInt() == KIO::Result_Overwrite;
}

bool OverwriteQuery::responseRename()
{
    return m_data.value(QStringLiteral( "response" )).toInt() == KIO::Result_Rename;
}

bool OverwriteQuery::responseSkip()
{
    return m_data.value(QStringLiteral( "response" )).toInt() == KIO::Result_Skip;
}

bool OverwriteQuery::responseAutoSkip()
{
    return m_data.value(QStringLiteral( "response" )).toInt() == KIO::Result_AutoSkip;
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
    dlg.data()->setPrompt(xi18nc("@info", "The archive <filename>%1</filename> is password protected. Please enter the password.",
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
    qCDebug(ARK) << "Executing prompt";
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

    setResponse(KMessageBox::warningYesNo(nullptr,
                                          xi18nc("@info", "The archive you're trying to open is corrupt.<nl/>"
                                                 "Some files may be missing or damaged."),
                                          i18nc("@title:window", "Corrupt archive"),
                                          KGuiItem(i18nc("@action:button", "Open as Read-Only")),
                                          KGuiItem(i18nc("@action:button", "Don't Open"))));
    QApplication::restoreOverrideCursor();
}

bool LoadCorruptQuery::responseYes() {
    return (m_data.value(QStringLiteral("response")).toInt() == KMessageBox::Yes);
}

ContinueExtractionQuery::ContinueExtractionQuery(const QString& error, const QString& archiveEntry)
    : m_chkDontAskAgain(i18n("Don't ask again."))
{
    m_data[QStringLiteral("error")] = error;
    m_data[QStringLiteral("archiveEntry")] = archiveEntry;
}

void ContinueExtractionQuery::execute()
{
    qCDebug(ARK) << "Executing prompt";
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

    QMessageBox box(QMessageBox::Warning,
                    i18n("Error during extraction"),
                    xi18n("Extraction of the entry:<nl/>"
                          "    <filename>%1</filename><nl/>"
                          "failed with the error message:<nl/>    %2<nl/><nl/>"
                          "Do you want to continue extraction?<nl/>", m_data.value(QStringLiteral("archiveEntry")).toString(),
                          m_data.value(QStringLiteral("error")).toString()),
                    QMessageBox::Yes|QMessageBox::Cancel);
    box.setCheckBox(&m_chkDontAskAgain);
    setResponse(box.exec());
    QApplication::restoreOverrideCursor();
}

bool ContinueExtractionQuery::responseCancelled() {
    return (m_data.value(QStringLiteral("response")).toInt() == QMessageBox::Cancel);
}

bool ContinueExtractionQuery::dontAskAgain() {
    return m_chkDontAskAgain.isChecked();
}

}
