/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999: Emily Ezust  emilye@corel.com

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


// Qt includes
#include <qdir.h>
#include <qdragobject.h>
#include <qevent.h>
#include <qmessagebox.h>
#include <qregexp.h>

// KDE includes
#include <kapp.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kkeydialog.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kmessagebox.h>
#include <kstatusbar.h>
#include <ktoolbar.h>

// c includes

#include <sys/stat.h>
#include <errno.h>

// ark includes
#include "arkwidget.h"
#include "arkwidget.moc"

#include "dirDlg.h"
#include "generalOptDlg.h"
#include "selectDlg.h"
#include "shellOutputDlg.h"
#include "arkstrings.h"

extern int errno;

enum Buttons { OPEN_BUTTON= 1000, NEW_BUTTON, FAVORITE_BUTTON, EXTRACT_BUTTON,
	       CLOSE_BUTTON };

QList<ArkWidget> *ArkWidget::windowList = 0;

ArkWidget::ArkWidget( QWidget *, const char *name ) : 
    KTMainWindow(name), m_nSizeOfFiles(0), m_nSizeOfSelectedFiles(0),
    m_nNumFiles(0), m_nNumSelectedFiles(0), m_bIsArchiveOpen(false),
    m_bIsSimpleCompressedFile(false)
{
    kdebug(0, 1601, "+ArkWidget::ArkWidget");
  
    m_data = new ArkData();
    // Creates a temp directory for this ark instance
    unsigned int pid = getpid();
    QString tmpdir;
    tmpdir.sprintf( "/tmp/ark.%d/", pid );
    QString ex( "mkdir " + tmpdir + " &>/dev/null" );
    system( ex.local8Bit() );
	
    m_data->setTmpDir( tmpdir );
    
    if (!windowList)
    {
	windowList = new QList<ArkWidget>();
    }
    
    windowList->setAutoDelete( FALSE );
    windowList->append( this );
    
    // Build the ark UI
    kdebug(0, 1601, "Build the GUI");
    setupMenuBar();
    kdebug(0, 1601, "Menubar build...");
    setupStatusBar();
    kdebug(0, 1601, "Statusbar build...");
    setupToolBar();
    kdebug(0, 1601, "Toolbar build...");
    createFileListView();
    kdebug(0, 1601, "GUI build...");

    // enable DnD
    setAcceptDrops(true);
        
    arch=0;
	
    //writeStatusMsg( i18n("Welcome to ark...") );


    // start out with some menu items disabled
    fileMenu->setItemEnabled(eMClose, false);
    actionMenu->setItemEnabled(eMAddFile, false);
//    editMenu->setItemEnabled(eMAddDir, false);
    actionMenu->setItemEnabled(eMDelete, false);
    actionMenu->setItemEnabled(eMExtract, false);
    editMenu->setItemEnabled(eMView, false);
    editMenu->setItemEnabled(eMRename, false);
    editMenu->setItemEnabled(eMSelectAll, false);

#if 0
  m_pFilePopup->setItemEnabled(eMExtract, false);
  m_pFilePopup->setItemEnabled(eMView, false);
  m_pFilePopup->setItemEnabled(eMRename, false);
  m_pFilePopup->setItemEnabled(eMDelete, false);

  m_pArchivePopup->setItemEnabled(eMAddFile, false);
  m_pArchivePopup->setItemEnabled(eMAddDir, false);
  m_pArchivePopup->setItemEnabled(eMSelectAll, false);
  m_pArchivePopup->setItemEnabled(eMClose, false);
#endif


    kdebug(0, 1601, "-ArkWidget::ArkWidget");

    resize(640,300);


}

ArkWidget::~ArkWidget()
{
	windowList->removeRef( this );
	delete archiveContent;
	delete recentPopup;
	delete accelerators;
	delete pop;
	delete m_data;
//	delete statusBarTimer;
	delete arch;
}

void ArkWidget::setupMenuBar()
{
	// KAccel initialization
	accelerators = new KAccel(this);

	accelerators->insertStdItem(KAccel::New, i18n("New"));
	accelerators->insertStdItem(KAccel::Open, i18n("Open"));
	accelerators->insertStdItem(KAccel::Close, i18n("Close"));
	accelerators->insertStdItem(KAccel::Quit, i18n("Quit"));

	accelerators->insertItem(i18n("Add"), "Add_accel", "SHIFT+A");
	accelerators->insertItem(i18n("Delete"), "Delete_accel", "SHIFT+D");
	accelerators->insertItem(i18n("Extract"), "Extract_accel", "SHIFT+E");
	accelerators->insertItem(i18n("View"), "View_accel", "SHIFT+V");
	accelerators->insertItem(i18n("Select"), "Selection", "CTRL+L");
	accelerators->insertItem(i18n("Select all"), "SelectionAll", "CTRL+A");
	accelerators->insertItem(i18n("Deselect all"), "DeselectionAll", "CTRL+D");
	accelerators->insertItem(i18n("Invert selection"), "InvertSel", "CTRL+I");

	accelerators->insertStdItem(KAccel::Help);

	// KAccel connections
	accelerators->connectItem(KAccel::New, this, SLOT(file_new()));
	accelerators->connectItem(KAccel::Open, this, SLOT(file_open()));
	accelerators->connectItem(KAccel::Close, this, SLOT(file_close()));
	accelerators->connectItem(KAccel::Quit, this, SLOT(file_quit()));

	accelerators->connectItem("Add_accel", this, SLOT(action_add()));
	accelerators->connectItem("Delete_accel", this, SLOT(action_delete()));
	accelerators->connectItem("Extract_accel", this, SLOT(action_extract()));
	accelerators->connectItem("View_accel", this, SLOT(action_view()));
	accelerators->connectItem("Selection", this, SLOT(edit_select()));
	accelerators->connectItem("SelectionAll", this, SLOT(edit_selectAll()));
	accelerators->connectItem("DeselectionAll", this, SLOT(edit_deselectAll()));
	accelerators->connectItem("InvertSel", this, SLOT(edit_invertSel()));

	// KAccel settings
	accelerators->readSettings( KGlobal::config() );
	int id;

	// File menu creation
	fileMenu = new QPopupMenu;
	recentPopup = new QPopupMenu;
	editMenu = new QPopupMenu;
	actionMenu = new QPopupMenu;
	optionsMenu = new QPopupMenu;

	KMenuBar *menu = menuBar();

	createRecentPopup();

	fileMenu->insertItem( i18n( "New &Window..."), this,
			      SLOT( file_newWindow()));
	fileMenu->insertSeparator();
	id = fileMenu->insertItem( i18n( "&New..." ), this, SLOT( file_new()),
				   0, eMNew);
	accelerators->changeMenuAccel(fileMenu, id, KAccel::New );

	id=fileMenu->insertItem( i18n( "&Open..." ), this,  SLOT( file_open()),
				 0, eMOpen);
	accelerators->changeMenuAccel(fileMenu, id, KAccel::Open );
	id=fileMenu->insertItem( i18n( "Open &recent" ), recentPopup);
	connect(recentPopup, SIGNAL(activated(int)), this,
		SLOT(file_openRecent(int)));
	fileMenu->insertItem( i18n( "Relo&ad" ), this,  SLOT( file_reload()) );

	fileMenu->insertSeparator();
	id=fileMenu->insertItem( i18n( "&Close"), this, SLOT( file_close()),
				 0, eMClose);
	accelerators->changeMenuAccel(fileMenu, id, KAccel::Close );

	id=fileMenu->insertItem( i18n( "&Quit"), this, SLOT( file_quit() ),
				 0, eMExit);
	accelerators->changeMenuAccel(fileMenu, id, KAccel::Quit );

	createEditMenu();
	
	// Options menu creation
	optionsMenu->insertItem( i18n( "&General..."), this, SLOT( options_general() ) );
	optionsMenu->insertItem( i18n( "&Directories..."), this, SLOT( options_dirs() ) );
	optionsMenu->insertItem( i18n( "&Keys..."), this, SLOT( options_keys() ) );
	optionsMenu->insertSeparator();
	optionsMenu->insertItem( i18n( "&Save settings now..."), this, SLOT( options_saveNow() ) );
	idSaveOnExit=optionsMenu->insertItem( i18n( "Save settings on e&xit..."), this, SLOT( options_saveOnExit() ) );
	optionsMenu->setItemChecked(idSaveOnExit, m_data->isSaveOnExitChecked());

	// Help menu creation
	QString about_ark = IDS_BLURB.arg( ARK_VERSION );
	QPopupMenu *helpmenu = helpMenu( about_ark );

	menu->insertItem( i18n( "&File"), fileMenu );
	idEditMenu = menu->insertItem( i18n( "&Edit"), editMenu );
	menu->setItemEnabled(idEditMenu, false);
	idActionMenu = menu->insertItem( i18n( "&Action"), actionMenu );
	menu->setItemEnabled(idActionMenu, false);
	menu->insertItem( i18n( "&Options"), optionsMenu );
	menu->insertSeparator();
	menu->insertItem( i18n( "&Help" ), helpmenu );

	setMenu( menu );

	pop = new KPopupMenu();
//	pop->setTitle( i18n("File Operations") );
//	pop->insertItem( i18n("Extract..."), this, SLOT( action_extract() ) );
//	pop->insertItem( i18n("View file"), this, SLOT( action_view() ) );
//	pop->insertSeparator();
//	pop->insertItem( i18n("Delete file"), this, SLOT( action_delete() ) );
}

void ArkWidget::createEditMenu()
{
	int id;

	id=editMenu->insertItem( i18n( "&Select..."), this, SLOT( edit_select() ) );
	accelerators->changeMenuAccel(editMenu, id, "Selection" );

	id=editMenu->insertItem( i18n( "&Select all"), this, SLOT( edit_selectAll() ), 0, eMSelectAll );
	accelerators->changeMenuAccel(editMenu, id, "SelectionAll" );

	id=editMenu->insertItem( i18n( "Dese&lect all"), this, SLOT( edit_deselectAll() ) );
	accelerators->changeMenuAccel(editMenu, id, "DeselectionAll" );

	id=editMenu->insertItem( i18n( "&Invert selection"), this, SLOT( edit_invertSel() ) );
	accelerators->changeMenuAccel(editMenu, id, "InvertSel" );

	editMenu->insertSeparator();
	
	editMenu->insertItem( i18n( "&View last shell output..."), this, SLOT( edit_view_last_shell_output() ) );
}

void ArkWidget::createActionMenu( int _flag )
{
	KASSERT(arch != 0, 0, 1601, "arch is empty !" );
	
	actionMenu->clear();
	if( _flag & Arch::Add ){
		idAdd=actionMenu->insertItem( i18n( "&Add..."), this, SLOT( action_add() ), 0, eMAddFile );
		accelerators->changeMenuAccel(actionMenu, idAdd, "Add_accel" );
	}
	else idAdd = -1;

        if( _flag & Arch::Delete ){
		idDelete=actionMenu->insertItem( i18n( "&Delete..."), this, SLOT( action_delete() ), eMDelete );
		actionMenu->setItemEnabled(idDelete, false);
		accelerators->changeMenuAccel(actionMenu, idDelete, "Delete_accel" );
	}
	else idDelete = -1;
	
        if( _flag & Arch::Extract ){
		idExtract=actionMenu->insertItem( i18n( "E&xtract..."), this, SLOT( action_extract() ), 0, eMExtract );
		accelerators->changeMenuAccel(actionMenu, idExtract, "Extract_accel" );
	}
	else idExtract = -1;
	
        if( _flag & Arch::View ){
		idView=actionMenu->insertItem( i18n( "&View..."), this, SLOT( action_view() ), 0, eMView );
		accelerators->changeMenuAccel(actionMenu, idView, "View_accel" );
	}
	else idView = -1;
	
	menuBar()->setItemEnabled(idActionMenu, true);
}

void ArkWidget::createRecentPopup()
{
	recentPopup->clear();

	QStringList *recentFiles = m_data->getRecentFiles();
	for (uint i=0; i<recentFiles->count(); i++)
	{
        	recentPopup->insertItem((*recentFiles)[i], -1, i);
	}
}

void ArkWidget::setupStatusBar()
{
    kdebug(0, 1601, "+ArkWidget::setupStatusBar");

    KStatusBar *sb = statusBar();
    sb->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    sb->insertItem(IDS_NO_FILES_SELECTED, eSelectedStatusLabel);
    QFrame *separator = new QFrame(sb, "separator");
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameStyle(QFrame::Panel | QFrame::Raised);
    sb->insertWidget(separator, 2, eStatusLabelSeparator);
    sb->insertItem(IDS_NO_FILES, eNumFilesStatusLabel);
    sb->setAlignment(0, AlignCenter);
    sb->setAlignment(2, AlignCenter);
    sb->setBorderWidth(2);

    QFrame *dummy = new QFrame(sb, "dummy");
    sb->insertWidget(dummy, 0, eStatusDummy);
    kdebug(0, 1601, "-ArkWidget::setupStatusBar");

}

void ArkWidget::setupToolBar()
{
    kdebug(0, 1601, "+ArkWidget::setupToolBar");

    KToolBar *tb = toolBar();


    tb->insertButton ( BarIcon("new_ark"), eNew, SIGNAL(clicked()), this,
		       SLOT(file_new()), TRUE, IDS_TOOLBAR_NEW);

    tb->insertButton ( BarIcon("open_ark"), eOpen, SIGNAL(clicked()), this,
		       SLOT(file_open()), TRUE, IDS_TOOLBAR_OPEN);

    tb->insertSeparator();

    tb->insertButton ( BarIcon("addfile"), eAddFile, SIGNAL(clicked()),
		       this, SLOT(action_add()), TRUE,
		       IDS_TOOLBAR_ADD_FILE);

    tb->insertButton ( BarIcon("adddir"), eAddDir, SIGNAL(clicked()), this,
		       SLOT(dir_add()), TRUE, IDS_TOOLBAR_ADD_DIR);

    tb->insertSeparator();

    tb->insertButton ( BarIcon("extract"), eExtract, SIGNAL(clicked()),
		       this, SLOT(action_extract()), TRUE,
		       IDS_TOOLBAR_EXTRACT);

    tb->insertButton ( BarIcon("delete"), eDelete, SIGNAL(clicked()), this,
		       SLOT(action_delete()), TRUE, IDS_TOOLBAR_DEL);

    tb->insertSeparator();

    tb->insertButton ( BarIcon("selectall"), eSelectAll, SIGNAL(clicked()),
		       this, SLOT(edit_selectAll()), TRUE,
		       IDS_TOOLBAR_SELECT_ALL);

    tb->insertButton ( BarIcon("view"), eView, SIGNAL(clicked()), this,
		       SLOT(file_show()), TRUE, IDS_TOOLBAR_VIEW);

    tb->insertSeparator();

    tb->insertButton ( BarIcon("options"), eOptions, SIGNAL(clicked()),
		       this, SLOT(options_dirs()), TRUE,
		       IDS_TOOLBAR_OPTIONS);

    tb->insertButton ( BarIcon("help"), eHelp, SIGNAL(clicked()), this,
		       SLOT(help()), TRUE, IDS_TOOLBAR_HELP);

    tb->setBarPos( KToolBar::Top );

    // start out with some disabled
    tb->setItemEnabled(eAddFile, false);
    tb->setItemEnabled(eAddDir, false);
    tb->setItemEnabled(eDelete, false);
    tb->setItemEnabled(eExtract, false);
    tb->setItemEnabled(eSelectAll, false);
    tb->setItemEnabled(eView, false);

    kdebug(0, 1601, "-ArkWidget::setupToolBar");
}


////////////////////////////////////////////////////////////////////
///////////////////////// updateStatusTotals ///////////////////////
////////////////////////////////////////////////////////////////////

void ArkWidget::updateStatusTotals()
{
    kdebug(0, 1601, "+ArkWidget::updateStatusTotals");
    m_nNumFiles = 0;
    FileLVI *pItem = (FileLVI *)archiveContent->firstChild();
    while (pItem)
    {
	++m_nNumFiles;
	// warning! hardcoded for now - 3 should be eSize
	kdebug(0, 1601, "Adding %d\n", atoi(pItem->text(3)));
	m_nSizeOfFiles += atoi(pItem->text(3));
	pItem = (FileLVI *)pItem->nextSibling();
    }

    kdebug(0, 1601, "We have %d elements\n", m_nNumFiles);

    char strInfo[BUFSIZ];
    sprintf(strInfo, "%s %d %s, %d KB", (const char *)IDS_TOTAL,
	    m_nNumFiles, (const char *) IDS_FILES, m_nSizeOfFiles);
    
    statusBar()->changeItem(strInfo, eNumFilesStatusLabel);
    kdebug(0, 1601, "-ArkWidget::updateStatusTotals");
}


#if 0

////////////////////////////////////////////////////////////////////
////////////////////////// addFile /////////////////////////////////
////////////////////////////////////////////////////////////////////

void CArkWidget::addFile(QStringList *list)
{
    // add the files in the list to the archive
    kdebug(0, 1601, "+ArkWidget::addFile(QStringList)");

  QApplication::setOverrideCursor( waitCursor );
#if 0
  CProgressDlg *pProgressDlg =
    //     new CProgressDlg(QString(KApplication::kde_icondir() + "/" + 
    new CProgressDlg(QString("to_archive_64.gif"),
		     IDS_ADDING, this, 0);
  pProgressDlg->setCaption(IDS_ADDING);
  pProgressDlg->show();
#endif

  switch (m_pArch->addFile(list))
  {
  case UNSUPDIR:  // from errors.h
    // ar doesn't let you add a directory!
      KMessageBox::error(this, IDS_NOT_SUPPORTED );
    break;
  }

#if 0
  delete pProgressDlg;
#endif
  QApplication::restoreOverrideCursor();

  // we may have been adding files into an empty archive - 
  // see if we need to change the button and menu item enables.
#if 0
  onFileNumChangeSetEnables();
#endif

  kdebug(0, 1601, "-ArkWidget::addFile(QStringList)");
}
#endif

////////////////////////////////////////////////////////////////////
////////////////////// isArchiveLocked /////////////////////////////
////////////////////////////////////////////////////////////////////

bool ArkWidget::isArchiveLocked(const QString & _strArchName)
{
    bool retval;

    kdebug(0, 1601, "+ArkWidget::isArchiveLocked");

    QString strLockPath = _strArchName.left(_strArchName.findRev('/')+1);
    QString strFileName = _strArchName.right(_strArchName.length()
					     - _strArchName.findRev('/')-1);
    QString strLockFileName;
    strLockFileName.sprintf("#%s#", (const char *)strFileName);

    QString strFullName = strLockPath + strLockFileName;
  
    kdebug(0, 1601, "Lock file is %s\n", (const char *)strLockFileName);

    struct stat statbuffer;
  
    if (stat((const char *)strFullName, &statbuffer) == -1)
    {
	retval = false;
    }
    else
    {
	retval = true;
    }

    kdebug(0, 1601, "-ArkWidget::isArchiveLocked");
    return retval;
}

///////////////////////////////////////////////////////////////////
//////////////////////// createLockFile ///////////////////////////
///////////////////////////////////////////////////////////////////

void ArkWidget::createLockFile()   // private
{
  // use m_strArchName to create a lock file

  kdebug(0, 1601, "+ArkWidget::createLockFile");

  kdebug(0, 1601, "Creating lockfile for %s\n", (const char *)m_strArchName);
  QString strLockPath = m_strArchName.left(m_strArchName.findRev('/')+1);
  QString strFileName = m_strArchName.right(m_strArchName.length()
					    - m_strArchName.findRev('/')-1);
  chdir( (const char *)strLockPath);

  QString strLockFileName;
  strLockFileName.sprintf("#%s#",  (const char *)strFileName);
  
  QString command;
  command.sprintf("touch '%s'", (const char *)strLockFileName);

  system( (const char *) command);
  kdebug(0, 1601, "-ArkWidget::createLockFile");
}   

///////////////////////////////////////////////////////////////////
//////////////////////// deleteLockFile ///////////////////////////
///////////////////////////////////////////////////////////////////

void ArkWidget::deleteLockFile()   // private
{
  // use m_strArchName to delete a lock file
  kdebug(0, 1601, "+ArkWidget::deleteLockFile");

  if (isArchiveOpen())
  {
    QString strLockPath = m_strArchName.left(m_strArchName.findRev('/')+1);
    QString strFileName = m_strArchName.right(m_strArchName.length()
					      - m_strArchName.findRev('/')-1);
    chdir( (const char *)strLockPath);
    
    QString strLockFileName;
    strLockFileName.sprintf("#%s#",  (const char *)strFileName);
    
    QString command;
    command.sprintf("rm '%s'", (const char *)strLockFileName);

    kdebug(0, 1601, "%s\n", (const char *)command);

    system( (const char *) command);
  }
  kdebug(0, 1601, "-ArkWidget::deleteLockFile");
}   


//////////////////////////////////////////////////////////////////////
///////////////////////// file_open //////////////////////////////////
//////////////////////////////////////////////////////////////////////


void ArkWidget::file_open(const QString & strFile)
{
  if (! strFile.isNull() )  // if they don't click cancel
  {
    struct stat statbuffer;

    if (stat(strFile, &statbuffer) == -1)
    {
      if (errno == ENOENT || errno == ENOTDIR || errno ==  EFAULT)
      {
	KMessageBox::error(this, IDS_DOESNT_EXIST);
      }
      else if (errno == EACCES)
      {
	KMessageBox::error(this, IDS_NO_ACCESS);
      }
      return;
    }
    else
    {
      // this will be the appropriate flag depending on whose file it is
      unsigned int nFlag = 0;
      if (geteuid() == statbuffer.st_uid)
      {
	nFlag = S_IRUSR; // it's mine
      }
      else if (getegid() == statbuffer.st_gid)
      {
	nFlag = S_IRGRP; // it's my group's
      }
      else
      {
	nFlag = S_IROTH;  // it's someone else's
      }
    
      if (! ((statbuffer.st_mode & nFlag) == nFlag))
      {
	KMessageBox::error(this, IDS_NO_PERMISSION );
	return;
      }
    }

    if (isArchiveLocked(strFile) && (m_strArchName != strFile))
    {
	int nRet = QMessageBox::warning(this, IDS_WARNING, IDS_ARCHIVE_LOCKED,
					IDS_YES, IDS_CANCEL);
      if (nRet == 1)  // cancel
	return;

    }

    // no errors if we made it this far.

    file_close();  // close old zip

    // Set the current archive filename to the filename
    m_strArchName = strFile;

    // display the archive contents
    showZip(strFile);
  }
}





//////////////////////////////////////////////////////////////////////
//
// ArkWidget slots
//
//////////////////////////////////////////////////////////////////////


void ArkWidget::saveProperties()
{
	kdebug(0, 1601, "+saveProperties (exit)");

	KConfig *kc = m_data->getKConfig();
	kc->setGroup( "ark" );

	if( m_data->isSaveOnExitChecked() )
		accelerators->writeSettings( m_data->getKConfig() );

	m_data->writeConfiguration();

	QString tmpdir = m_data->getTmpDir();
	QString ex( "rm -rf "+tmpdir );
	system( ex.local8Bit() );

	kdebug(0, 1601, "-saveProperties (exit)");
}


// File menu /////////////////////////////////////////////////////////

void ArkWidget::file_new()
{
	QString file = KFileDialog::getSaveFileName(QString::null, m_data->getFilter());
	if( !file.isEmpty() )
	{
	    createFileListView();
	    Arch *tempArch = createArchive( file );
	    if (tempArch != 0)
	    {
		arch = tempArch;
	    }
	}
}

void ArkWidget::slotCreate( bool _success, QString _filename, int _flag )
{
    if( _success ){
	newCaption( _filename );
	createActionMenu( _flag );
    }
    else
    {
	KMessageBox::error(this, i18n( "Can't create archive of that type") );
    }
}

void ArkWidget::file_newWindow()
{
	ArkWidget *kw = new ArkWidget;
	kw->show();
}

void ArkWidget::file_open()
{
    QString file = KFileDialog::getOpenFileName(m_data->getOpenDir(),
						m_data->getFilter(), this);
    file_open( file );
}

void ArkWidget::file_openRecent(int i)
{
	QString filename = recentPopup->text(i);
	file_open( filename );

	kdebug(0, 1601, "-ArkWidget::file_openRecent");
}

void ArkWidget::showZip( QString _filename )
{
	kdebug(0, 1601, "+ArkWidget::showZip");

	createFileListView();

	Arch *tempArch = openArchive( _filename );
	if (tempArch != 0)
	{
	    arch = tempArch;
	    // Create a lock file so that we won't open the same archive twice.
	    createLockFile();
	    setCaption(IDS_ARCHIVER_PREFIX + _filename);
	}

	updateStatusTotals();
	kdebug(0, 1601, "-ArkWidget::showZip");
}

void ArkWidget::slotOpen( bool _success, QString _filename, int _flag )
{
    kdebug(0, 1601, "+ArkWidget::slotOpen");
    
    archiveContent->setUpdatesEnabled(true);
    archiveContent->triggerUpdate();
    
    if( _success )
    {
	newCaption( _filename );
	menuBar()->setItemEnabled(idEditMenu, true);
	createActionMenu( _flag );
	
	QFileInfo fi( _filename );
	QString path = fi.dirPath( true );
	m_data->setLastOpenDir( path );
	updateStatusTotals();
	// make sure the enables are right
	openEnables();
    }
    else
    {
//	archiveContent->clear();
//	clearCurrentArchive();	    // just leave current
    }
	
    kdebug(0, 1601, "-ArkWidget::slotOpen");
}

///////////////////////////////////////////////////////////////////
//////////////////////// openEnables //////////////////////////////
///////////////////////////////////////////////////////////////////

void ArkWidget::openEnables()   // private
{
    m_bIsArchiveOpen = true;

    // enable appropriate menu items 

    fileMenu->setItemEnabled(eMClose, true);
    actionMenu->setItemEnabled(eMAddFile, true);
//    editMenu->setItemEnabled(eMAddDir, true);
    if (m_nNumFiles > 0)
    {
	editMenu->setItemEnabled(eMSelectAll, true);
	actionMenu->setItemEnabled(eMExtract, true);
    }
#if 0
    m_pArchivePopup->setItemEnabled(eMAddFile, true);
  m_pArchivePopup->setItemEnabled(eMAddDir, true);
  m_pArchivePopup->setItemEnabled(eMClose, true);

    if (m_nNumFiles > 0)
    {
	m_pArchivePopup->setItemEnabled(eMSelectAll, true);
	m_pArchivePopup->setItemEnabled(eMExtract, true);
    }
#endif

  // enable appropriate toolbuttons

    KToolBar *tb = toolBar();
    tb->setItemEnabled(eAddFile, true);
    tb->setItemEnabled(eAddDir, true);
    
    if (m_nNumFiles > 0)
    {
	tb->setItemEnabled(eSelectAll, true);
	tb->setItemEnabled(eExtract, true);
    }
}

//////////////////////////////////////////////////////////////////////
///////////////// onFileNumChangeSetEnables //////////////////////////
//////////////////////////////////////////////////////////////////////

void ArkWidget::onFileNumChangeSetEnables() // private
{
#if 0
    bool bHaveFiles = (m_nNumFiles > 0);

  // enable the select all and extract options
 
  m_pEditMenu->setItemEnabled(eMSelectAll, bHaveFiles);
  m_pEditMenu->setItemEnabled(eMExtract, bHaveFiles);
  m_pArchivePopup->setItemEnabled(eMSelectAll, bHaveFiles);
  m_pArchivePopup->setItemEnabled(eMExtract, bHaveFiles);
  m_aButtons[eSelectAll]->setEnabled(bHaveFiles);
  m_aButtons[eExtract]->setEnabled(bHaveFiles);

  // if there are no files, disable delete, view and rename.
  // If there are files, don't enable them! There may not be a selection yet.
  if (!bHaveFiles)
  {
    m_aButtons[eDelete]->setEnabled(false);
    m_pEditMenu->setItemEnabled(eMDelete, false);

    m_aButtons[eView]->setEnabled(false);
    m_pEditMenu->setItemEnabled(eMView, false);

    m_pEditMenu->setItemEnabled(eMRename, false);

  }
#endif
}


void ArkWidget::file_reload()
{
	QString filename = arch->fileName();
	file_open( filename );
}

void ArkWidget::reload()
{
	file_reload();
}
	
void ArkWidget::file_close()
{
    kdebug(0, 1601, "+ArkWidget::file_close");
    if (isArchiveOpen())
    {
	deleteLockFile();
	setCaption(IDS_ARCHIVER);
	delete arch;
	arch = 0;
	m_bIsArchiveOpen = false;
	
	if (archiveContent)
	{
	    archiveContent->clear();
	}
    
	updateStatusTotals();
    }
    kdebug(0, 1601, "-ArkWidget::file_close");
}

void ArkWidget::window_close()
{
    kdebug(0, 1601, "+ArkWidget::window_close");
    file_close();
    if( windowList->count() < 2 )
    {
	saveProperties();
	kapp->quit();
    }
    else
    {
	saveProperties();
	delete this;
    }
    kdebug(0, 1601, "-ArkWidget::window_close");
}


void ArkWidget::closeEvent( QCloseEvent * )
{
    
    window_close();
    kapp->quit();
}


void ArkWidget::file_quit()
{
    file_close();
    saveProperties();
    kapp->quit();
}

// Edit menu /////////////////////////////////////////////////////////

void ArkWidget::edit_select()
{
	SelectDlg *sd = new SelectDlg( m_data, this );
	if( sd->exec() ){
		QString exp = sd->getRegExp();
		m_data->setSelectRegExp( exp );

		QRegExp reg_exp( exp, true, true );
		KASSERT(reg_exp.isValid(), 0, 1601, "ArkWidget::edit_select: regular expression is not valid.");
		
		FileLVI * flvi = (FileLVI*)archiveContent->firstChild();

		while (flvi)
		{
			if( reg_exp.match(flvi->text(0))==0 ){
		        	archiveContent->setSelected(flvi, true);
			}
			flvi = (FileLVI*)flvi->itemBelow();
		}
			
	}
}

void ArkWidget::edit_selectAll()
{
	FileLVI * flvi = (FileLVI*)archiveContent->firstChild();

	while (flvi)
	{
        	archiveContent->setSelected(flvi, true);
		flvi = (FileLVI*)flvi->itemBelow();
	}
}

void ArkWidget::edit_deselectAll()
{
	archiveContent->clearSelection();
}

void ArkWidget::edit_invertSel()
{
	FileLVI * flvi = (FileLVI*)archiveContent->firstChild();

	while (flvi)
	{
        	archiveContent->setSelected(flvi, !flvi->isSelected());
		flvi = (FileLVI*)flvi->itemBelow();
	}

}

void ArkWidget::edit_view_last_shell_output()
{
	ShellOutputDlg* sod = new ShellOutputDlg( m_data, this );
	sod->exec();
}


// Action menu /////////////////////////////////////////////////////////

void ArkWidget::action_add()
{
	arch->addFile( 0 );
}

void ArkWidget::action_delete()
{
    kdebug(0, 1601, "+ArkWidget::action_delete");

    KASSERT(!archiveContent->isSelectionEmpty(), 3, 1601, "Nothing to be removed !" );

    if( KMessageBox::questionYesNo(this, i18n("Do you really want to delete the selectionned items?")) == KMessageBox::Yes)
    {
	arch->remove();
	updateStatusTotals();
    }

    kdebug(0, 1601, "-ArkWidget::action_delete");
}

void ArkWidget::action_extract()
{
	arch->extract();
}

void ArkWidget::action_view()
{
/*
	if( lb->currentItem() != -1 )
	{
		showFile( lb->currentItem() );
	}
*/
}

void ArkWidget::showFile( int /*index*/, int /*col*/ )
{
	QString tmp;
	QString tname;
	QString name;
	QString fullname;

/*
	col++; // Don't ask.
	tmp = listing->at( index );
	tname = tmp.right( tmp.length() - (tmp.findRev('\t')+1) );

	if( arch == 0 )
	{
		fullname = fav->path();
		fullname+= "/";
		fullname+= tname;
		showZip( fullname );
	}else{
		fullname = "file:";
		fullname += arch->unarchFile( index, tmpdir );
		(void) new KRun ( fullname );
	}
*/
}

// Options menu //////////////////////////////////////////////////////

void ArkWidget::options_general()
{
	GeneralDlg *gd = new GeneralDlg( m_data, this );
	gd->exec();
	delete gd;
}

void ArkWidget::options_dirs()
{
	DirDlg *dd = new DirDlg( m_data, this );
	dd->exec();
	delete dd;
}

void ArkWidget::options_keys()
{
	KKeyDialog::configureKeys(accelerators, this);
}

void ArkWidget::options_saveNow()
{
	m_data->writeConfigurationNow();
}

void ArkWidget::options_saveOnExit()
{
	optionsMenu->setItemChecked(idSaveOnExit, !m_data->isSaveOnExitChecked());
	m_data->setSaveOnExitChecked( !m_data->isSaveOnExitChecked() );
}

// Help menu /////////////////////////////////////////////////////////

void ArkWidget::help()
{
	kapp->invokeHTMLHelp( "", "" );
}


// Service functions /////////////////////////////////////////////////

void ArkWidget::slotSelectionChanged()
{
    KToolBar *tb = toolBar();

    actionMenu->setItemEnabled( idDelete, !archiveContent->isSelectionEmpty() );

    updateStatusSelection();

	
    if (m_nNumSelectedFiles > 0)
    {
	// enable appropriate menu items and toolbuttons
	actionMenu->setItemEnabled(eMDelete, true);
	actionMenu->setItemEnabled(eMExtract, true);
    
#if 0
	filePopup->setItemEnabled(eMExtract, true);
	filePopup->setItemEnabled(eMDelete, true);
#endif
    
	tb->setItemEnabled(eDelete, true);

	// Because there's no 'multiple view' or 'multiple rename', we disable
	// these options when there are multiple selections. But it is 
	// unconditionally enabled in the edit popup because the user had
	// to have right-clicked on an element in order to see the menu.
#if 0
	filePopup->setItemEnabled(eMView, true);
	filePopup->setItemEnabled(eMRename, true);
#endif

	bool bEnable = (1 == m_nNumSelectedFiles);

	editMenu->setItemEnabled(eMView, bEnable);
//	editMenu->setItemEnabled(eMRename, bEnable);

	tb->setItemEnabled(eExtract, true);
	tb->setItemEnabled(eView, bEnable);
    }
    else
    {
	actionMenu->setItemEnabled(eMDelete, false);

//	filePopup->setItemEnabled(eMDelete, false);

	editMenu->setItemEnabled(eMView, false);
//	filePopup->setItemEnabled(eMView, false);
//	editMenu->setItemEnabled(eMRename, false);
//	m_pFilePopup->setItemEnabled(eMRename, false);

	tb->setItemEnabled(eView, false);
	tb->setItemEnabled(eDelete, false);
    }
}


////////////////////////////////////////////////////////////////////
//////////////////// updateStatusSelection /////////////////////////
////////////////////////////////////////////////////////////////////

void ArkWidget::updateStatusSelection()
{
  kdebug(0, 1601, "+ArkWidget::updateStatusSelection");

  m_nNumSelectedFiles = 0;
  m_nSizeOfSelectedFiles = 0;

  FileLVI * flvi = (FileLVI*)archiveContent->firstChild();
  while (flvi)
  {
      if (flvi->isSelected())
      {
	  ++m_nNumSelectedFiles;
	  // warning! hardcoded for now - 3 should be eSize
	  m_nSizeOfSelectedFiles += atoi(flvi->text(3));
      }
      flvi = (FileLVI*)flvi->itemBelow();
  }

  char strInfo[BUFSIZ];
  sprintf(strInfo, "%d %s %s, %d KB", m_nNumSelectedFiles,
	  (const char *) ((m_nNumSelectedFiles != 1)? IDS_FILES : IDS_FILE),
	  (const char *) IDS_SELECTED, m_nSizeOfSelectedFiles);

  statusBar()->changeItem(strInfo, eSelectedStatusLabel);

  kdebug(0, 1601, "-ArkWidget::updateStatusSelection");
}



// Drag & Drop ////////////////////////////////////////////////////////

void ArkWidget::dragEnterEvent(QDragEnterEvent* event)
{
	event->accept(QUrlDrag::canDecode(event));
}

void ArkWidget::dropEvent(QDropEvent* event )
{
    kdebug(0, 1601, "+ArkWidget::dropEvent");

	QStringList dlist;
	QString url;
	QString file;
	bool opennew=false;

        if(!QUrlDrag::decodeToUnicodeUris(event, dlist))
		return;
        
	KMessageBox::sorry(this, "Sorry, not yet implemented.");
	return;

	// TODO:
	if( !arch ){	/* No archive is currently loaded */
		const char *foo;
		url = dlist[0];
		file = url.right( url.length()-5 );
		foo = file.local8Bit();
		
//		TODO: port to signal/slot
//		if( openArchive(file) )
//			opennew=true;
//		else
//			file_new();
	}
	if( arch && !opennew )  /* An archive was open or we just created one */
	{		        /* so we add the url list in the current one */
		int retcode;
		retcode = arch->addFile( &dlist );
		if( !retcode )
		{
			archiveContent->clear();
		} else {
//			if( retcode == UNSUPDIR )
//				arkWarning( i18n("Can't add directories with this archive type"));
//			else
//				KMessageBox::error( this, i18n( "Error saving to archive"));
		}
	}

	kdebug(0, 1601, "-dropEvent");
}


void ArkWidget::showFavorite()
{
	const QFileInfoList *flist;
	QDir *fav;

	clearCurrentArchive();

	archiverMode = false;

//	delete archiveContent;
	createFileListView();

	archiveContent->addColumn( i18n("File") );
	archiveContent->addColumn( i18n("Size") );
	archiveContent->setColumnAlignment(1, AlignRight);
	archiveContent->setMultiSelection( false );

	fav = new QDir( m_data->getFavoriteDir() );
	if( !fav->exists() )
	{
		KMessageBox::error( this, i18n("Archive directory does not exist."));
		return;
	}
	flist = fav->entryInfoList();
	QFileInfoListIterator flisti( *flist );
	++flisti; // Skip . directory

	if( (flisti.current())->fileName() == ".." )
	{
		FileLVI *flvi = new FileLVI(archiveContent);
		flvi->setText(0, "..");
		archiveContent->insertItem(flvi);
		++flisti;
	}

	QString size;
	bool isDirectory;
	for( uint i=0; i < flist->count()-2; i++ )
	{
		QString name( (flisti.current())->fileName() );
		isDirectory = (flisti.current())->isDir();
		if( (getArchType(name)!=-1) || (isDirectory) )
		{
			FileLVI *flvi = new FileLVI(archiveContent);
			flvi->setText(0, name);
			if(!isDirectory)
			{
		                size.sprintf("%d", (flisti.current())->size());
				flvi->setText(1, size);
				archiveContent->insertItem(flvi);
			}
		}
		++flisti;
	}
	archiveContent->setColumnWidth(0, archiveContent->columnWidth(0) + 10 );

	setCaption( m_data->getFavoriteDir() );

	delete fav;

//	writeStatusMsg( i18n( "Archive Directory") );
}

/**
 * Writes a message in the status bar.
 * This message is visible during 5 seconds.
 */
#if 0
void ArkWidget::writeStatusMsg(const QString text)
{
	statusBarTimer->stop();
	statusBar()->changeItem(text, 0);
	statusBarTimer->start(5000,true);
}
#endif

#if 0
void ArkWidget::clearStatusBar()
{
	statusBar()->changeItem("",0);
}
#endif

void ArkWidget::clearCurrentArchive()
{
	if (!archiveContent)
		archiveContent->clear();

	if (!arch){
		delete arch;
		arch = 0;
	}

	setCaption("");
	setView(0);
	
	menuBar()->setItemEnabled(idEditMenu, false);
	menuBar()->setItemEnabled(idActionMenu, false);
	
	toolBar()->setItemEnabled( EXTRACT_BUTTON, false );
}

void ArkWidget::arkWarning(const QString& msg)
{
        KMessageBox::information(this, msg);
}

#if 0
void ArkWidget::slotStatusBarTimeout()
{
	clearStatusBar();
}
#endif

void ArkWidget::newCaption(const QString& _filename){

	//TODO: add the item count in the statusbar

//	QString caption;
//	caption = i18n("ark - %1[%2 files]").arg(filename).arg(archiveContent->count());
	setCaption( _filename );

	toolBar()->setItemEnabled( EXTRACT_BUTTON, true );

	m_data->addRecentFile( _filename );
	createRecentPopup();
}

void ArkWidget::createFileListView()
{
	archiveContent = new FileListView(this);
	archiveContent->setMultiSelection(true);
	setView(archiveContent);
	updateRects();
	archiveContent->show();

	connect( archiveContent, SIGNAL( selectionChanged()), this, SLOT( slotSelectionChanged() ) );
}


int ArkWidget::getArchType( QString archname )
{
	if( archname.contains(".tgz", FALSE) || archname.contains(".tar.gz", FALSE)
			|| archname.contains( ".tar.Z", FALSE ) || archname.contains(".tar.bz", FALSE)
			|| archname.contains( ".tar.bz2", FALSE ) || archname.contains(".tar.lzo", FALSE)
			|| archname.contains( ".tbz", FALSE ) || archname.contains(".tzo", FALSE)
			|| archname.contains( ".taz", FALSE) )
		return TAR_FORMAT;
//	if( archname.contains(".lha", FALSE) || archname.contains(".lzh", FALSE ))
//		return LHA_FORMAT;
	if( archname.contains(".zip", FALSE) )
		return ZIP_FORMAT;
//	if( archname.contains(".a", FALSE ) )
//		return AA_FORMAT;
	return -1;
}


Arch *ArkWidget::createArchive( QString _filename )
{
    // returns a pointer to the new archive or, if there's a problem,
    // returns 0.

    Arch * newArch = 0;

    switch( getArchType( _filename ) )
    {
#if 0
    case TAR_FORMAT:
	newArch = new TarArch( m_data, this, name );
	newArch->createArch( name );
	ret = true;
	break;
#endif
    case ZIP_FORMAT:
	newArch = new ZipArch( m_data, this, _filename );
	connect( newArch, SIGNAL(sigOpen(bool,QString,int)), this, SLOT(slotOpen(bool,QString,int)) );
	connect( newArch, SIGNAL(sigCreate(bool,QString,int)), this, SLOT(slotCreate(bool,QString,int)) );
	newArch->create();
	break;
#if 0
    case LHA_FORMAT:
	newArch = new LhaArch( m_data );
	newArch->createArch( name );
	ret = true;
	break;
    case AA_FORMAT:
	newArch = new ArArch( m_data );
	newArch->createArch( name );
	ret = true;
	break;
#endif
    default:
	KMessageBox::error(this, i18n("Unknown archive format or corrupted archive") );
//	clearCurrentArchive();     // just leave the old one.
    }

    return newArch;
}

Arch *ArkWidget::openArchive( QString _filename )
{
    // returns a pointer to the new archive unless there was a problem.
    // otherwise returns 0

    Arch *newArch = 0;

    switch( getArchType( _filename ) )
    {
#if 0

    case TAR_FORMAT:
	newArch = new TarArch(  m_data, this, _filename );
	newArch->openArch( name );
	ret = true;
	break;
#endif   
    case ZIP_FORMAT:
	newArch = new ZipArch( m_data, this, _filename );
	connect( newArch, SIGNAL(sigOpen(bool,QString,int)),
		 this, SLOT(slotOpen(bool,QString,int)) );
	connect( newArch, SIGNAL(sigCreate(bool,QString,int)),
		 this, SLOT(slotCreate(bool,QString,int)) );
	newArch->open();
	break;
#if 0
    case LHA_FORMAT:
	newArch = new LhaArch( m_data );
	newArch->openArch( name, archiveContent );
	ret = true;
	break;
    case AA_FORMAT:
	newArch = new ArArch( m_data );
	newArch->openArch( name, archiveContent );
	ret = true;
	break;
#endif
    default:
	KMessageBox::error( this, i18n("Unknown archive format or corrupted archive") );
//	clearCurrentArchive();     // just leave the old one
    }
    return newArch;    
}

