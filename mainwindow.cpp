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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

// QT includes
#include <QLayout>
#include <QLabel>
#include <QHBoxLayout>

// KDE includes
#include <kdebug.h>
#include <klocale.h>
#include <kedittoolbar.h>
#include <kstatusbar.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kmenu.h>
#include <kparts/componentfactory.h>
#include <kparts/browserextension.h>
#include <kshortcutsdialog.h>
#include <kcombobox.h>
#include <kio/netaccess.h>
#include <kxmlguifactory.h>
#include <kglobal.h>
#include <kprogressdialog.h>
#include <kstandardshortcut.h>
#include <kstandardaction.h>
#include <kicon.h>
#include <kactioncollection.h>
// ark includes
#include "arkapp.h"
#include "settings.h"
#include "archiveformatinfo.h"
#include "arkwidget.h"

MainWindow::MainWindow( QWidget * /*parent*/, const char *name )
	: KParts::MainWindow(), progressDialog( 0 )
{
    setXMLFile( "arkui.rc" );
    m_part = KParts::ComponentFactory::createPartInstanceFromLibrary<KParts::ReadWritePart>( "libarkpart", this, this);
    if (m_part )
    {
        m_part->setObjectName("ArkPart");
        //Since most of the functionality is still in ArkWidget:
        m_widget = static_cast< ArkWidget* >( m_part->widget() );
 	m_widget->setObjectName(name);

        setStandardToolBarMenuEnabled( true );
        setupActions();

        connect( m_part->widget(), SIGNAL( request_file_quit() ), this, SLOT(  file_quit() ) );
        connect( KParts::BrowserExtension::childObject( m_part ), SIGNAL( openUrlRequestDelayed
                                              ( const KUrl &, const KParts::URLArgs & ) ),
                 m_part, SLOT( openURL( const KUrl & ) ) );

        m_widget->setArchivePopupEnabled( true );
        connect( m_part->widget(), SIGNAL( signalArchivePopup( const QPoint & ) ), this,
                 SLOT( slotArchivePopup( const QPoint & ) ) );

        connect( m_part, SIGNAL( removeRecentURL( const KUrl & ) ), this,
                 SLOT( slotRemoveRecentURL( const KUrl & ) ) );
        connect( m_part, SIGNAL( addRecentURL( const KUrl & ) ), this,
                 SLOT( slotAddRecentURL( const KUrl & ) ) );
        connect( m_part, SIGNAL( fixActionState( const bool & ) ), this,
                 SLOT( slotFixActionState( const bool & ) ) );
        connect( m_widget, SIGNAL( disableAllActions() ), this,
                 SLOT( slotDisableActions() ) );

        ArkApplication::getInstance()->addWindow();
        connect( m_widget, SIGNAL( removeOpenArk( const  KUrl &) ), this,
                 SLOT( slotRemoveOpenArk( const KUrl & ) ) );
        connect( m_widget, SIGNAL( addOpenArk( const  KUrl & ) ), this,
                 SLOT( slotAddOpenArk( const KUrl & ) ) );

        setCentralWidget( m_part->widget() );
        createGUI( m_part );

        if ( !initialGeometrySet() )
        {
            resize( 640, 300 );
        }
        setAutoSaveSettings( "MainWindow" );
    }
    else
        kFatal( 1601 ) << "libark could not found. Aborting. " << endl;

}



MainWindow::~MainWindow()
{
    ArkApplication::getInstance()->removeWindow();
    delete m_part;
    delete progressDialog;
    progressDialog = 0;
}

void
MainWindow::setupActions()
{
    newWindowAction = actionCollection()->addAction("new_window");
    newWindowAction->setText(i18n("New &Window"));
    newWindowAction->setIcon(KIcon("window-new"));

    connect(newWindowAction, SIGNAL(triggered(bool)), SLOT(file_newWindow()));

    newArchAction = KStandardAction::openNew(this, SLOT(file_new()), actionCollection());
    openAction = KStandardAction::open(this, SLOT(file_open()), actionCollection());

    reloadAction = actionCollection()->addAction("reload_arch");
    reloadAction->setIcon(KIcon("view-refresh"));
    reloadAction->setText(i18n("Re&load"));
    connect(reloadAction, SIGNAL(triggered(bool)), SLOT(file_reload()));
    reloadAction->setShortcuts(KStandardShortcut::shortcut( KStandardShortcut::Reload ));

    closeAction = KStandardAction::close(this, SLOT(file_close()), actionCollection());
    actionCollection()->addAction("file_close",closeAction);


    recent = KStandardAction::openRecent(this, SLOT(openURL(const KUrl&)), actionCollection());
    recent->loadEntries(KGlobal::config()->group( QString() ));

    createStandardStatusBarAction();

    KStandardAction::quit(this, SLOT(window_close()), actionCollection());
    KStandardAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());
    KStandardAction::keyBindings(this, SLOT( slotConfigureKeyBindings()), actionCollection());

    openAction->setEnabled( true );
    recent->setEnabled( true );
    closeAction->setEnabled( false );
    reloadAction->setEnabled( false );
}

void
MainWindow::slotDisableActions()
{
    openAction->setEnabled(false);
    newArchAction->setEnabled(false);
    closeAction->setEnabled(false);
    reloadAction->setEnabled(false);
}

void
MainWindow::slotFixActionState( const bool & bHaveFiles )
{
    openAction->setEnabled(true);
    newArchAction->setEnabled(true);
    closeAction->setEnabled(bHaveFiles);
    reloadAction->setEnabled(bHaveFiles);
}

void
MainWindow::file_newWindow()
{
    MainWindow *kw = new MainWindow;
    kw->resize( 640, 300 );
    kw->show();
}

void
MainWindow::file_new()
{
    m_widget->file_new();
}

void
MainWindow::file_reload()
{
    KUrl url( m_part->url() );
    file_close();
    m_part->openUrl( url );
}

void
MainWindow::editToolbars()
{
    saveMainWindowSettings( KGlobal::config()->group( QLatin1String( "MainWindow") ) );
    KEditToolBar dlg( factory(), QString(), this );
    connect(&dlg, SIGNAL( newToolbarConfig() ), this, SLOT( slotNewToolbarConfig() ));
    dlg.exec();
}

void
MainWindow::slotNewToolbarConfig()
{
    createGUI( m_part );
    applyMainWindowSettings( KGlobal::config()->group( QLatin1String("MainWindow") ) );
}

void
MainWindow::slotConfigureKeyBindings()
{
    KShortcutsDialog dlg( KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsAllowed, this );

    dlg.addCollection( actionCollection() );
    dlg.addCollection( m_part->actionCollection() );

    dlg.configure();
}

void
MainWindow::slotArchivePopup( const QPoint &pPoint)
{
    static_cast<KMenu *>(factory()->container("archive_popup", this))->popup(pPoint);
}

// see if the ark is already open in another window
bool
MainWindow::arkAlreadyOpen( const KUrl & url )
{
    if (ArkApplication::getInstance()->isArkOpenAlready(url))
    {
        if ( m_part->url() == url ) return true;
        // raise the window containing the already open archive
        ArkApplication::getInstance()->raiseArk(url);

        // close this window
        window_close();

        // notify the user what's going on
        KMessageBox::information(0, i18n("The archive %1 is already open and has been raised.\nNote: if the filename does not match, it only means that one of the two is a symbolic link.", url.prettyUrl()));
        return true;
    }
    return false;
}


void
MainWindow::openURL( const KUrl & url, bool tempFile )
{
    if( !arkAlreadyOpen( url ) )
    {
        if ( tempFile && url.isLocalFile() )
            m_widget->deleteAfterUse( url.path() );
        m_part->openUrl( url );
    }
}

KUrl
MainWindow::getOpenURL( bool addOnly, const QString & caption,
                               const QString & startDir, const QString & suggestedName )
{
    kDebug( 1601 ) << "startDir is: " << startDir << endl;
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
    combo->addItems( list );

    QString filter = ArchiveFormatInfo::self()->filter();
    if ( !suggestedName.isEmpty() )
    {
        filter = QString::null;
        combo->setCurrentIndex(list.indexOf( ArchiveFormatInfo::self()->descriptionForMimeType(
                                 KMimeType::findByPath( suggestedName, 0, true )->name() ) ) );
    }

    label->setBuddy( combo );

    l->addWidget( label );
    l->addWidget( combo, 1 );

    KUrl dir;
    if ( addOnly )
        dir = startDir;
    else
        dir = "kfiledialog://ArkOpenDir";

    KFileDialog dlg( dir, filter, this, forceFormatWidget );
    dlg.setOperationMode( addOnly ? KFileDialog::Saving
                                  : KFileDialog::Opening );

    dlg.setCaption( addOnly ? caption : i18n("Open") );
    dlg.setMode( addOnly ? ( KFile::File | KFile::ExistingOnly )
                                  :  KFile::File );
    dlg.setSelection( suggestedName );

    dlg.exec();

    KUrl url;
    url = dlg.selectedUrl();

    if ( combo->currentIndex() !=0 ) // i.e. != "Autodetect"
        m_widget->setOpenAsMimeType(
            ArchiveFormatInfo::self()->mimeTypeForDescription( combo->currentText() ) );
    else
        m_widget->setOpenAsMimeType( QString::null );

    return url;
}

void
MainWindow::file_open()
{
    KUrl url = getOpenURL();
    if( !arkAlreadyOpen( url ) )
        m_part->openUrl( url );
}

void
MainWindow::file_close()
{
    m_part->closeUrl();
}

void
MainWindow::window_close()
{
    file_close();
    slotSaveProperties();
    //kDebug(1601) << "-ArkWidget::window_close" << endl;
    close();
}

bool
MainWindow::queryClose()
{
    window_close();
    return true;
}

void
MainWindow::file_quit()
{
    window_close();
}

void
MainWindow::slotSaveProperties()
{
    recent->saveEntries( KGlobal::config()->group( QString() ) );
}

void
MainWindow::saveProperties( KConfig* config )
{
    //TODO: make it work for URLS
    config->writePathEntry( "SMOpenedFile",m_widget->getArchName() );
    config->sync();
}


void
MainWindow::readProperties( KConfig* config )
{
    QString file = config->readPathEntry("SMOpenedFile");
    kDebug(1601) << "ArkWidget::readProperties( KConfig* config ) file=" << file << endl;
    if ( !file.isEmpty() )
        openURL( file );
}

void
MainWindow::slotAddRecentURL( const KUrl & url )
{
    recent->addUrl( url );
    recent->saveEntries( KGlobal::config()->group( QString() ) );
}

void
MainWindow::slotRemoveRecentURL( const KUrl & url )
{
    recent->removeUrl( url );
    recent->saveEntries( KGlobal::config()->group( QString() ) );
}

void
MainWindow::slotAddOpenArk( const KUrl & _arkname )
{
    ArkApplication::getInstance()->addOpenArk( _arkname, this );
}

void
MainWindow::slotRemoveOpenArk( const KUrl & _arkname )
{
    ArkApplication::getInstance()->removeOpenArk( _arkname );
}

void
MainWindow::setExtractOnly ( bool b )
{
    m_widget->setExtractOnly(  b );
}

void
MainWindow::extractTo( const KUrl & targetDirectory, const KUrl & archive, bool guessName )
{
    startProgressDialog( i18n( "Extracting..." ) );
    m_widget->extractTo( targetDirectory, archive, guessName );
    m_part->openUrl( archive );
}

void
MainWindow::addToArchive( const KUrl::List & filesToAdd, const QString & /*cwd*/,
                                 const KUrl & archive, bool askForName )
{
    KUrl archiveFile;
    if ( askForName || archive.isEmpty() )
    {
        // user definded actions in servicemus are being started by konq
        // from konqis working directory, not from the one being shown when
        // the popupmenu was requested; work around that so the user
        // sees a list of the archives in the diretory he is looking at.
        // makes it show the 'wrong' dir when being called from the commandline
        // like: /dira> ark -add /dirb/file, but well...
        KUrl cwdURL;
        cwdURL.setPath( filesToAdd.first().path() );
        QString dir = cwdURL.directory( KUrl::AppendTrailingSlash );

        archiveFile = getOpenURL( true, i18n( "Select Archive to Add Files To" ),
                                  dir /*cwd*/, archive.fileName() );
    }
    else
        archiveFile = archive;

    if ( archiveFile.isEmpty() )
    {
        kDebug( 1601 ) << "no archive selected." << endl;
        file_quit();
        return;
    }

    startProgressDialog( i18n( "Compressing..." ) );

    bool exists = KIO::NetAccess::exists( archiveFile, false, m_widget );
    kDebug( 1601 ) << archiveFile << endl;

    if ( !m_widget->addToArchive( filesToAdd, archiveFile ) )
        file_quit();
    if ( exists )
        m_part->openUrl( archiveFile );
}

void
MainWindow::startProgressDialog( const QString & text )
{
    if ( !progressDialog )
	{
        progressDialog = new KProgressDialog( this, QString::null, text, false );
		progressDialog->setObjectName("progress_dialog");
	}
	else
        progressDialog->setLabel( text );

//    progressDialog->setWFlags( Qt::WType_TopLevel );

    progressDialog->setAllowCancel( true );
    progressDialog->setPlainCaption( i18n( "Please Wait" ) );

    progressDialog->progressBar()->setMaximum( 0 );
    progressDialog->progressBar()->setTextVisible( false );

//    progressDialog->setInitialSize( QSize(200,100), true );
    progressDialog->setMinimumDuration( 500 );
    progressDialog->show();
    KDialog::centerOnScreen( progressDialog );
    connect( progressDialog, SIGNAL( cancelClicked() ), this, SLOT( window_close() ) );

    timer = new QTimer( this );
    connect( timer, SIGNAL( timeout() ), this, SLOT( slotProgress() ) );

    timer->setSingleShot( false );
    timer->start( 200 );
}

void
MainWindow::slotProgress()
{
    progressDialog->progressBar()->setValue( progressDialog->progressBar()->value() + 4 );
}


#include "mainwindow.moc"

