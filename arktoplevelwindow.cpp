/*

 ark -- archiver for the KDE project

 Copyright (C) 2002-2003: Georg Robbers <Georg.Robbers@urz.uni-hd.de>
 Copyright (C) 2003: Helio Chissini de Castro <helio@conectiva.com>

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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// QT includes
#include <qlayout.h>

// KDE includes
#include <kdebug.h>
#include <klocale.h>
#include <kedittoolbar.h>
#include <kstatusbar.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kparts/componentfactory.h>
#include <kparts/browserextension.h>
#include <kkeydialog.h>
#include <kcombobox.h>
#include <kio/netaccess.h>
#include <kaccel.h>

// ark includes
#include "arkapp.h"
#include "arksettings.h"
#include "archiveformatinfo.h"

ArkTopLevelWindow::ArkTopLevelWindow( QWidget * /*parent*/, const char *name ) :
        KParts::MainWindow()
{
    setXMLFile( "arkui.rc" );
    m_part = KParts::ComponentFactory::createPartInstanceFromLibrary<KParts::ReadWritePart>( "libarkpart", this, name, this, "ArkPart");
    if (m_part )
    {
        //Since most of the functionality is still in ArkWidget:
        m_widget = static_cast< ArkWidget* >( m_part->widget() );

        setStandardToolBarMenuEnabled( true );
        setupActions();

        connect( m_part->widget(), SIGNAL( request_file_quit() ), this, SLOT(  file_quit() ) );
        connect( KParts::BrowserExtension::childObject( m_part ), SIGNAL( openURLRequestDelayed
                                              ( const KURL &, const KParts::URLArgs & ) ),
                 m_part, SLOT( openURL( const KURL & ) ) );

        m_widget->setArchivePopupEnabled( true );
        connect( m_part->widget(), SIGNAL( signalArchivePopup( const QPoint & ) ), this,
                 SLOT( slotArchivePopup( const QPoint & ) ) );

        connect( m_part, SIGNAL( removeRecentURL( const KURL & ) ), this,
                 SLOT( slotRemoveRecentURL( const KURL & ) ) );
        connect( m_part, SIGNAL( addRecentURL( const KURL & ) ), this,
                 SLOT( slotAddRecentURL( const KURL & ) ) );
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

        if ( !initialGeometrySet() )
        {
            resize( 640, 300 );
        }
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

void
ArkTopLevelWindow::setupActions()
{
    newWindowAction = new KAction(i18n("New &Window"), "window_new", KShortcut(), this,
                                  SLOT(file_newWindow()), actionCollection(), "new_window");

    newArchAction = KStdAction::openNew(this, SLOT(file_new()), actionCollection());
    openAction = KStdAction::open(this, SLOT(file_open()), actionCollection());

    reloadAction = new KAction(i18n("Re&load"), "reload", KStdAccel::shortcut( KStdAccel::Reload ), this,
                               SLOT(file_reload()), actionCollection(), "reload_arch");
    closeAction = KStdAction::close(this, SLOT(file_close()), actionCollection(), "file_close");

    recent = KStdAction::openRecent(this, SLOT(openURL(const KURL&)), actionCollection());
    KConfig *kc = m_widget->settings()->getKConfig();
    recent->loadEntries(kc);

    createStandardStatusBarAction();

    KStdAction::quit(this, SLOT(window_close()), actionCollection());
    KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());
    KStdAction::keyBindings(this, SLOT( slotConfigureKeyBindings()), actionCollection());

    openAction->setEnabled( true );
    recent->setEnabled( true );
    closeAction->setEnabled( false );
    reloadAction->setEnabled( false );
}

void
ArkTopLevelWindow::slotDisableActions()
{
    openAction->setEnabled(false);
    newArchAction->setEnabled(false);
    closeAction->setEnabled(false);
    reloadAction->setEnabled(false);
}

void
ArkTopLevelWindow::slotFixActionState( const bool & bHaveFiles )
{
    openAction->setEnabled(true);
    newArchAction->setEnabled(true);
    closeAction->setEnabled(bHaveFiles);
    reloadAction->setEnabled(bHaveFiles);
}

void
ArkTopLevelWindow::file_newWindow()
{
    ArkTopLevelWindow *kw = new ArkTopLevelWindow;
    kw->resize( 640, 300 );
    kw->show();
}

void
ArkTopLevelWindow::file_new()
{
    m_widget->file_new();
}

void
ArkTopLevelWindow::file_reload()
{
    KURL url( m_part->url() );
    file_close();
    m_part->openURL( url );
}

void
ArkTopLevelWindow::editToolbars()
{
    saveMainWindowSettings( KGlobal::config(), QString::fromLatin1("MainWindow") );
    KEditToolbar dlg( factory(), this );
    connect(&dlg, SIGNAL( newToolbarConfig() ), this, SLOT( slotNewToolbarConfig() ));
    dlg.exec();
}

void
ArkTopLevelWindow::slotNewToolbarConfig()
{
    createGUI( m_part );
    applyMainWindowSettings( KGlobal::config(), QString::fromLatin1("MainWindow") );
}

void
ArkTopLevelWindow::slotConfigureKeyBindings()
{
    KKeyDialog dlg( true, this );

    dlg.insert( actionCollection() );
    dlg.insert( m_part->actionCollection() );

    dlg.configure();
}

void
ArkTopLevelWindow::slotArchivePopup( const QPoint &pPoint)
{
    static_cast<KPopupMenu *>(factory()->container("archive_popup", this))->popup(pPoint);
}

// see if the ark is already open in another window
bool
ArkTopLevelWindow::arkAlreadyOpen( const KURL & url )
{
    if (ArkApplication::getInstance()->isArkOpenAlready(url))
    {
        if ( m_part->url() == url ) return true;
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


void
ArkTopLevelWindow::openURL( const KURL & url )
{
    if( !arkAlreadyOpen( url ) )
        m_part->openURL( url );
}

KURL
ArkTopLevelWindow::getOpenURL( bool addOnly, const QString & caption,
                               const QString & startDir, const QString & suggestedName )
{
    kdDebug( 1601 ) << "startDir is: " << startDir << endl;
    QWidget * forceFormatWidget = new QWidget( this );
    QHBoxLayout * l = new QHBoxLayout( forceFormatWidget );

    QLabel * label = new QLabel( forceFormatWidget );
    label->setText( i18n( "Open &as:" ) );
    label->adjustSize();

    KComboBox * combo = new KComboBox( forceFormatWidget );

    QStringList list;
    list = ArchiveFormatInfo::self()->allDescriptions();
    list.sort();
    list.prepend( i18n( "Autodetect (default)" ) );
    combo->insertStringList( list );

    QString filter = ArchiveFormatInfo::self()->filter();
    if ( !suggestedName.isEmpty() )
    {
        filter = QString::null;
        combo->setCurrentItem( list.findIndex( ArchiveFormatInfo::self()->descriptionForMimeType(
                                 KMimeType::findByPath( suggestedName, 0, true )->name() ) ) );
    }

    label->setBuddy( combo );

    l->addWidget( label );
    l->addWidget( combo, 1 );

    QString dir;
    if ( addOnly )
        dir = startDir;
    else
        dir = m_widget->settings()->getOpenDir();

    KFileDialog dlg( dir, filter, this, "filedialog", true, forceFormatWidget );
    dlg.setOperationMode( addOnly ? KFileDialog::Saving
                                  : KFileDialog::Opening );

    dlg.setCaption( addOnly ? caption : i18n("Open") );
    dlg.setMode( addOnly ? ( KFile::File | KFile::ExistingOnly )
                                  :  KFile::File );
    if ( !suggestedName.isEmpty() )
        dlg.setSelection( dir + suggestedName );
    else
        dlg.setSelection( dir );

    dlg.exec();

    KURL url;
    url = dlg.selectedURL();

    if ( combo->currentItem() !=0 ) // i.e. != "Autodetect"
        m_widget->setOpenAsMimeType(
            ArchiveFormatInfo::self()->mimeTypeForDescription( combo->currentText() ) );
    else
        m_widget->setOpenAsMimeType( QString::null );

    return url;
}

void
ArkTopLevelWindow::file_open()
{
    KURL url = getOpenURL();
    if( !arkAlreadyOpen( url ) )
        m_part->openURL( url );
}

void
ArkTopLevelWindow::file_close()
{
    m_part->closeURL();
}

void
ArkTopLevelWindow::window_close()
{
    file_close();
    slotSaveProperties();
    //kdDebug(1601) << "-ArkWidget::window_close" << endl;
    close();
}

bool
ArkTopLevelWindow::queryClose()
{
    window_close();
    return true;
}

void
ArkTopLevelWindow::file_quit()
{
    window_close();
}

void
ArkTopLevelWindow::slotSaveProperties()
{
    KConfig *kc = m_widget->settings()->getKConfig();
    recent->saveEntries(kc);

}

void
ArkTopLevelWindow::saveProperties( KConfig* config )
{
    //TODO: make it work for URLS
    config->writePathEntry( "SMOpenedFile",m_widget->getArchName() );
    config->sync();
}


void
ArkTopLevelWindow::readProperties( KConfig* config )
{
    QString file = config->readPathEntry("SMOpenedFile");
    kdDebug(1601) << "ArkWidget::readProperties( KConfig* config ) file=" << file << endl;
    if ( !file.isEmpty() )
        openURL( file );
}

void
ArkTopLevelWindow::slotAddRecentURL( const KURL & url )
{
    recent->addURL( url );
    KConfig *kc = m_widget->settings()->getKConfig();
    recent->saveEntries(kc);
}

void
ArkTopLevelWindow::slotRemoveRecentURL( const KURL & url )
{
    recent->removeURL( url );
    KConfig *kc = m_widget->settings()->getKConfig();
    recent->saveEntries(kc);
}

void
ArkTopLevelWindow::slotAddOpenArk( const KURL & _arkname )
{
    ArkApplication::getInstance()->addOpenArk( _arkname, this );
}

void
ArkTopLevelWindow::slotRemoveOpenArk( const KURL & _arkname )
{
    ArkApplication::getInstance()->removeOpenArk( _arkname );
}

void
ArkTopLevelWindow::setExtractOnly ( bool b )
{
    m_widget->setExtractOnly(  b );
}

void
ArkTopLevelWindow::extractTo( const KURL & targetDirectory, const KURL & archive, bool guessName )
{
    m_widget->extractTo( targetDirectory, archive, guessName );
    m_part->openURL( archive );
}

void
ArkTopLevelWindow::addToArchive( const KURL::List & filesToAdd, const QString & /*cwd*/,
                                 const KURL & archive, bool askForName )
{
    KURL archiveFile;
    if ( askForName || archive.isEmpty() )
    {
        // user definded actions in servicemus are being started by konq
        // from konqis working directory, not from the one being shown when
        // the popupmenu was requested; work around that so the user
        // sees a list of the archives in the diretory he is looking at.
        // makes it show the 'wrong' dir when being called from the commandline
        // like: /dira> ark -add /dirb/file, but well...
        KURL cwdURL;
        cwdURL.setPath( filesToAdd.first().path() );
        QString dir = cwdURL.directory( false );

        archiveFile = getOpenURL( true, i18n( "Select Archive to Add Files To" ),
                                  dir /*cwd*/, archive.fileName() );
    }
    else
        archiveFile = archive;

    if ( archiveFile.isEmpty() )
    {
        kdDebug( 1601 ) << "no archive selected." << endl;
        file_quit();
        return;
    }

    bool exists = KIO::NetAccess::exists( archiveFile, false, m_widget );
    kdDebug( 1601 ) << archiveFile << endl;
    m_widget->addToArchive( filesToAdd, archiveFile );
    if ( exists )
        m_part->openURL( archiveFile );
}

#include "arktoplevelwindow.moc"

