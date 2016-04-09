/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2002-2003: Georg Robbers <Georg.Robbers@urz.uni-hd.de>
 * Copyright (C) 2003: Helio Chissini de Castro <helio@conectiva.com>
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (C) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
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

#include "mainwindow.h"
#include "ark_debug.h"
#include "kerfuffle/archive_kerfuffle.h"
#include "kerfuffle/createdialog.h"
#include "kerfuffle/settingspage.h"
#include "mimetypes.h"
#include "part/interface.h"

#include <KPluginFactory>
#include <KMessageBox>
#include <KLocalizedString>
#include <KActionCollection>
#include <KStandardAction>
#include <KRecentFilesAction>
#include <KEditToolBar>
#include <KShortcutsDialog>
#include <KService>
#include <KSharedConfig>
#include <KConfigDialog>
#include <KConfigGroup>
#include <KConfigSkeleton>
#include <KXMLGUIFactory>

#include <QDebug>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QFileDialog>
#include <QMimeData>
#include <QPointer>
#include <QRegularExpression>
#include <QStatusBar>

static bool isValidArchiveDrag(const QMimeData *data)
{
    return ((data->hasUrls()) && (data->urls().count() == 1));
}

MainWindow::MainWindow(QWidget *)
        : KParts::MainWindow()
{
    setXMLFile(QStringLiteral("arkui.rc"));

    setupActions();

    resize(640, 480);

    setAutoSaveSettings(QStringLiteral("MainWindow"));

    setAcceptDrops(true);
}

MainWindow::~MainWindow()
{
    if (m_recentFilesAction) {
        m_recentFilesAction->saveEntries(KSharedConfig::openConfig()->group("Recent Files"));
    }

    guiFactory()->removeClient(m_part);
    delete m_part;
    m_part = 0;
}

void MainWindow::dragEnterEvent(QDragEnterEvent * event)
{
    qCDebug(ARK) << "dragEnterEvent" << event;

    Interface *iface = qobject_cast<Interface*>(m_part);
    if (iface->isBusy()) {
        return;
    }

    if (!event->source() && isValidArchiveDrag(event->mimeData())) {
        event->acceptProposedAction();
    }
    return;
}

void MainWindow::dropEvent(QDropEvent * event)
{
    qCDebug(ARK) << "dropEvent" << event;

    Interface *iface = qobject_cast<Interface*>(m_part);
    if (iface->isBusy()) {
        return;
    }

    if ((event->source() == NULL) &&
        (isValidArchiveDrag(event->mimeData()))) {
        event->acceptProposedAction();
    }

    //TODO: if this call provokes a message box the drag will still be going
    //while the box is onscreen. looks buggy, do something about it
    openUrl(event->mimeData()->urls().at(0));
}

void MainWindow::dragMoveEvent(QDragMoveEvent * event)
{
    qCDebug(ARK) << "dragMoveEvent" << event;

    Interface *iface = qobject_cast<Interface*>(m_part);
    if (iface->isBusy()) {
        return;
    }

    if ((event->source() == NULL) &&
        (isValidArchiveDrag(event->mimeData()))) {
        event->acceptProposedAction();
    }
}

bool MainWindow::loadPart()
{
    KPluginFactory *factory = 0;
    KService::Ptr service = KService::serviceByDesktopName(QStringLiteral("ark_part"));

    if (service) {
        factory = KPluginLoader(service->library()).factory();
    }

    m_part = factory ? static_cast<KParts::ReadWritePart*>(factory->create<KParts::ReadWritePart>(this)) : 0;

    if (!m_part) {
        KMessageBox::error(this, i18n("Unable to find Ark's KPart component, please check your installation."));
        qCWarning(ARK) << "Error loading Ark KPart.";
        return false;
    }

    m_part->setObjectName(QStringLiteral("ArkPart"));
    setCentralWidget(m_part->widget());
    createGUI(m_part);
    statusBar()->hide();

    connect(m_part, SIGNAL(busy()), this, SLOT(updateActions()));
    connect(m_part, SIGNAL(ready()), this, SLOT(updateActions()));
    connect(m_part, SIGNAL(quit()), this, SLOT(quit()));

    return true;
}

void MainWindow::setupActions()
{
    m_newAction = actionCollection()->addAction(KStandardAction::New, QStringLiteral("ark_file_new"), this, SLOT(newArchive()));
    m_openAction = actionCollection()->addAction(KStandardAction::Open, QStringLiteral("ark_file_open"), this, SLOT(openArchive()));
    actionCollection()->addAction(KStandardAction::Quit, QStringLiteral("ark_quit"), this, SLOT(quit()));

    m_recentFilesAction = KStandardAction::openRecent(this, SLOT(openUrl(QUrl)), Q_NULLPTR);
    actionCollection()->addAction(QStringLiteral("ark_file_open_recent"), m_recentFilesAction);

    m_recentFilesAction->setToolBarMode(KRecentFilesAction::MenuMode);
    m_recentFilesAction->setToolButtonPopupMode(QToolButton::DelayedPopup);
    m_recentFilesAction->setIconText(i18nc("action, to open an archive", "Open"));
    m_recentFilesAction->setToolTip(i18n("Open an archive"));
    m_recentFilesAction->loadEntries(KSharedConfig::openConfig()->group("Recent Files"));
    connect(m_recentFilesAction, SIGNAL(triggered()),
            this, SLOT(openArchive()));

    KStandardAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());
    KStandardAction::keyBindings(this, SLOT(editKeyBindings()), actionCollection());
    KStandardAction::preferences(this, SLOT(showSettings()), actionCollection());
}

void MainWindow::updateActions()
{
    Interface *iface = qobject_cast<Interface*>(m_part);
    m_newAction->setEnabled(!iface->isBusy());
    m_openAction->setEnabled(!iface->isBusy());
    m_recentFilesAction->setEnabled(!iface->isBusy());
}

void MainWindow::editKeyBindings()
{
    KShortcutsDialog dlg(KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsAllowed, this);
    dlg.addCollection(actionCollection());
    dlg.addCollection(m_part->actionCollection());

    dlg.configure();
}

void MainWindow::editToolbars()
{
    KConfigGroup cfg(KSharedConfig::openConfig(), "MainWindow");
    saveMainWindowSettings(cfg);

    QPointer<KEditToolBar> dlg = new KEditToolBar(factory(), this);
    dlg.data()->exec();

    createGUI(m_part);

    applyMainWindowSettings(KSharedConfig::openConfig()->group(QStringLiteral("MainWindow")));

    delete dlg.data();
}

void MainWindow::openArchive()
{
    Interface *iface = qobject_cast<Interface*>(m_part);
    Q_ASSERT(iface);
    Q_UNUSED(iface);

    QPointer<QFileDialog> dlg = new QFileDialog(this, i18nc("to open an archive", "Open Archive"));

    dlg->setMimeTypeFilters(Kerfuffle::supportedMimeTypes());

    dlg->setFileMode(QFileDialog::ExistingFile);
    dlg->setAcceptMode(QFileDialog::AcceptOpen);
    if (dlg->exec() == QDialog::Accepted) {
        openUrl(dlg->selectedUrls().first());
    }
    delete dlg;
}

void MainWindow::openUrl(const QUrl& url)
{
    if (!url.isEmpty()) {
        m_part->setArguments(m_openArgs);

        if (m_part->openUrl(url)) {
            m_recentFilesAction->addUrl(url);
        } else {
            m_recentFilesAction->removeUrl(url);
        }
    }
}

void MainWindow::setShowExtractDialog(bool option)
{
    if (option) {
        m_openArgs.metaData()[QStringLiteral("showExtractDialog")] = QStringLiteral("true");
    } else {
        m_openArgs.metaData().remove(QStringLiteral("showExtractDialog"));
    }
}

void MainWindow::quit()
{
    close();
}

void MainWindow::showSettings()
{
    Interface *iface = qobject_cast<Interface*>(m_part);
    Q_ASSERT(iface);

    KConfigDialog *dialog = new KConfigDialog(this, QStringLiteral("settings"), iface->config());

    foreach (Kerfuffle::SettingsPage *page, iface->settingsPages(this)) {
        dialog->addPage(page, page->name(), page->iconName());
    }
    // Hide the icons list if only one page has been added.
    dialog->setFaceType(KPageDialog::Auto);

    connect(dialog, &KConfigDialog::settingsChanged, this, &MainWindow::writeSettings);
    dialog->show();
}

void MainWindow::writeSettings()
{
    Interface *iface = qobject_cast<Interface*>(m_part);
    Q_ASSERT(iface);
    iface->config()->save();
}

void MainWindow::newArchive()
{
    qCDebug(ARK) << "Creating new archive";

    Interface *iface = qobject_cast<Interface*>(m_part);
    Q_ASSERT(iface);
    Q_UNUSED(iface);

    QPointer<Kerfuffle::CreateDialog> dialog = new Kerfuffle::CreateDialog(
        Q_NULLPTR, // parent
        i18n("Create New Archive"), // caption
        QUrl()); // startDir

    if (dialog.data()->exec()) {
        const QUrl saveFileUrl = dialog.data()->selectedUrl();
        const QString password = dialog.data()->password();

        qCDebug(ARK) << "CreateDialog returned URL:" << saveFileUrl.toString();
        qCDebug(ARK) << "CreateDialog returned mime:" << dialog.data()->currentMimeType().name();

        m_openArgs.metaData()[QStringLiteral("createNewArchive")] = QStringLiteral("true");
        if (dialog.data()->compressionLevel() > -1) {
            m_openArgs.metaData()[QStringLiteral("compressionLevel")] = QString::number(dialog.data()->compressionLevel());
        }
        m_openArgs.metaData()[QStringLiteral("encryptionPassword")] = password;

        if (dialog.data()->isHeaderEncryptionEnabled()) {
            m_openArgs.metaData()[QStringLiteral("encryptHeader")] = QStringLiteral("true");
        }

        openUrl(saveFileUrl);

        m_openArgs.metaData().remove(QStringLiteral("showExtractDialog"));
        m_openArgs.metaData().remove(QStringLiteral("createNewArchive"));
        m_openArgs.metaData().remove(QStringLiteral("compressionLevel"));
        m_openArgs.metaData().remove(QStringLiteral("encryptionPassword"));
        m_openArgs.metaData().remove(QStringLiteral("encryptHeader"));
    }

    delete dialog.data();
}
