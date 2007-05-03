/*

  Copyright (C) 2001 Macadamian Technologies Inc (author: Jian Huang <jian@macadamian.com>)
  Copyright (C) 2003 Georg Robbers <Georg.Robbers@urz.uni-hd.de>
  Copyright (C) 2005 Henrique Pinto <henrique.pinto@kdemail.net>

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

#include "ark_part.h"
#include "arkfactory.h"
#include "arkwidget.h"
#include "settings.h"
#include "filelistview.h"

#include <K3ListViewSearchLine>

#include <KDebug>
#include <KMenu>
#include <KMessageBox>
#include <KAboutData>
#include <kxmlguifactory.h>
#include <KStatusBar>
#include <KIconLoader>
#include <kio/netaccess.h>
#include <KLocale>
#include <KStandardAction>
#include <KIcon>
#include <KActionCollection>

#include <QFile>
#include <QTimer>
#include <QPushButton>
#include <QLabel>
#include <QFrame>

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



ArkPart::ArkPart( QWidget *parentWidget, QObject *parent,
				  const QStringList &, bool readWrite )
        : KParts::ReadWritePart(parent)
{
    setComponentData(ArkFactory::componentData());
    awidget = new  ArkWidget( parentWidget );

    setWidget(awidget);
    connect( awidget, SIGNAL( fixActions() ), this, SLOT( fixEnables() ) );
    connect( awidget, SIGNAL( disableAllActions() ), this, SLOT( disableActions() ) );
    connect( awidget, SIGNAL( signalFilePopup( const QPoint& ) ), this, SLOT( slotFilePopup( const QPoint& ) ) );
    connect( awidget, SIGNAL( setWindowCaption( const QString & ) ), this, SIGNAL( setWindowCaption( const QString & ) ) );
    connect( awidget, SIGNAL( removeRecentURL( const KUrl & ) ), this, SIGNAL( removeRecentURL( const KUrl & ) ) );
    connect( awidget, SIGNAL( addRecentURL( const KUrl & ) ), this, SIGNAL( addRecentURL( const KUrl & ) ) );

    if( readWrite )
        setXMLFile( "ark_part.rc" );
    else
    {
        setXMLFile( "ark_part_readonly.rc" );
    }
    setReadWrite( readWrite );

    setupActions();

    m_ext = new ArkBrowserExtension( this, "ArkBrowserExtension" );
    connect( awidget, SIGNAL( openUrlRequest( const KUrl & ) ),
             m_ext, SLOT( slotOpenUrlRequested( const KUrl & ) ) );

    m_bar = new ArkStatusBarExtension( this );
    connect( awidget, SIGNAL( setStatusBarText( const QString & ) ), m_bar,
                 SLOT( slotSetStatusBarText( const QString & ) ) );
    connect( awidget, SIGNAL( setStatusBarSelectedFiles( const QString & ) ), m_bar,
                 SLOT( slotSetStatusBarSelectedFiles( const QString & ) ) );
    connect( awidget, SIGNAL( setBusy( const QString & ) ), m_bar,
                 SLOT( slotSetBusy( const QString & ) ) );
    connect( awidget, SIGNAL( setReady() ), m_bar,
                 SLOT( slotSetReady() ) );
    connect( this, SIGNAL( started(KIO::Job*) ), SLOT( transferStarted(KIO::Job*) ) );
    connect( this, SIGNAL( completed() ), SLOT( transferCompleted() ) );
    connect( this, SIGNAL( canceled(const QString&) ),
             SLOT( transferCanceled(const QString&) ) );

    setProgressInfoEnabled( false );
}

ArkPart::~ArkPart()
{}

void
ArkPart::setupActions()
{
    addFileAction = actionCollection()->addAction("addfile");
    addFileAction->setIcon(KIcon("ark-addfile"));
    addFileAction->setText(i18n("Add &File..."));
    connect(addFileAction, SIGNAL(triggered(bool)), awidget, SLOT(action_add()));

    addDirAction = actionCollection()->addAction("adddir");
    addDirAction->setText(i18n("Add Folde&r..."));
    addDirAction->setIcon(KIcon("ark-adddir"));
    connect(addDirAction, SIGNAL(triggered(bool)), awidget, SLOT(action_add_dir()));

    extractAction =actionCollection()->addAction("extract");
    extractAction->setText(i18n("E&xtract..."));
    extractAction->setIcon(KIcon("ark-extract"));
    connect(extractAction, SIGNAL(triggered(bool)), awidget, SLOT(action_extract()));

    deleteAction  = new KAction(KIcon("ark-delete"), i18n("De&lete"), this);
    actionCollection()->addAction("delete", deleteAction );
    connect(deleteAction, SIGNAL(triggered(bool)), awidget, SLOT(action_delete()));

    viewAction = actionCollection()->addAction("view");
    viewAction->setText(i18nc("to view something","&View"));
    viewAction->setIcon(KIcon("ark-view"));
    connect(viewAction, SIGNAL(triggered(bool)), awidget, SLOT(action_view()));

    openWithAction  = new KAction(i18n("&Open With..."), this);
    actionCollection()->addAction("open_with", openWithAction );
    connect(openWithAction, SIGNAL(triggered(bool) ), awidget, SLOT(slotOpenWith()));


    editAction  = new KAction(i18n("Edit &With..."), this);
    actionCollection()->addAction("edit", editAction );
    connect(editAction, SIGNAL(triggered(bool) ), awidget, SLOT(action_edit()));

    selectAllAction = KStandardAction::selectAll(awidget->fileList(), SLOT(selectAll()), actionCollection());
    actionCollection()->addAction("select_all", selectAllAction);

    deselectAllAction  = new KAction(i18n("&Unselect All"), this);
    actionCollection()->addAction("deselect_all", deselectAllAction );
    connect(deselectAllAction, SIGNAL(triggered(bool) ), awidget->fileList(), SLOT(unselectAll()));

    invertSelectionAction  = new KAction(i18n("&Invert Selection"), this);
    actionCollection()->addAction("invert_selection", invertSelectionAction );
    connect(invertSelectionAction, SIGNAL(triggered(bool) ), awidget->fileList(), SLOT(invertSelection()));

    saveAsAction = KStandardAction::saveAs(this, SLOT(file_save_as()), actionCollection());

    //KStandardAction::preferences(awidget, SLOT(showSettings()), actionCollection());

    QAction * action = actionCollection()->addAction("options_configure_ark");
    action->setText(i18n( "Configure &Ark..." ));
    action->setIcon( KIcon("configure"));
    connect(action, SIGNAL(triggered(bool) ), awidget, SLOT( showSettings() ));


    showSearchBar  = new KToggleAction(i18n("Show Search Bar"), this);
    actionCollection()->addAction("options_show_search_bar", showSearchBar );
    showSearchBar->setCheckedState(KGuiItem(i18n("Hide Search Bar")));

    showSearchBar->setChecked( ArkSettings::showSearchBar() );

    connect( showSearchBar, SIGNAL( toggled( bool ) ), awidget, SLOT( slotShowSearchBarToggled( bool ) ) );

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
    awidget->searchBar()->setEnabled(bHaveFiles);

    bool b = ( bHaveFiles
               && (awidget->numSelectedFiles() == 1)
               && (awidget->fileList()->currentItem()->childCount() == 0)
             );
    viewAction->setEnabled( b );
    openWithAction->setEnabled( b );
    editAction->setEnabled( b && !bReadOnly ); // You can't edit files in read-only archives
    emit fixActionState( bHaveFiles );
}

void ArkPart::initialEnables()
{
    saveAsAction->setEnabled( false );
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

    awidget->searchBar()->setEnabled(false);
}

void ArkPart::disableActions()
{
    saveAsAction->setEnabled(false);
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
    awidget->searchBar()->setEnabled(false);
}

bool ArkPart::openURL( const KUrl & url )
{
    awidget->setRealURL( url );
    return KParts::ReadWritePart::openUrl( KIO::NetAccess::mostLocalUrl( url, awidget ) );
}

bool ArkPart::openFile()
{
    KUrl url;
    url.setPath( localFilePath() );
    if( !QFile::exists( localFilePath() ) )
    {
        emit setWindowCaption( QString() );
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
    KUrl u = awidget->getSaveAsFileName();
    if ( u.isEmpty() ) // user canceled
        return;

    if ( !awidget->allowedArchiveName( u ) )
        awidget->convertTo( u );
    else if ( awidget->file_save_as( u ) )
        m_ext->slotOpenUrlRequested( u );
    else
        kWarning( 1601 ) <<  "Save As failed." << endl;
}

bool ArkPart::saveFile()
{
    return true;
}

bool ArkPart::closeArchive()
{
    awidget->file_close();
    awidget->setModified( false );
    return ReadWritePart::closeUrl();
}

bool ArkPart::closeUrl()
{
  if ( !isReadWrite() || !awidget->isModified() || awidget->realURL().isLocalFile() )
    return closeArchive();

  QString docName = awidget->realURL().prettyUrl();

  int res = KMessageBox::warningYesNoCancel( widget(),
          i18n( "The archive \"%1\" has been modified.\n"
                "Do you want to save it?", docName ),
          i18n( "Save Archive?" ), KStandardGuiItem::save(), KStandardGuiItem::discard() );

  switch ( res )
  {
    case KMessageBox::Yes :
        return awidget->file_save_as( awidget->realURL() ) && closeArchive();

    case KMessageBox::No :
        return closeArchive();

    default : // case KMessageBox::Cancel
        return false;
  }
}

void ArkPart::slotFilePopup( const QPoint &pPoint )
{
    if ( factory() )
        static_cast<KMenu *>(factory()->container("file_popup", this))->popup(pPoint);
}

void ArkPart::transferStarted( KIO::Job *job )
{
    m_job = job;

    m_bar->slotSetBusy( i18n( "Downloading %1...", url().prettyUrl() ),
                        (job != 0), (job != 0) );

    if ( job )
    {
        disableActions();
        connect( job, SIGNAL( percent(KJob*, unsigned long) ),
                 SLOT( progressInformation(KJob*, unsigned long) ) );
        connect( m_bar->cancelButton(), SIGNAL( clicked() ),
                 SLOT( cancelTransfer() ) );
    }
}

void ArkPart::transferCompleted()
{
    if ( m_job )
    {
        disconnect( m_job, SIGNAL( percent(KJob*, unsigned long) ),
                    this, SLOT( progressInformation(KJob*, unsigned long) ) );
        m_job = 0;
    }

    m_bar->slotSetReady();
}

void ArkPart::transferCanceled( const QString& errMsg )
{
    m_job = 0;
    if ( !errMsg.isEmpty() )
    {
        KMessageBox::error( awidget, errMsg );
    }
    initialEnables();
    m_bar->slotSetReady();
}

void ArkPart::progressInformation( KJob *, unsigned long progress )
{
    m_bar->setProgress( progress );
}

void ArkPart::cancelTransfer()
{
    disconnect( m_bar->cancelButton(), SIGNAL( clicked() ),
                this, SLOT( cancelTransfer() ) );
    if ( m_job )
    {
        m_job->kill( KJob::EmitResult  );
        transferCanceled( QString() );
    }
}

ArkBrowserExtension::ArkBrowserExtension( KParts::ReadOnlyPart * parent, const char * /*name*/ )
                : KParts::BrowserExtension( parent )
{
}

void ArkBrowserExtension::slotOpenUrlRequested( const KUrl & url )
{
    emit openUrlRequest( url, KParts::URLArgs() );
}

ArkStatusBarExtension::ArkStatusBarExtension( KParts::ReadWritePart * parent )
                : KParts::StatusBarExtension( parent ),
                  m_bBusy( false ),
                  m_pStatusLabelSelect( 0 ),
                  m_pStatusLabelTotal( 0 ),
                  m_pBusyText( 0 ),
                  m_cancelButton( 0 ),
                  m_pProgressBar( 0 ),
                  m_pTimer( 0 )
{
}

ArkStatusBarExtension::~ArkStatusBarExtension()
{
}

void ArkStatusBarExtension::setupStatusBar()
{
    if ( m_pTimer                      // setup already done
         || !statusBar() )
    {
        return;
    }

    m_pTimer = new QTimer( this );
    connect( m_pTimer, SIGNAL( timeout() ), this, SLOT( slotProgress() ) );

    m_pStatusLabelTotal = new QLabel( statusBar() );
    m_pStatusLabelTotal->setFrameStyle( QFrame::NoFrame );
    m_pStatusLabelTotal->setAlignment( Qt::AlignRight );
    m_pStatusLabelTotal->setText( i18n( "Total: 0 files" ) );

    m_pStatusLabelSelect = new QLabel( statusBar() );
    m_pStatusLabelSelect->setFrameStyle( QFrame::NoFrame );
    m_pStatusLabelSelect->setAlignment( Qt::AlignLeft );
    m_pStatusLabelSelect->setText(i18n( "0 files selected" ) );

    m_cancelButton = new QPushButton( SmallIcon( "cancel" ), QString(), statusBar() );

    addStatusBarItem( m_pStatusLabelSelect, 3000, false );
    addStatusBarItem( m_pStatusLabelTotal, 3000, false );
}

void ArkStatusBarExtension::slotSetStatusBarText( const QString & text )
{
    if ( !statusBar() )
        return;

    setupStatusBar();
    m_pStatusLabelTotal->setText( text );
}

void ArkStatusBarExtension::slotSetStatusBarSelectedFiles( const QString & text )
{

    if ( !statusBar() )
        return;

    setupStatusBar();
    m_pStatusLabelSelect->setText( text );
}

void ArkStatusBarExtension::slotSetBusy( const QString & text, bool showCancelButton, bool detailedProgress )
{
    if ( m_bBusy || !statusBar() )
        return;

    setupStatusBar();
    if ( !m_pBusyText )
    {
        m_pBusyText = new QLabel( statusBar() );

        m_pBusyText->setAlignment( Qt::AlignLeft );
        m_pBusyText->setFrameStyle( QFrame::Panel | QFrame::Raised );
    }

    if ( !m_pProgressBar )
    {
        m_pProgressBar = new QProgressBar( statusBar() );
        m_pProgressBar->setFixedHeight( m_pBusyText->fontMetrics().height() );
    }

    if ( !detailedProgress )
    {
        m_pProgressBar->setMaximum( 0 );
        m_pProgressBar->setTextVisible( false );
    }
    else
    {
        m_pProgressBar->setMaximum(100);
        m_pProgressBar->setTextVisible( true );
    }

    m_pBusyText->setText( text );

    removeStatusBarItem( m_pStatusLabelSelect );
    removeStatusBarItem( m_pStatusLabelTotal );

    addStatusBarItem( m_pBusyText, 5, true );
    addStatusBarItem( m_pProgressBar, 1, true );
    if ( showCancelButton )
    {
        addStatusBarItem( m_cancelButton, 0, true );
    }

    if ( !detailedProgress )
    {
        m_pTimer->start( 200);
	m_pTimer->setSingleShot(false);
    }
    m_bBusy = true;
}

void ArkStatusBarExtension::slotSetReady()
{
    if ( !m_bBusy || !statusBar() )
        return;

    setupStatusBar();
    m_pTimer->stop();
    m_pProgressBar->setValue( 0 );

    removeStatusBarItem( m_pBusyText );
    removeStatusBarItem( m_pProgressBar );
    removeStatusBarItem( m_cancelButton );

    addStatusBarItem( m_pStatusLabelSelect, 3000, false );
    addStatusBarItem( m_pStatusLabelTotal, 3000, false );

    m_bBusy = false;
}

void ArkStatusBarExtension::slotProgress()
{
    if ( !statusBar() )
        return;

    setupStatusBar();
    m_pProgressBar->setValue( m_pProgressBar->value() + 4 );
}

void ArkStatusBarExtension::setProgress( unsigned long progress )
{
    if ( m_pProgressBar && ( m_pProgressBar->maximum() != 0 ) )
    {
        m_pProgressBar->setValue( progress );
    }
}

#include "ark_part.moc"
