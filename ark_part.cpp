/*
  Copyright (C)
 
  2001: Macadamian Technologies Inc (author: Jian Huang, jian@macadamian.com)
  2003: Georg Robbers <Georg.Robbers@urz.uni-hd.de>
 
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

#include "ark_part.h"
#include "arksettings.h"
#include "arkfactory.h"

#include <kdebug.h>
#include <kpopupmenu.h>
#include <kmessagebox.h>
#include <kaboutdata.h>
#include <kxmlguifactory.h>
#include <kstatusbar.h>

#include <qfile.h>
#include <qtimer.h>

KAboutData *ArkPart::createAboutData()
{
    KAboutData *about = new KAboutData("ark", I18N_NOOP("ark"),
                                       "1.0",
                                       I18N_NOOP("Ark KParts Component"),
                                       KAboutData::License_GPL,
                                       I18N_NOOP( "(c) 1997-2003, The Various Ark Developers" ));
    about->addAuthor("Robert Palmbos",0, "palm9744@kettering.edu");
    about->addAuthor("Francois-Xavier Duranceau",0, "duranceau@kde.org");
    about->addAuthor("Corel Corporation (author: Emily Ezust)",0,
                     "emilye@corel.com");
    about->addAuthor("Corel Corporation (author: Michael Jarrett)", 0,
                     "michaelj@corel.com");
    about->addAuthor("Jian Huang");
    about->addAuthor( "Roberto Teixeira", 0, "maragato@kde.org" );

    return about;
}



ArkPart::ArkPart( QWidget *parentWidget, const char * /*widgetName*/, QObject *parent,
                  const char *name, const QStringList &, bool readWrite )
        : KParts::ReadWritePart(parent, name)
{
    kdDebug()<<"ArkPart::ArkPart"<<endl;
    setInstance(ArkFactory::instance());
    awidget = new  ArkWidget( parentWidget, "ArkWidget" );

    setWidget(awidget);
    connect( awidget, SIGNAL( fixActions() ), this, SLOT( fixEnables() ) );
    connect( awidget, SIGNAL( disableAllActions() ), this, SLOT( disableActions() ) );
    connect( awidget, SIGNAL( signalFilePopup( const QPoint& ) ), this, SLOT( slotFilePopup( const QPoint& ) ) );
    connect( awidget, SIGNAL( setWindowCaption( const QString & ) ), this, SIGNAL( setWindowCaption( const QString & ) ) );
    connect( awidget, SIGNAL( removeRecentURL( const KURL & ) ), this, SIGNAL( removeRecentURL( const KURL & ) ) );
    connect( awidget, SIGNAL( addRecentURL( const KURL & ) ), this, SIGNAL( addRecentURL( const KURL & ) ) );

    if( readWrite )
        setXMLFile( "ark_part.rc" );
    else
    {
        setXMLFile( "ark_part_readonly.rc" );
    }
    setReadWrite( readWrite );

    setupActions();

    m_ext = new ArkBrowserExtension( this, "ArkBrowserExtension" );
    connect( awidget, SIGNAL( openURLRequest( const KURL & ) ),
             m_ext, SLOT( slotOpenURLRequested( const KURL & ) ) );

    m_bar = new ArkStatusBarExtension( this );
    connect( awidget, SIGNAL( setStatusBarText( const QString & ) ), m_bar,
                 SLOT( slotSetStatusBarText( const QString & ) ) );
    connect( awidget, SIGNAL( setStatusBarSelectedFiles( const QString & ) ), m_bar,
                 SLOT( slotSetStatusBarSelectedFiles( const QString & ) ) );
    connect( awidget, SIGNAL( setBusy( const QString & ) ), m_bar,
                 SLOT( slotSetBusy( const QString & ) ) );
    connect( awidget, SIGNAL( setReady() ), m_bar,
                 SLOT( slotSetReady() ) );
}

ArkPart::~ArkPart()
{}

void
ArkPart::setupActions()
{
    shellOutputAction  = new KAction(i18n("&View Shell Output"), 0, awidget,
                                     SLOT(edit_view_last_shell_output()), actionCollection(), "shell_output");

    addFileAction = new KAction(i18n("Add &File..."), "ark_addfile", 0, awidget,
                                SLOT(action_add()), actionCollection(), "addfile");

    addDirAction = new KAction(i18n("Add &Directory..."), "ark_adddir", 0, awidget,
                               SLOT(action_add_dir()), actionCollection(), "adddir");

    extractAction = new KAction(i18n("E&xtract..."), "ark_extract", 0, awidget,
                                SLOT(action_extract()),	actionCollection(), "extract");

    deleteAction = new KAction(i18n("De&lete"), "ark_delete", 0, awidget,
                               SLOT(action_delete()), actionCollection(), "delete");

    viewAction = new KAction(i18n("to view something","&View"), "ark_view", 0, awidget,
                             SLOT(action_view()), actionCollection(), "view");


    openWithAction = new KAction(i18n("&Open With..."), 0, awidget,
                                 SLOT(slotOpenWith()), actionCollection(), "open_with");


    editAction = new KAction(i18n("Edit &With..."), 0, awidget,
                             SLOT(action_edit()), actionCollection(), "edit");

    selectAction =  new KAction(i18n("&Select..."), 0, awidget,
                                SLOT(edit_select()),	actionCollection(), "select");

    selectAllAction = KStdAction::selectAll(awidget,
                                            SLOT(edit_selectAll()), actionCollection(), "select_all");

    deselectAllAction =  new KAction(i18n("&Deselect All"), 0, awidget,
                                     SLOT(edit_deselectAll()), actionCollection(), "deselect_all");

    invertSelectionAction = new KAction(i18n("&Invert Selection"), 0, awidget,
                                        SLOT(edit_invertSel()), actionCollection(), "invert_selection");

    saveAsAction = KStdAction::saveAs(this, SLOT(file_save_as()), actionCollection());
    ( void ) new KAction( i18n( "Configure &Ark..." ), "configure" , 0, awidget,
                                        SLOT( options_dirs() ), actionCollection(), "options_configure_ark" );

    initialEnables();
}


void ArkPart::fixEnables()
{
    bool bHaveFiles = ( awidget->getNumFilesInArchive() > 0 );
    bool bReadOnly = false;
    bool bAddDirSupported = true;
    QString extension;
    if ( awidget->archiveType() == ZOO_FORMAT || awidget->archiveType() == AA_FORMAT
            || awidget->archiveType() == COMPRESSED_FORMAT)
        bAddDirSupported = false;

    if (awidget->archive())
        bReadOnly = awidget->archive()->isReadOnly();

    saveAsAction->setEnabled(bHaveFiles);
    selectAction->setEnabled(bHaveFiles);
    selectAllAction->setEnabled(bHaveFiles);
    deselectAllAction->setEnabled(bHaveFiles);
    invertSelectionAction->setEnabled(bHaveFiles);

    deleteAction->setEnabled(bHaveFiles && awidget->numSelectedFiles() > 0
                             && awidget->archive() && !bReadOnly);
    addFileAction->setEnabled(awidget->isArchiveOpen() &&
                              !bReadOnly);
    addDirAction->setEnabled(awidget->isArchiveOpen() &&
                             !bReadOnly && bAddDirSupported);
    extractAction->setEnabled(bHaveFiles);

    bool b = ( bHaveFiles && awidget->numSelectedFiles() == 1 );
    viewAction->setEnabled( b );
    openWithAction->setEnabled( b );
    editAction->setEnabled( b );
    emit fixActionState( bHaveFiles );
}

void ArkPart::initialEnables()
{
    saveAsAction->setEnabled( false );
    selectAction->setEnabled(false);
    selectAllAction->setEnabled(false);
    deselectAllAction->setEnabled(false);
    invertSelectionAction->setEnabled(false);

    viewAction->setEnabled(false);

    deleteAction->setEnabled(false);
    extractAction->setEnabled(false);
    addFileAction->setEnabled(false);
    addDirAction->setEnabled(false);
    openWithAction->setEnabled(false);
    editAction->setEnabled(false);
}

void ArkPart::disableActions()
{
    saveAsAction->setEnabled(false);
    selectAction->setEnabled(false);
    selectAllAction->setEnabled(false);
    deselectAllAction->setEnabled(false);
    invertSelectionAction->setEnabled(false);

    viewAction->setEnabled(false);
    deleteAction->setEnabled(false);
    extractAction->setEnabled(false);
    addFileAction->setEnabled(false);
    addDirAction->setEnabled(false);
    openWithAction->setEnabled(false);
    editAction->setEnabled(false);

}

bool ArkPart::openURL( const KURL & url )
{
    awidget->setRealURL( url );
    return KParts::ReadWritePart::openURL( url );
}

bool ArkPart::openFile()
{
    KURL url;
    url.setPath( m_file );
    if( !QFile::exists( m_file ) )
    {
        emit setWindowCaption(  QString::null );
        emit removeRecentURL( awidget->realURL() );
        return false;
    }
    emit addRecentURL( awidget->realURL() );
    awidget->setModified( false );
    awidget->file_open( url );
    return true;
}

void ArkPart::file_save_as()
{
    KURL u = awidget->getSaveAsFileName();
    if ( u.isEmpty() ) // user canceled
        return;

    if ( !awidget->allowedArchiveName( u ) )
        awidget->convertTo( u );
    else if ( awidget->file_save_as( u ) )
        m_ext->slotOpenURLRequested( u );
    else
        kdWarning( 1601 ) <<  "Save As failed." << endl;
}

bool ArkPart::saveFile()
{
    return true;
}

bool ArkPart::closeArchive()
{
    awidget->file_close();
    awidget->setModified( false );
    return ReadWritePart::closeURL();
}

bool ArkPart::closeURL()
{
  if ( !isReadWrite() || !awidget->isModified() || url().isLocalFile() )
    return closeArchive();

  QString docName = url().prettyURL();

  int res = KMessageBox::warningYesNoCancel( widget(),
          i18n( "The archive \"%1\" has been modified.\n"
                "Do you want to save it?" ).arg( docName ),
          i18n( "Save Archive?" ), KStdGuiItem::save(), KStdGuiItem::discard() );

  switch ( res )
  {
    case KMessageBox::Yes :
        return awidget->file_save_as( url() ) && closeArchive();

    case KMessageBox::No :
        return closeArchive();

    default : // case KMessageBox::Cancel
        return false;
  }
}

void ArkPart::slotFilePopup( const QPoint &pPoint )
{
    static_cast<KPopupMenu *>(factory()->container("file_popup", this))->popup(pPoint);
}

void ArkPart::slotSaveProperties()
{
    awidget->settings()->writeConfiguration();

    kdDebug(1601) << "-saveProperties (exit)" << endl;
}

ArkBrowserExtension::ArkBrowserExtension( KParts::ReadOnlyPart * parent, const char * name )
                : KParts::BrowserExtension( parent, name )
{
}

void ArkBrowserExtension::slotOpenURLRequested( const KURL & url )
{
    emit openURLRequest( url, KParts::URLArgs() );
}

ArkStatusBarExtension::ArkStatusBarExtension( KParts::ReadWritePart * parent )
                : KParts::StatusBarExtension( parent ),
                  m_bBusy( false ), m_pTimer( NULL )
{
    m_pProgressBar = NULL;
    m_pBusyText = NULL;
    m_pStatusLabelTotal = NULL;
    m_pStatusLabelSelect = NULL;
}

ArkStatusBarExtension::~ArkStatusBarExtension()
{
    if ( m_bBusy )
    {
        removeStatusBarItem( m_pBusyText );
        removeStatusBarItem( m_pProgressBar );
    }
    else if ( m_pStatusLabelSelect )
    {
        removeStatusBarItem( m_pStatusLabelSelect );
        removeStatusBarItem( m_pStatusLabelTotal );
    }
}

void ArkStatusBarExtension::setupStatusBar()
{
    if ( m_pTimer ) // setup already done
        return;
    m_pTimer = new QTimer( this );
    connect( m_pTimer, SIGNAL( timeout() ), this, SLOT( slotProgress() ) );

    m_pStatusLabelTotal = new QLabel( statusBar() );
    m_pStatusLabelTotal->setFrameStyle( QFrame::Panel | QFrame::Raised );
    m_pStatusLabelTotal->setAlignment( AlignRight );
    m_pStatusLabelTotal->setText( i18n( "Total: 0 files" ) );

    m_pStatusLabelSelect = new QLabel( statusBar() );
    m_pStatusLabelSelect->setFrameStyle( QFrame::Panel | QFrame::Raised );
    m_pStatusLabelSelect->setAlignment( AlignLeft );
    m_pStatusLabelSelect->setText(i18n( "0 files selected" ) );

    addStatusBarItem( m_pStatusLabelSelect, 3000, false );
    addStatusBarItem( m_pStatusLabelTotal, 3000, false );
}

void ArkStatusBarExtension::slotSetStatusBarText( const QString & text )
{
    setupStatusBar();
    m_pStatusLabelTotal->setText( text );
}

void ArkStatusBarExtension::slotSetStatusBarSelectedFiles( const QString & text )
{
    setupStatusBar();
    m_pStatusLabelSelect->setText( text );
}

void ArkStatusBarExtension::slotSetBusy( const QString & text )
{
    if ( m_bBusy )
        return;

    setupStatusBar();
    if ( !m_pBusyText )
    {
        m_pBusyText = new QLabel( statusBar() );

        m_pBusyText->setAlignment( AlignLeft );
        m_pBusyText->setFrameStyle( QFrame::Panel | QFrame::Raised );
    }

    if ( !m_pProgressBar )
    {
        m_pProgressBar = new KProgress( statusBar() );

        m_pProgressBar->setTotalSteps( 0 );
        m_pProgressBar->setPercentageVisible( false );
        m_pProgressBar->setFixedHeight( m_pBusyText->fontMetrics().height() );
    }
    m_pBusyText->setText( text );

    removeStatusBarItem( m_pStatusLabelSelect );
    removeStatusBarItem( m_pStatusLabelTotal );

    addStatusBarItem( m_pBusyText, 5, true );
    addStatusBarItem( m_pProgressBar, 1, true );

    m_pTimer->start( 200, FALSE );
    m_bBusy = true;
}

void ArkStatusBarExtension::slotSetReady()
{
    if ( !m_bBusy )
        return;

    setupStatusBar();
    m_pTimer->stop();
    m_pProgressBar->setProgress( 0 );

    removeStatusBarItem( m_pBusyText );
    removeStatusBarItem( m_pProgressBar );

    addStatusBarItem( m_pStatusLabelSelect, 3000, false );
    addStatusBarItem( m_pStatusLabelTotal, 3000, false );

    m_bBusy = false;
}

void ArkStatusBarExtension::slotProgress()
{
    setupStatusBar();
    m_pProgressBar->setProgress( m_pProgressBar->progress() + 4 );
}

#include "ark_part.moc"
