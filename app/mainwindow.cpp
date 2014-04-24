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
#include "kerfuffle/archive.h"
#include "part/interface.h"

#include <KPluginLoader>
#include <KPluginFactory>
#include <KMessageBox>
#include <KApplication>
#include <KLocale>
#include <KActionCollection>
#include <KStandardAction>
#include <KFileDialog>
#include <KRecentFilesAction>
#include <KGlobal>
#include <KDebug>
#include <KEditToolBar>
#include <KShortcutsDialog>

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QWeakPointer>

static bool isValidArchiveDrag(const QMimeData *data)
{
    return ((data->hasUrls()) && (data->urls().count() == 1));
}

MainWindow::MainWindow(QWidget *)
        : KParts::MainWindow()
{
    setXMLFile(QLatin1String( "arkui.rc" ));

    setupActions();
    statusBar();

    if (!initialGeometrySet()) {
        resize(640, 480);
    }
    setAutoSaveSettings(QLatin1String( "MainWindow" ));

    setAcceptDrops(true);
}

MainWindow::~MainWindow()
{
    if (m_recentFilesAction) {
        m_recentFilesAction->saveEntries(KGlobal::config()->group("Recent Files"));
    }
    delete m_part;
    m_part = 0;
}

void MainWindow::dragEnterEvent(QDragEnterEvent * event)
{
    kDebug() << event;

    Interface *iface = qobject_cast<Interface*>(m_part);
    if (iface->isBusy()) {
        return;
    }

    if ((event->source() == NULL) &&
        (isValidArchiveDrag(event->mimeData()))) {
        event->acceptProposedAction();
    }
    return;
}

void MainWindow::dropEvent(QDropEvent * event)
{
    kDebug() << event;

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
    kDebug() << event;

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
    KPluginLoader loader(QLatin1String( "arkpart" ));
    KPluginFactory *factory = loader.factory();
    if (factory) {
        m_part = static_cast<KParts::ReadWritePart*>(factory->create<KParts::ReadWritePart>(this));
    }
    if (!factory || !m_part) {
        KMessageBox::error(this, i18n("Unable to find Ark's KPart component, please check your installation."));
        kWarning() << "Error loading Ark KPart: " << loader.errorString();
        return false;
    }

    m_part->setObjectName( QLatin1String("ArkPart" ));
    setCentralWidget(m_part->widget());
    createGUI(m_part);

    connect(m_part, SIGNAL(busy()), this, SLOT(updateActions()));
    connect(m_part, SIGNAL(ready()), this, SLOT(updateActions()));
    connect(m_part, SIGNAL(quit()), this, SLOT(quit()));

    return true;
}

void MainWindow::setupActions()
{
    m_newAction = KStandardAction::openNew(this, SLOT(newArchive()), actionCollection());
    m_openAction = KStandardAction::open(this, SLOT(openArchive()), actionCollection());
    KStandardAction::quit(this, SLOT(quit()), actionCollection());

    m_recentFilesAction = KStandardAction::openRecent(this, SLOT(openUrl(KUrl)), actionCollection());
    m_recentFilesAction->setToolBarMode(KRecentFilesAction::MenuMode);
    m_recentFilesAction->setToolButtonPopupMode(QToolButton::DelayedPopup);
    m_recentFilesAction->setIconText(i18nc("action, to open an archive", "Open"));
    m_recentFilesAction->setStatusTip(i18n("Click to open an archive, click and hold to open a recently-opened archive"));
    m_recentFilesAction->setToolTip(i18n("Open an archive"));
    m_recentFilesAction->loadEntries(KGlobal::config()->group("Recent Files"));
    connect(m_recentFilesAction, SIGNAL(triggered()),
            this, SLOT(openArchive()));

    createStandardStatusBarAction();

    KStandardAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());
    KStandardAction::keyBindings(this, SLOT(editKeyBindings()), actionCollection());
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
    saveMainWindowSettings(KGlobal::config()->group(QLatin1String("MainWindow")));

    QWeakPointer<KEditToolBar> dlg = new KEditToolBar(factory(), this);
    dlg.data()->exec();

    createGUI(m_part);

    applyMainWindowSettings(KGlobal::config()->group(QLatin1String("MainWindow")));

    delete dlg.data();
}

void MainWindow::openArchive()
{
    Interface *iface = qobject_cast<Interface*>(m_part);
    Q_ASSERT(iface);
    const KUrl url = KFileDialog::getOpenUrl(KUrl("kfiledialog:///ArkOpenDir"),
                                       Kerfuffle::supportedMimeTypes().join( QLatin1String( " " )),
                                       this);
    openUrl(url);
}

void MainWindow::openUrl(const KUrl& url)
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
        m_openArgs.metaData()[QLatin1String( "showExtractDialog" )] = QLatin1String( "true" );
    } else {
        m_openArgs.metaData().remove(QLatin1String( "showExtractDialog" ));
    }
}

void MainWindow::quit()
{
    close();
}

void MainWindow::newArchive()
{
    Interface *iface = qobject_cast<Interface*>(m_part);
    Q_ASSERT(iface);

    const QStringList mimeTypes = Kerfuffle::supportedWriteMimeTypes();

    kDebug() << "Supported mimetypes are" << mimeTypes.join( QLatin1String( " " ));

    const KUrl saveFileUrl =
        KFileDialog::getSaveUrl(KUrl("kfiledialog:///ArkNewDir"),
                                mimeTypes.join(QLatin1String(" ")));

    m_openArgs.metaData()[QLatin1String( "createNewArchive" )] = QLatin1String( "true" );

    openUrl(saveFileUrl);

    m_openArgs.metaData().remove(QLatin1String( "showExtractDialog" ));
    m_openArgs.metaData().remove(QLatin1String( "createNewArchive" ));
}
