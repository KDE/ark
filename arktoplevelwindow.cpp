/*

 ark -- archiver for the KDE project

 Copyright (C) 2002: Georg Robbers <Georg.Robbers@urz.uni-hd.de>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

// ark includes
#include "arkapp.h"
#include "ark_part.h"
#include "arksettings.h"
#include "arkwidget.h"

// KDE includes
#include <kdebug.h>
#include <klocale.h>
#include <kedittoolbar.h>
#include <kstatusbar.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kparts/componentfactory.h>
#include <kkeydialog.h>

#include <qwhatsthis.h>

ArkTopLevelWindow::ArkTopLevelWindow( QWidget *parent, const char *name ) :
        KParts::MainWindow()
{
    setXMLFile( "arkui.rc" );
    m_part = KParts::ComponentFactory::createPartInstanceFromLibrary<ArkPart>( "libarkpart", this, name, this, name);
    if (m_part )
    {
        //Since most of the functionality is still in ArkWidget:
        m_widget = static_cast< ArkWidget* >( m_part->widget() );

        setStandardToolBarMenuEnabled( true );
        setupActions();
        setupStatusBar();

        connect( m_part->widget(), SIGNAL( request_file_quit() ), this, SLOT(  file_quit() ) );

        m_part->setArchivePopupEnabled( false );
        connect( m_part->widget(), SIGNAL( signalArchivePopup( const QPoint & ) ), this,
                 SLOT( slotArchivePopup( const QPoint & ) ) );

        connect( m_part->widget(), SIGNAL( setStatusBarText( const QString & ) ), this,
                 SLOT( slotSetStatusBarText( const QString & ) ) );
        connect( m_part->widget(), SIGNAL( setStatusBarSelectedFiles( const QString & ) ), this,
                 SLOT( slotSetStatusBarSelectedFiles( const QString & ) ) );

        connect( m_part, SIGNAL(  removeRecentURL( const QString & ) ), this,
                 SLOT( slotRemoveRecentURL( const QString & ) ) );
        connect( m_part, SIGNAL( addRecentURL( const QString & ) ), this,
                 SLOT( slotAddRecentURL( const QString & ) ) );
        connect( m_part, SIGNAL( fixActionState( const bool & ) ), this,
                 SLOT( slotFixActionState( const bool & ) ) );
        connect( m_widget, SIGNAL( disableAllActions() ), this,
                 SLOT( slotDisableActions() ) );

        ArkApplication::getInstance()->addWindow();
        connect( m_widget, SIGNAL( removeOpenArk( const  KURL &) ), this,
                 SLOT( slotRemoveOpenArk( const KURL & ) ) );
        connect( m_widget, SIGNAL( addOpenArk( const  KURL & ) ), this,
                 SLOT( slotAddOpenArk( const KURL & ) ) );

        setCentralWidget( m_part->widget() );
        createGUI( m_part );
        setAutoSaveSettings( "MainWindow" );
    }
    else
        kdFatal( 1601 ) << "libark could not found. Aborting. " << endl;

}



ArkTopLevelWindow::~ArkTopLevelWindow()
{
    ArkApplication::getInstance()->removeWindow();
    delete m_part;
}

void ArkTopLevelWindow::setupActions()
{
    newWindowAction = new KAction(i18n("New &Window"), "window_new", KShortcut(), this,
                                  SLOT(file_newWindow()), actionCollection(), "new_window");

    newArchAction = KStdAction::openNew(this, SLOT(file_new()), actionCollection());
    openAction = KStdAction::open(this, SLOT(file_open()), actionCollection());

    reloadAction = new KAction(i18n("Re&load"), "reload", 0, this,
                               SLOT(file_reload()), actionCollection(), "reload_arch");
    saveAsAction = KStdAction::saveAs(this, SLOT(file_save_as()), actionCollection());
    closeAction = KStdAction::close(this, SLOT(file_close()), actionCollection(), "file_close");

    recent = KStdAction::openRecent(this, SLOT(openURL(const KURL&)), actionCollection());
    KConfig *kc = m_widget->settings()->getKConfig();
    recent->loadEntries(kc);

    KStdAction::quit(this, SLOT(window_close()), actionCollection());
    statusbarAction = KStdAction::showStatusbar(this, SLOT(toggleStatusBar()), actionCollection());
    KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());
    KStdAction::keyBindings(this, SLOT( slotConfigureKeyBindings()), actionCollection());
    KStdAction::saveOptions(this, SLOT(slotSaveOptions()), actionCollection());
    KStdAction::preferences(this, SLOT(slotPreferences()), actionCollection());

    openAction->setEnabled( true );
    recent->setEnabled( true );
    closeAction->setEnabled( false );
    saveAsAction->setEnabled( false );
    reloadAction->setEnabled( false );
}

void ArkTopLevelWindow::slotDisableActions()
{
    openAction->setEnabled(false);
    newArchAction->setEnabled(false);
    closeAction->setEnabled(false);
    saveAsAction->setEnabled(false);
    reloadAction->setEnabled(false);
}

void ArkTopLevelWindow::slotFixActionState( const bool & bHaveFiles )
{
    openAction->setEnabled(true);
    newArchAction->setEnabled(true);
    closeAction->setEnabled(bHaveFiles);
    saveAsAction->setEnabled(bHaveFiles);
    reloadAction->setEnabled(bHaveFiles);

}

void ArkTopLevelWindow::file_newWindow()
{
    ArkTopLevelWindow *kw = new ArkTopLevelWindow;
    kw->resize( 640, 300 );
    kw->show();
}

void ArkTopLevelWindow::file_new()
{
    m_widget->file_new();
}

void ArkTopLevelWindow::file_reload()
{
    KURL url( m_part->url() );
    file_close();
    m_part->openURL( url );
}

void ArkTopLevelWindow::file_save_as()
{
    KURL u = m_widget->getSaveAsFileName();
    if ( m_widget->allowedArchiveName( u ) )
    {
        m_part->saveAs( u );
        m_part->openURL( u );
    }
}

void ArkTopLevelWindow::editToolbars()
{
    saveMainWindowSettings( KGlobal::config(), QString::fromLatin1("MainWindow") );
    KEditToolbar dlg( factory(), this );
    connect(&dlg, SIGNAL( newToolbarConfig() ), this, SLOT( slotNewToolbarConfig() ));
    dlg.exec();
}

void ArkTopLevelWindow::slotNewToolbarConfig()
{
    createGUI( m_part );
    applyMainWindowSettings( KGlobal::config(), QString::fromLatin1("MainWindow") );
}

void ArkTopLevelWindow::slotConfigureKeyBindings()
{
    KKeyDialog dlg( true, this );

    dlg.insert( actionCollection() );
    dlg.insert( m_part->actionCollection() );

    dlg.configure();
}

void ArkTopLevelWindow::slotPreferences()
{
    m_widget->options_dirs();
}

void ArkTopLevelWindow::slotSaveOptions()
{
    m_widget->options_saveNow();
}

void ArkTopLevelWindow::slotArchivePopup( const QPoint &pPoint)
{
    static_cast<KPopupMenu *>(factory()->container("archive_popup", this))->popup(pPoint);
}

void ArkTopLevelWindow::toggleStatusBar()
{
    if ( statusbarAction->isChecked() )
    {
        statusBar()->show();
    }
    else
    {
        statusBar()->hide();
    }
}

// see if the ark is already open in another window
bool ArkTopLevelWindow::arkAlreadyOpen( const KURL & url )
{
    if ( m_part->url() == url ) return true;
    if (ArkApplication::getInstance()->isArkOpenAlready(url))
    {
        // raise the window containing the already open archive
        ArkApplication::getInstance()->raiseArk(url);

        // close this window
        window_close();

        // notify the user what's going on
        KMessageBox::information(0, i18n("The archive %1 is already open and has been raised.\nNote: if the filename does not match, it only means that one of the two is a symbolic link.").arg(url.prettyURL()));
        return true;
    }
    return false;
}


void ArkTopLevelWindow::openURL( const KURL & url )
{
    if( !arkAlreadyOpen( url ) )
        m_part->openURL( url );
}

void ArkTopLevelWindow::file_open()
{
    KURL url;
    url = KFileDialog::getOpenURL(m_widget->settings()->getOpenDir(), m_widget->settings()->getFilter(), this);

    if( !arkAlreadyOpen( url ) )
        m_part->openURL( url );
}

void ArkTopLevelWindow::file_close()
{
    m_widget->file_close();
}

void ArkTopLevelWindow::window_close()
{
    file_close();
    slotSaveProperties();
    close();
}

bool ArkTopLevelWindow::queryClose()
{
    window_close();
    return true;
}

void ArkTopLevelWindow::file_quit()
{
    window_close();
}


void ArkTopLevelWindow::setupStatusBar()
{
    kdDebug(1601) << "+ArkWidget::setupStatusBar" << endl;

    KStatusBar *sb = statusBar();

    QWhatsThis::add
        (sb, i18n("The statusbar shows you how many files you have and how many you have selected. It also shows you total sizes for these groups of files."));

    m_pStatusLabelSelect = new QLabel(sb);
    m_pStatusLabelSelect->setFrameStyle(QFrame::Panel | QFrame::Raised);
    m_pStatusLabelSelect->setAlignment(AlignLeft);
    m_pStatusLabelSelect->setText(i18n("0 files selected"));

    m_pStatusLabelTotal = new QLabel(sb);
    m_pStatusLabelTotal->setFrameStyle(QFrame::Panel | QFrame::Raised);
    m_pStatusLabelTotal->setAlignment(AlignRight);
    m_pStatusLabelTotal->setText(i18n("Total: 0 files"));

    sb->addWidget(m_pStatusLabelSelect, 3000);
    sb->addWidget(m_pStatusLabelTotal, 3000);

    kdDebug(1601) << "-ArkWidget::setupStatusBar" << endl;
}

void ArkTopLevelWindow::slotSetStatusBarText( const QString & text )
{
    m_pStatusLabelTotal->setText( text );
}

void ArkTopLevelWindow::slotSetStatusBarSelectedFiles( const QString & text )
{
    m_pStatusLabelSelect->setText( text );
}

void ArkTopLevelWindow::slotSaveProperties()
{
    KConfig *kc = m_widget->settings()->getKConfig();
    recent->saveEntries(kc);

    m_widget->settings()->writeConfiguration();

    kdDebug(1601) << "-saveProperties (exit)" << endl;
}

void ArkTopLevelWindow::saveProperties( KConfig* config )
{
    //TODO: make it work for URLS
    config->writeEntry( "SMOpenedFile",m_widget->getArchName() );
    config->sync();
    kdDebug(1601) << "ArkWidget::saveProperties( KConfig* config )" << endl;
}


void ArkTopLevelWindow::readProperties( KConfig* config )
{
    QString file = config->readEntry("SMOpenedFile");
    kdDebug(1601) << "ArkWidget::readProperties( KConfig* config ) file=" << file << endl;
    if ( !file.isEmpty() )
        openURL( file );
}

void ArkTopLevelWindow::slotAddRecentURL( const QString & url )
{
    recent->addURL( url );
    KConfig *kc = m_widget->settings()->getKConfig();
    recent->saveEntries(kc);
    kdDebug( 1601 ) << "RecentURL: " << url << " added." << endl;
}

void ArkTopLevelWindow::slotRemoveRecentURL( const QString & url )
{
    recent->removeURL( url );
    KConfig *kc = m_widget->settings()->getKConfig();
    recent->saveEntries(kc);
}

void ArkTopLevelWindow::slotAddOpenArk( const KURL & _arkname )
{
    ArkApplication::getInstance()->addOpenArk( _arkname, this );
}

void ArkTopLevelWindow::slotRemoveOpenArk( const KURL & _arkname )
{
    ArkApplication::getInstance()->removeOpenArk( _arkname );
}

void ArkTopLevelWindow::setExtractOnly ( bool b )
{
    m_widget->setExtractOnly(  b );
}

#include "arktoplevelwindow.moc"

