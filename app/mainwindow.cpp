/*
    SPDX-FileCopyrightText: 2002-2003 Georg Robbers <Georg.Robbers@urz.uni-hd.de>
    SPDX-FileCopyrightText: 2003 Helio Chissini de Castro <helio@conectiva.com>
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>
    SPDX-FileCopyrightText: 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2021 Jiří Wolker <woljiri@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mainwindow.h"
#include "ark_debug.h"
#include "createdialog.h"
#include "settingsdialog.h"
#include "settingspage.h"
#include "pluginmanager.h"
#include "interface.h"

#include <KParts/ReadWritePart>
#include <KPluginFactory>
#include <KMessageBox>
#include <KLocalizedString>
#include <KActionCollection>
#include <KStandardAction>
#include <KRecentFilesMenu>
#include <KSharedConfig>
#include <KConfigDialog>
#include <KXMLGUIFactory>
#include <KConfigSkeleton>
#include <KToolBar>

#include <QApplication>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QFileDialog>
#include <QMenuBar>
#include <QMimeData>
#include <QPointer>
#include <QStatusBar>

static bool isValidArchiveDrag(const QMimeData *data)
{
    return ((data->hasUrls()) && (data->urls().count() == 1));
}

MainWindow::MainWindow(QWidget *)
        : KParts::MainWindow()
        , m_windowContents(new QStackedWidget(this))
{
    setupActions();
    setAcceptDrops(true);
    // Ark doesn't provide a fullscreen mode; remove the corresponding window button
    setWindowFlags(windowFlags() & ~Qt::WindowFullscreenButtonHint);

    setCentralWidget(m_windowContents);
    m_welcomeView = new WelcomeView(this);
    m_windowContents->addWidget(m_welcomeView);
}

MainWindow::~MainWindow()
{
    guiFactory()->removeClient(m_part);
    delete m_part;
    m_part = nullptr;
    m_welcomeView = nullptr;
}

void MainWindow::dragEnterEvent(QDragEnterEvent * event)
{
    qCDebug(ARK) << event;

    Interface *iface = qobject_cast<Interface*>(m_part);
    if (iface->isBusy()) {
        return;
    }

    const bool partAcceptsDrops = !m_part->url().isEmpty() && m_part->isReadWrite();
    if (!event->source() && isValidArchiveDrag(event->mimeData()) && !partAcceptsDrops) {
        event->acceptProposedAction();
    }
    return;
}

void MainWindow::dropEvent(QDropEvent * event)
{
    qCDebug(ARK) << event;

    Interface *iface = qobject_cast<Interface*>(m_part);
    if (iface->isBusy()) {
        return;
    }

    if ((event->source() == nullptr) &&
        (isValidArchiveDrag(event->mimeData()))) {
        event->acceptProposedAction();
    }

    //TODO: if this call provokes a message box the drag will still be going
    //while the box is onscreen. looks buggy, do something about it
    openUrl(event->mimeData()->urls().at(0));
}

void MainWindow::dragMoveEvent(QDragMoveEvent * event)
{
    qCDebug(ARK) << event;

    Interface *iface = qobject_cast<Interface*>(m_part);
    if (iface->isBusy()) {
        return;
    }

    if ((event->source() == nullptr) &&
        (isValidArchiveDrag(event->mimeData()))) {
        event->acceptProposedAction();
    }
}

bool MainWindow::loadPart()
{
    m_part = KPluginFactory::instantiatePlugin<KParts::ReadWritePart>(KPluginMetaData(QStringLiteral("kf" QT_STRINGIFY(QT_VERSION_MAJOR) "/parts/arkpart"))).plugin;

    if (!m_part) {
        KMessageBox::error(this, i18n("Unable to find Ark's KPart component, please check your installation."));
        qCWarning(ARK) << "Error loading Ark KPart.";
        return false;
    }

    m_part->setObjectName(QStringLiteral("ArkPart"));
    QWidget *partwidget = m_part->widget();
    m_windowContents->addWidget(partwidget);
    m_windowContents->setCurrentWidget(partwidget);

    // needs to be above createGUI()
    KHamburgerMenu * const hamburgerMenu = KStandardAction::hamburgerMenu(nullptr, nullptr, m_part->actionCollection());
    connect(hamburgerMenu, &KHamburgerMenu::aboutToShowMenu,
            this, &MainWindow::updateHamburgerMenu);
    hamburgerMenu->setMenuBar(menuBar());

    QAction * const showMenuBarAction = actionCollection()->action(
                                    QLatin1String(KStandardAction::name(KStandardAction::ShowMenubar)));
    hamburgerMenu->setShowMenuBarAction(showMenuBarAction);

    setXMLFile(QStringLiteral("arkui.rc"));
    setupGUI(ToolBar | Keys | Save);
    createGUI(m_part);

    // FIXME: workaround for BUG 171080
    showMenuBarAction->setChecked(!menuBar()->isHidden());

    statusBar()->hide();

    connect(m_part, SIGNAL(ready()), this, SLOT(updateActions()));
    connect(m_part, SIGNAL(ready()), this, SLOT(hideWelcomeScreen()));
    connect(m_part, SIGNAL(quit()), this, SLOT(quit()));
    // #365200: this will disable m_recentFilesAction, while openUrl() will enable it.
    // So updateActions() needs to be called after openUrl() returns.
    connect(m_part, SIGNAL(busy()), this, SLOT(updateActions()), Qt::QueuedConnection);
    connect(m_part, QOverload<>::of(&KParts::ReadOnlyPart::completed), this, &MainWindow::addPartUrl);

    updateActions();
    showWelcomeScreen();

    return true;
}

KRecentFilesMenu *MainWindow::recentFilesMenu() const
{
    return m_recentFilesMenu;
}

void MainWindow::showWelcomeScreen(bool force)
{
    if (!force) {
        const KConfigGroup configGroup = KSharedConfig::openConfig()->group("General");
        if (!configGroup.readEntry("ShowWelcomeScreenOnStartup", true)) {
            return;
        }
    }

    m_windowContents->setCurrentWidget(m_welcomeView);
}

void MainWindow::hideWelcomeScreen()
{
    Q_ASSERT(m_part->widget());
    m_windowContents->setCurrentWidget(m_part->widget());
}

void MainWindow::setupActions()
{
    m_newAction = KStandardAction::openNew(this, &MainWindow::newArchive, this);
    actionCollection()->addAction(QStringLiteral("ark_file_new"), m_newAction);
    m_openAction = KStandardAction::open(this, &MainWindow::openArchive, this);
    actionCollection()->addAction(QStringLiteral("ark_file_open"), m_openAction);
    auto quitAction = KStandardAction::quit(this, &MainWindow::quit, this);
    actionCollection()->addAction(QStringLiteral("ark_quit"), quitAction);
    m_recentFilesMenu = new KRecentFilesMenu(this);
    actionCollection()->addAction(QStringLiteral("ark_file_open_recent"), m_recentFilesMenu->menuAction());
    connect(m_recentFilesMenu, &KRecentFilesMenu::urlTriggered, this, &MainWindow::openUrl);

    KStandardAction::preferences(this, &MainWindow::showSettings, actionCollection());

    QAction *a = actionCollection()->addAction(QStringLiteral("help_welcome_page"));
    a->setText(i18n("Welcome Page"));
    a->setIcon(qApp->windowIcon());
    a->setWhatsThis(i18n("Show the welcome page"));
    connect(a, &QAction::triggered, this, [this]() {
        showWelcomeScreen(true);
    });

    // add Menubar toggle to 'Settings' menu
    KToggleAction* showMenuBar = KStandardAction::showMenubar(nullptr, nullptr, actionCollection());
    showMenuBar->setWhatsThis(xi18nc("@info:whatsthis",
            "This switches between having a <emphasis>Menubar</emphasis> "
            "and having a <interface>Hamburger Menu</interface> button. Both "
            "contain mostly the same commands and configuration options."));
    connect(showMenuBar, &KToggleAction::triggered,                   // Fixes #286822
            this, [this]{ menuBar()->setVisible(!menuBar()->isVisible()); }, Qt::QueuedConnection);
}

void MainWindow::updateHamburgerMenu()
{
    const KActionCollection* ac = m_part->actionCollection();
    auto hamburgerMenu = static_cast<KHamburgerMenu *>(
                    ac->action(QLatin1String(KStandardAction::name(KStandardAction::HamburgerMenu))));
    auto menu = hamburgerMenu->menu();
    if (!menu) {
        menu = new QMenu(this);
        hamburgerMenu->setMenu(menu);
    } else {
        menu->clear();
    }

    if (!toolBar()->isVisible()) {
        // If neither the menu bar nor the toolbar are visible, these actions should be available.
        menu->addAction(actionCollection()->action(QLatin1String(KStandardAction::name(KStandardAction::ShowMenubar))));
        menu->addAction(toolBarMenuAction());
        menu->addSeparator();
    }

    menu->addAction(m_newAction);
    menu->addAction(m_openAction);
    menu->addMenu(m_recentFilesMenu);
    menu->addSeparator();

    menu->addAction(ac->action(QStringLiteral("extract")));
    menu->addAction(ac->action(QStringLiteral("add")));
    menu->addAction(ac->action(QStringLiteral("edit_find")));
    menu->addSeparator();

    menu->addMenu(static_cast<QMenu *>(factory()->container(QStringLiteral("ark_file"), m_part)));
    menu->addSeparator();

    menu->addMenu(static_cast<QMenu *>(factory()->container(QStringLiteral("settings"), this)));
}

void MainWindow::updateActions()
{
    Interface *iface = qobject_cast<Interface*>(m_part);
    Kerfuffle::PluginManager pluginManager;
    m_newAction->setEnabled(!iface->isBusy() && !pluginManager.availableWritePlugins().isEmpty());
    m_openAction->setEnabled(!iface->isBusy());
    m_recentFilesMenu->setEnabled(!iface->isBusy());
}

void MainWindow::openArchive()
{
    Interface *iface = qobject_cast<Interface*>(m_part);
    Q_ASSERT(iface);
    Q_UNUSED(iface);

    Kerfuffle::PluginManager pluginManager;
    auto dlg = new QFileDialog(this, i18nc("to open an archive", "Open Archive"));

    dlg->setMimeTypeFilters(pluginManager.supportedMimeTypes(Kerfuffle::PluginManager::SortByComment));
    dlg->setFileMode(QFileDialog::ExistingFile);
    dlg->setAcceptMode(QFileDialog::AcceptOpen);

    connect(dlg, &QDialog::finished, this, [this, dlg](int result) {
        if (result == QDialog::Accepted) {
            openUrl(dlg->selectedUrls().at(0));
        }
        dlg->deleteLater();
    });

    dlg->open();
}

void MainWindow::openUrl(const QUrl& url)
{
    if (url.isEmpty()) {
        return;
    }

    hideWelcomeScreen();
    m_part->setArguments(m_openArgs);
    m_part->openUrl(url);
}

void MainWindow::setShowExtractDialog(bool option)
{
    if (option) {
        m_openArgs.metaData()[QStringLiteral("showExtractDialog")] = QStringLiteral("true");
    } else {
        m_openArgs.metaData().remove(QStringLiteral("showExtractDialog"));
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Preview windows don't have a parent, so we need to manually close them.
    const auto topLevelWidgets = qApp->topLevelWidgets();
    for (QWidget *widget : topLevelWidgets) {
        if (widget->isVisible()) {
            widget->close();
        }
    }

    KParts::MainWindow::closeEvent(event);
}

// Set a sane default window size
QSize MainWindow::sizeHint() const
{
    return QSize(950, 600);
}

void MainWindow::quit()
{
    close();
}

void MainWindow::showSettings()
{
    if (KConfigDialog::showDialog(QStringLiteral("settings"))) {
        return;
    }

    Interface *iface = qobject_cast<Interface*>(m_part);
    Q_ASSERT(iface);

    auto dialog = new Kerfuffle::SettingsDialog(this, QStringLiteral("settings"), iface->config());

    const auto pages = iface->settingsPages(this);
    for (Kerfuffle::SettingsPage *page : pages) {
        dialog->addPage(page, page->name(), page->iconName());
        connect(dialog, &KConfigDialog::settingsChanged, page, &Kerfuffle::SettingsPage::slotSettingsChanged);
        connect(dialog, &Kerfuffle::SettingsDialog::defaultsButtonClicked, page, &Kerfuffle::SettingsPage::slotDefaultsButtonClicked);
    }
    // Hide the icons list if only one page has been added.
    dialog->setFaceType(KPageDialog::Auto);
    dialog->setModal(true);

    connect(dialog, &KConfigDialog::settingsChanged, this, &MainWindow::writeSettings);
    connect(dialog, &KConfigDialog::settingsChanged, this, &MainWindow::updateActions);
    dialog->show();
}

void MainWindow::writeSettings()
{
    Interface *iface = qobject_cast<Interface*>(m_part);
    Q_ASSERT(iface);
    iface->config()->save();
}

void MainWindow::addPartUrl()
{
    m_recentFilesMenu->addUrl(m_part->url());
}

void MainWindow::newArchive()
{
    qCDebug(ARK) << "Creating new archive";

    Interface *iface = qobject_cast<Interface*>(m_part);
    Q_ASSERT(iface);
    Q_UNUSED(iface);

    QPointer<Kerfuffle::CreateDialog> dialog = new Kerfuffle::CreateDialog(
        nullptr, // parent
        i18n("Create New Archive"), // caption
        QUrl()); // startDir

    if (dialog.data()->exec()) {
        const QUrl saveFileUrl = dialog.data()->selectedUrl();
        const QString password = dialog.data()->password();
        const QString fixedMimeType = dialog.data()->currentMimeType().name();

        qCDebug(ARK) << "CreateDialog returned URL:" << saveFileUrl.toString();
        qCDebug(ARK) << "CreateDialog returned mime:" << fixedMimeType;

        m_openArgs.metaData()[QStringLiteral("createNewArchive")] = QStringLiteral("true");
        m_openArgs.metaData()[QStringLiteral("fixedMimeType")] = fixedMimeType;
        if (dialog.data()->compressionLevel() > -1) {
            m_openArgs.metaData()[QStringLiteral("compressionLevel")] = QString::number(dialog.data()->compressionLevel());
        }
        if (dialog.data()->volumeSize() > 0) {
            qCDebug(ARK) << "Setting volume size:" << QString::number(dialog.data()->volumeSize());
            m_openArgs.metaData()[QStringLiteral("volumeSize")] = QString::number(dialog.data()->volumeSize());
        }
        if (!dialog.data()->compressionMethod().isEmpty()) {
            m_openArgs.metaData()[QStringLiteral("compressionMethod")] = dialog.data()->compressionMethod();
        }
        if (!dialog.data()->encryptionMethod().isEmpty()) {
            m_openArgs.metaData()[QStringLiteral("encryptionMethod")] = dialog.data()->encryptionMethod();
        }

        m_openArgs.metaData()[QStringLiteral("encryptionPassword")] = password;

        if (dialog.data()->isHeaderEncryptionEnabled()) {
            m_openArgs.metaData()[QStringLiteral("encryptHeader")] = QStringLiteral("true");
        }

        openUrl(saveFileUrl);

        m_openArgs.metaData().remove(QStringLiteral("showExtractDialog"));
        m_openArgs.metaData().remove(QStringLiteral("createNewArchive"));
        m_openArgs.metaData().remove(QStringLiteral("fixedMimeType"));
        m_openArgs.metaData().remove(QStringLiteral("compressionLevel"));
        m_openArgs.metaData().remove(QStringLiteral("encryptionPassword"));
        m_openArgs.metaData().remove(QStringLiteral("encryptHeader"));
    }

    delete dialog.data();
}
