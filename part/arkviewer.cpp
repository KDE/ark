/*
 * ark: A program for modifying archives via a GUI.
 *
 * Copyright (C) 2004-2008 Henrique Pinto <henrique.pinto@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#include "arkviewer.h"
#include "ark_debug.h"

#include <KIO/JobUiDelegate>
#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>
#include <KMimeTypeTrader>
#include <KMessageBox>
#include <KParts/OpenUrlArguments>
#include <KXMLGUIFactory>
#include <KActionCollection>
#include <KAboutPluginDialog>
#include <KPluginMetaData>

#include <QFile>
#include <QMimeDatabase>
#include <QProgressDialog>
#include <QPushButton>
#include <QStyle>
#include <QAction>

#include <algorithm>

ArkViewer::ArkViewer()
        : KParts::MainWindow()
{
    setupUi(this);

    m_buttonBox->button(QDialogButtonBox::Close)->setShortcut(Qt::Key_Escape);
    // Bug 369390: This prevents the Enter key from closing the window.
    m_buttonBox->button(QDialogButtonBox::Close)->setAutoDefault(false);

    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QMainWindow::close);

    KStandardAction::close(this, &QMainWindow::close, actionCollection());

    setXMLFile(QStringLiteral("ark_viewer.rc"));
    setupGUI(ToolBar);
}

ArkViewer::~ArkViewer()
{
    if (m_part) {
        QProgressDialog progressDialog(this);
        progressDialog.setWindowTitle(i18n("Closing preview"));
        progressDialog.setLabelText(i18n("Please wait while the preview is being closed..."));

        progressDialog.setMinimumDuration(500);
        progressDialog.setModal(true);
        progressDialog.setCancelButton(nullptr);
        progressDialog.setRange(0, 0);

        // #261785: this preview dialog is not modal, so we need to delete
        //          the previewed file ourselves when the dialog is closed;

        m_part.data()->closeUrl();

        if (!m_fileName.isEmpty()) {
            QFile::remove(m_fileName);
        }
    }

    guiFactory()->removeClient(m_part);
    delete m_part;
}

void ArkViewer::openExternalViewer(const KService::Ptr viewer, const QString& fileName)
{
    qCDebug(ARK) << "Using external viewer";

    const QList<QUrl> fileUrlList = {QUrl::fromLocalFile(fileName)};
    KIO::ApplicationLauncherJob *job = new KIO::ApplicationLauncherJob(viewer);
    job->setUrls(fileUrlList);
    job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
    // The temporary file will be removed when the viewer application exits.
    job->setRunFlags(KIO::ApplicationLauncherJob::DeleteTemporaryFiles);
    job->start();
}

void ArkViewer::openInternalViewer(const KService::Ptr viewer, const QString& fileName, const QMimeType& mimeType)
{
    qCDebug(ARK) << "Opening internal viewer";

    ArkViewer *internalViewer = new ArkViewer();
    internalViewer->show();
    if (internalViewer->viewInInternalViewer(viewer, fileName, mimeType)) {
        // The internal viewer is showing the file, and will
        // remove the temporary file in its destructor.  So there
        // is no more to do here.
        return;
    }
    else {
        KMessageBox::sorry(nullptr, i18n("The internal viewer cannot preview this file."));
        delete internalViewer;

        qCDebug(ARK) << "Removing temporary file:" << fileName;
        QFile::remove(fileName);
    }
}

bool ArkViewer::askViewAsPlainText(const QMimeType& mimeType)
{
    int response;
    if (!mimeType.isDefault()) {
        // File has a defined MIME type, and not the default
        // application/octet-stream.  So it could be viewable as
        // plain text, ask the user.
        response = KMessageBox::warningContinueCancel(nullptr,
            xi18n("The internal viewer cannot preview this type of file<nl/>(%1).<nl/><nl/>Do you want to try to view it as plain text?", mimeType.name()),
            i18nc("@title:window", "Cannot Preview File"),
            KGuiItem(i18nc("@action:button", "Preview as Text"), QIcon::fromTheme(QStringLiteral("text-plain"))),
            KStandardGuiItem::cancel(),
            QStringLiteral("PreviewAsText_%1").arg(mimeType.name()));
    }
    else {
        // No defined MIME type, or the default application/octet-stream.
        // There is still a possibility that it could be viewable as plain
        // text, so ask the user.  Not the same as the message/question
        // above, because the wording and default are different.
        response = KMessageBox::warningContinueCancel(nullptr,
            xi18n("The internal viewer cannot preview this unknown type of file.<nl/><nl/>Do you want to try to view it as plain text?"),
            i18nc("@title:window", "Cannot Preview File"),
            KGuiItem(i18nc("@action:button", "Preview as Text"), QIcon::fromTheme(QStringLiteral("text-plain"))),
            KStandardGuiItem::cancel(),
            QString(),
            KMessageBox::Dangerous);
    }

    return response != KMessageBox::Cancel;
}

void ArkViewer::view(const QString& fileName)
{
    QMimeDatabase db;
    QMimeType mimeType = db.mimeTypeForFile(fileName);
    qCDebug(ARK) << "viewing" << fileName << "with mime type:" << mimeType.name();

    const KService::Ptr internalViewer = ArkViewer::getInternalViewer(mimeType.name());

    if (internalViewer) {
        openInternalViewer(internalViewer, fileName, mimeType);
        return;
    }

    const KService::Ptr externalViewer = ArkViewer::getExternalViewer(mimeType.name());

    if (externalViewer) {
        openExternalViewer(externalViewer, fileName);
        return;
    }

    // No internal or external viewer available for the file.  Ask the user if it
    // should be previewed as text/plain.
    if (askViewAsPlainText(mimeType)) {
        const KService::Ptr textViewer = ArkViewer::getInternalViewer(QStringLiteral("text/plain"));
        openInternalViewer(textViewer, fileName, db.mimeTypeForName(QStringLiteral("text/plain")));
    } else {
        qCDebug(ARK) << "Removing temporary file:" << fileName;
        QFile::remove(fileName);
    }
}

bool ArkViewer::viewInInternalViewer(const KService::Ptr viewer, const QString& fileName, const QMimeType &mimeType)
{
    setWindowFilePath(fileName);

    // Set icon and comment for the mimetype.
    m_iconLabel->setPixmap(QIcon::fromTheme(mimeType.iconName()).pixmap(style()->pixelMetric(QStyle::PixelMetric::PM_SmallIconSize)));
    m_commentLabel->setText(mimeType.comment());

    // Create the ReadOnlyPart instance.
    m_part = viewer->createInstance<KParts::ReadOnlyPart>(this, this);

    if (!m_part.data()) {
        return false;
    }

    // Insert the KPart into its placeholder.
    centralWidget()->layout()->replaceWidget(m_partPlaceholder, m_part.data()->widget());

    QAction* action = actionCollection()->addAction(QStringLiteral("help_about_kpart"));
    const KPluginMetaData partMetaData = m_part->metaData();
    const QString iconName = partMetaData.iconName();
    if (!iconName.isEmpty()) {
        action->setIcon(QIcon::fromTheme(iconName));
    }
    action->setText(i18nc("@action", "About Viewer Component"));
    connect(action, &QAction::triggered, this, &ArkViewer::aboutKPart);

    createGUI(m_part.data());
    setAutoSaveSettings(QStringLiteral("Viewer"), true);

    m_part.data()->openUrl(QUrl::fromLocalFile(fileName));
    m_part.data()->widget()->setFocus();
    m_fileName = fileName;

    return true;
}

KService::Ptr ArkViewer::getExternalViewer(const QString &mimeType)
{
    KService::List offers = KMimeTypeTrader::self()->query(mimeType, QStringLiteral("Application"));

    if (!offers.isEmpty()) {
        return offers.first();
    } else {
        return KService::Ptr();
    }
}

KService::Ptr ArkViewer::getInternalViewer(const QString& mimeType)
{
    // No point in even trying to find anything for application/octet-stream
    if (mimeType == QLatin1String("application/octet-stream")) {
        return KService::Ptr();
    }

    // Try to get a read-only kpart for the internal viewer
    KService::List offers = KMimeTypeTrader::self()->query(mimeType, QStringLiteral("KParts/ReadOnlyPart"));

    auto arkPartIt = std::find_if(offers.begin(), offers.end(), [](KService::Ptr service) {
        return service->storageId() == QLatin1String("ark_part.desktop");
    });

    // Use the Ark part only when the mime type matches an archive type directly.
    // Many file types (e.g. Open Document) are technically just archives
    // but browsing their internals is typically not what the user wants.
    if (arkPartIt != offers.end()) {
        // Not using hasMimeType() as we're explicitly not interested in inheritance.
        if (!(*arkPartIt)->mimeTypes().contains(mimeType)) {
            offers.erase(arkPartIt);
        }
    }

    // Skip the KHTML part
    auto khtmlPart = std::find_if(offers.begin(), offers.end(), [](KService::Ptr service) {
        return service->desktopEntryName() == QLatin1String("khtml");
    });

    if (khtmlPart != offers.end()) {
        offers.erase(khtmlPart);
    }

    if (!offers.isEmpty()) {
        return offers.first();
    } else {
        return KService::Ptr();
    }
}

void ArkViewer::aboutKPart()
{
    if (!m_part) {
        return;
    }

    auto *dialog = new KAboutPluginDialog(m_part->metaData(), this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}
