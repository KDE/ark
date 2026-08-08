/*
    SPDX-FileCopyrightText: 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "queries.h"
#include "ark_debug.h"

#include <KIO/RenameDialog>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPasswordDialog>
#include <KPasswordLineEdit>
#include <KProtocolManager>

#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QUrl>

namespace Kerfuffle
{
Query::Query()
{
}

QVariant Query::response() const
{
    return data(QStringLiteral("response"));
}

QVariant Query::data(const QString &key) const
{
    QMutexLocker locker(&m_responseMutex);
    return m_data.value(key);
}

void Query::setData(const QString &key, const QVariant &value)
{
    QMutexLocker locker(&m_responseMutex);
    m_data[key] = value;
}

void Query::waitForResponse()
{
    QMutexLocker locker(&m_responseMutex);
    // A thread can be woken for no reason of ours, so the answer is what says to go on.
    while (!m_data.contains(QLatin1String("response"))) {
        m_responseCondition.wait(&m_responseMutex);
    }
}

void Query::setResponse(const QVariant &response)
{
    QMutexLocker locker(&m_responseMutex);
    // The question was given up on and answered already, and whoever asked it has moved
    // on, so a dialog that ends after that has nothing left to say.
    if (m_aborted) {
        return;
    }
    m_data[QStringLiteral("response")] = response;
    m_responseCondition.wakeAll();
}

void Query::abort()
{
    QMutexLocker locker(&m_responseMutex);
    // An answered question has nothing left to give up on, and its answer stands.
    if (m_data.contains(QLatin1String("response"))) {
        return;
    }

    m_aborted = true;
    m_data[QStringLiteral("response")] = cancelledResponse();
    m_responseCondition.wakeAll();
}

OverwriteQuery::OverwriteQuery(const QString &filename)
    : m_noRenameMode(false)
    , m_multiMode(true)
{
    setData(QStringLiteral("filename"), filename);
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

    const QString sourcePath = data(QStringLiteral("filename")).toString();

    QUrl sourceUrl;
    // Try to use an archive KIO (e.g. zip:/) URL as source URL so we get a proper file preview.
    if (!m_archiveMimeType.isEmpty()) {
        if (const QString scheme = KProtocolManager::protocolForArchiveMimetype(m_archiveMimeType); !scheme.isEmpty()) {
            sourceUrl.setScheme(scheme);
        }
    }
    if (sourceUrl.scheme().isEmpty()) {
        sourceUrl.setScheme(QStringLiteral("ark"));
    }

    sourceUrl.setPath(m_archiveFileName + QLatin1Char('/') + sourcePath);

    QUrl destUrl = QUrl::fromLocalFile(m_destination);

    QPointer<KIO::RenameDialog> dialog = new KIO::RenameDialog(nullptr, i18nc("@title:window", "File Already Exists"), sourceUrl, destUrl, options);
    dialog.data()->exec();

    setData(QStringLiteral("newFilename"), dialog.data()->newDestUrl().toDisplayString(QUrl::PreferLocalFile));

    setResponse(dialog.data()->result());

    delete dialog.data();

    QApplication::restoreOverrideCursor();
}

QVariant OverwriteQuery::cancelledResponse() const
{
    return QVariant(KIO::Result_Cancel);
}

bool OverwriteQuery::responseCancelled()
{
    return data(QStringLiteral("response")).toInt() == KIO::Result_Cancel;
}
bool OverwriteQuery::responseOverwriteAll()
{
    return data(QStringLiteral("response")).toInt() == KIO::Result_OverwriteAll;
}
bool OverwriteQuery::responseOverwrite()
{
    return data(QStringLiteral("response")).toInt() == KIO::Result_Overwrite;
}

bool OverwriteQuery::responseRename()
{
    return data(QStringLiteral("response")).toInt() == KIO::Result_Rename;
}

bool OverwriteQuery::responseSkip()
{
    return data(QStringLiteral("response")).toInt() == KIO::Result_Skip;
}

bool OverwriteQuery::responseAutoSkip()
{
    return data(QStringLiteral("response")).toInt() == KIO::Result_AutoSkip;
}

QString OverwriteQuery::newFilename()
{
    return data(QStringLiteral("newFilename")).toString();
}

void OverwriteQuery::setArchiveFileName(const QString &fileName)
{
    m_archiveFileName = fileName;
}

void OverwriteQuery::setArchiveMimeType(const QString &mimeType)
{
    m_archiveMimeType = mimeType;
}

void OverwriteQuery::setDestination(const QString &destination)
{
    m_destination = destination;
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

PasswordNeededQuery::PasswordNeededQuery(const QString &archiveFilename, bool incorrectTryAgain)
{
    setData(QStringLiteral("archiveFilename"), archiveFilename);
    setData(QStringLiteral("incorrectTryAgain"), incorrectTryAgain);
}

void PasswordNeededQuery::execute()
{
    qCDebug(ARK_LOG) << "Executing password prompt";

    // If we are being called from the KPart, the cursor is probably Qt::WaitCursor
    // at the moment (#231974)
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

    QPointer<KPasswordDialog> dlg = new KPasswordDialog;

    // Disabling Ok button on dialog open
    dlg.data()->buttonBox()->button(QDialogButtonBox::Ok)->setEnabled(false);

    auto linePassword = dlg.data()->findChild<KPasswordLineEdit *>(QStringLiteral("passEdit"));

    // If password is non empty, enable submit button
    QObject::connect(linePassword->lineEdit(), &QLineEdit::textChanged, linePassword->lineEdit(), [=] {
        if (linePassword->lineEdit()->text().isEmpty()) {
            dlg.data()->buttonBox()->button(QDialogButtonBox::Ok)->setEnabled(false);
        } else {
            dlg.data()->buttonBox()->button(QDialogButtonBox::Ok)->setEnabled(true);
        }
    });

    dlg.data()->setPrompt(xi18nc("@info",
                                 "The archive <filename>%1</filename> is password protected. Please enter the password.",
                                 data(QStringLiteral("archiveFilename")).toString()));

    if (data(QStringLiteral("incorrectTryAgain")).toBool()) {
        dlg.data()->showErrorMessage(i18n("Incorrect password, please try again."), KPasswordDialog::PasswordError);
    }

    const bool notCancelled = dlg.data()->exec();
    const QString password = dlg.data()->password();

    setData(QStringLiteral("password"), password);
    setResponse(notCancelled && !password.isEmpty());

    QApplication::restoreOverrideCursor();

    delete dlg.data();
}

QString PasswordNeededQuery::password()
{
    return data(QStringLiteral("password")).toString();
}

QVariant PasswordNeededQuery::cancelledResponse() const
{
    return QVariant(false);
}

bool PasswordNeededQuery::responseCancelled()
{
    return !data(QStringLiteral("response")).toBool();
}

LoadCorruptQuery::LoadCorruptQuery(const QString &archiveFilename)
{
    setData(QStringLiteral("archiveFilename"), archiveFilename);
}

void LoadCorruptQuery::execute()
{
    qCDebug(ARK_LOG) << "Executing prompt";
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

    setResponse(KMessageBox::warningTwoActions(nullptr,
                                               xi18nc("@info",
                                                      "The archive you're trying to open is corrupt.<nl/>"
                                                      "Some files may be missing or damaged."),
                                               i18nc("@title:window", "Corrupt archive"),
                                               KGuiItem(i18nc("@action:button", "Open as Read-Only")),
                                               KGuiItem(i18nc("@action:button", "Don't Open"))));
    QApplication::restoreOverrideCursor();
}

QVariant LoadCorruptQuery::cancelledResponse() const
{
    return QVariant(KMessageBox::SecondaryAction);
}

bool LoadCorruptQuery::responseYes()
{
    return (data(QStringLiteral("response")).toInt() == KMessageBox::PrimaryAction);
}

ContinueExtractionQuery::ContinueExtractionQuery(const QString &error, const QString &archiveEntry)
    : m_chkDontAskAgain(i18n("Don't ask again."))
{
    setData(QStringLiteral("error"), error);
    setData(QStringLiteral("archiveEntry"), archiveEntry);
}

void ContinueExtractionQuery::execute()
{
    qCDebug(ARK_LOG) << "Executing prompt";
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

    QMessageBox box(QMessageBox::Warning,
                    i18n("Error during extraction"),
                    xi18n("Extraction of the entry:<nl/>"
                          "    <filename>%1</filename><nl/>"
                          "failed with the error message:<nl/>    %2<nl/><nl/>"
                          "Do you want to continue extraction?<nl/>",
                          data(QStringLiteral("archiveEntry")).toString(),
                          data(QStringLiteral("error")).toString()),
                    QMessageBox::Yes | QMessageBox::Cancel);
    box.setCheckBox(&m_chkDontAskAgain);
    setResponse(box.exec());
    QApplication::restoreOverrideCursor();
}

QVariant ContinueExtractionQuery::cancelledResponse() const
{
    return QVariant(QMessageBox::Cancel);
}

bool ContinueExtractionQuery::responseCancelled()
{
    return (data(QStringLiteral("response")).toInt() == QMessageBox::Cancel);
}

bool ContinueExtractionQuery::dontAskAgain()
{
    return m_chkDontAskAgain.isChecked();
}

}
