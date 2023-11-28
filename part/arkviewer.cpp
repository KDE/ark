/*
    ark: A program for modifying archives via a GUI.

    SPDX-FileCopyrightText: 2004-2008 Henrique Pinto <henrique.pinto@kdemail.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "arkviewer.h"
#include "ark_debug.h"

#include <KAboutPluginDialog>
#include <KActionCollection>
#include <KApplicationTrader>
#include <KIO/ApplicationLauncherJob>
#include <KIO/JobUiDelegate>
#include <KIO/JobUiDelegateFactory>
#include <KLocalizedString>
#include <KMessageBox>
#include <KParts/OpenUrlArguments>
#include <KParts/PartLoader>
#include <KPluginMetaData>
#include <KXMLGUIFactory>

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

    KStandardAction::close(this, &QMainWindow::close, actionCollection());

    QAction *closeAction = actionCollection()->addAction(QStringLiteral("close"), this, &ArkViewer::close);
    closeAction->setShortcut(Qt::Key_Escape);

    setXMLFile(QStringLiteral("ark_viewer.rc"));
    setupGUI(ToolBar | Save);
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
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
    // The temporary file will be removed when the viewer application exits.
    job->setRunFlags(KIO::ApplicationLauncherJob::DeleteTemporaryFiles);
    job->start();
}

void ArkViewer::openInternalViewer(const KPluginMetaData& viewer, const QString& fileName, const QString& entryPath, const QMimeType& mimeType)
{
    qCDebug(ARK) << "Opening internal viewer";

    ArkViewer *internalViewer = new ArkViewer();
    internalViewer->show();
    if (internalViewer->viewInInternalViewer(viewer, fileName, entryPath, mimeType)) {
        // The internal viewer is showing the file, and will
        // remove the temporary file in its destructor.  So there
        // is no more to do here.
        return;
    }
    else {
        KMessageBox::error(nullptr, i18n("The internal viewer cannot preview this file."));
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

void ArkViewer::view(const QString& fileName, const QString& entryPath, const QMimeType& mimeType)
{
    QMimeDatabase db;
    qCDebug(ARK) << "viewing" << fileName << "from" << entryPath << "with mime type:" << mimeType.name();

    const std::optional<KPluginMetaData> internalViewer = ArkViewer::getInternalViewer(mimeType.name());

    if (internalViewer) {
        openInternalViewer(*internalViewer, fileName, entryPath, mimeType);
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
        const std::optional<KPluginMetaData> textViewer = ArkViewer::getInternalViewer(QStringLiteral("text/plain"));

        if (textViewer) {
            openInternalViewer(*textViewer, fileName, entryPath, db.mimeTypeForName(QStringLiteral("text/plain")));
            return;
        }
    }
    qCDebug(ARK) << "Removing temporary file:" << fileName;
    QFile::remove(fileName);
}

bool ArkViewer::viewInInternalViewer(const KPluginMetaData& viewer, const QString& fileName, const QString& entryPath, const QMimeType &mimeType)
{
    // Set icon and comment for the mimetype.
    m_iconLabel->setPixmap(QIcon::fromTheme(mimeType.iconName()).pixmap(style()->pixelMetric(QStyle::PixelMetric::PM_SmallIconSize)));
    m_commentLabel->setText(mimeType.comment());

    // Create the ReadOnlyPart instance.
    const auto result = KParts::PartLoader::instantiatePart<KParts::ReadOnlyPart>(viewer, this, this);

    if (!result) {
        qCDebug(ARK) << "Failed to create internal viewer" << result.errorString;
        return false;
    }

    m_part = result.plugin;

    if (!m_part.data()) {
        return false;
    }

    // Insert the KPart into its placeholder.
    mainLayout->insertWidget(0, m_part->widget());

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

    // Needs to come after openUrl to override the part-provided caption
    setWindowTitle(entryPath);
    setWindowFilePath(fileName);

    return true;
}

KService::Ptr ArkViewer::getExternalViewer(const QString &mimeType)
{
    return KApplicationTrader::preferredService(mimeType);
}

std::optional<KPluginMetaData> ArkViewer::getInternalViewer(const QString& mimeType)
{
    // No point in even trying to find anything for application/octet-stream
    if (mimeType == QLatin1String("application/octet-stream")) {
        return {};
    }

    // Try to get a read-only kpart for the internal viewer
    QVector<KPluginMetaData> offers = KParts::PartLoader::partsForMimeType(mimeType);

    auto arkPartIt = std::find_if(offers.begin(), offers.end(), [](const KPluginMetaData& service) {
        return service.pluginId() == QLatin1String("arkpart");
    });

    // Use the Ark part only when the mime type matches an archive type directly.
    // Many file types (e.g. Open Document) are technically just archives
    // but browsing their internals is typically not what the user wants.
    if (arkPartIt != offers.end()) {
        // Not using hasMimeType() as we're explicitly not interested in inheritance.
        if (!arkPartIt->mimeTypes().contains(mimeType)) {
            offers.erase(arkPartIt);
        }
    }

    // Skip the KHTML part
    auto khtmlPart = std::find_if(offers.begin(), offers.end(), [](const KPluginMetaData &part) {
        return part.pluginId() == QLatin1String("khtmlpart");
    });

    if (khtmlPart != offers.end()) {
        offers.erase(khtmlPart);
    }


    // The oktetapart can open any file, but a hex viewer isn't really useful here
    // Skip it so we prefer an external viewer instead
    auto oktetaPart = std::find_if(offers.begin(), offers.end(), [](const KPluginMetaData &part) {
        return part.pluginId() == QLatin1String("oktetapart");
    });

    if (oktetaPart != offers.end()) {
        offers.erase(oktetaPart);
    }

    if (!offers.isEmpty()) {
        return offers.at(0);
    } else {
        return {};
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

#include "moc_arkviewer.cpp"
