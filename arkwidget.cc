/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org

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
#include <kwm.h>

// ark includes
#include "arkwidget.h"
#include "arkwidget.moc"
#include "deleteDlg.h"
#include "dirDlg.h"
#include "generalOptDlg.h"
#include "selectDlg.h"
#include "shellOutputDlg.h"

enum Buttons { OPEN_BUTTON= 1000, FAVORITE_BUTTON, EXTRACT_BUTTON,
	       CLOSE_BUTTON };

QList<ArkWidget> *ArkWidget::windowList = 0;

ArkWidget::ArkWidget( QWidget *, const char *name ) : 
	KTMainWindow( name )
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
	    windowList = new QList<ArkWidget>();

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
	
	writeStatusMsg( i18n("Welcome to ark...") );

	kdebug(0, 1601, "-ArkWidget::ArkWidget");
}

ArkWidget::~ArkWidget()
{
	windowList->removeRef( this );
	delete archiveContent;
	delete recentPopup;
	delete accelerators;
	delete pop;
	delete m_data;
	delete statusBarTimer;
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
	QPopupMenu *fileMenu = new QPopupMenu;
	recentPopup = new QPopupMenu;
	editMenu = new QPopupMenu;
	actionMenu = new QPopupMenu;
	optionsMenu = new QPopupMenu;

	KMenuBar *menu = menuBar();

	createRecentPopup();

	fileMenu->insertItem( i18n( "New &Window..."), this, SLOT( file_newWindow() ) );
	fileMenu->insertSeparator();
	id=fileMenu->insertItem( i18n( "&New..." ), this, SLOT( file_new()) );
	accelerators->changeMenuAccel(fileMenu, id, KAccel::New );

	id=fileMenu->insertItem( i18n( "&Open..." ), this,  SLOT( file_open()) );
	accelerators->changeMenuAccel(fileMenu, id, KAccel::Open );
	id=fileMenu->insertItem( i18n( "Open &recent" ), recentPopup);
	connect(recentPopup, SIGNAL(activated(int)), SLOT(file_openRecent(int)));
	fileMenu->insertItem( i18n( "Relo&ad" ), this,  SLOT( file_reload()) );

	fileMenu->insertSeparator();
	id=fileMenu->insertItem( i18n( "&Close"), this, SLOT( file_close() ) );
	accelerators->changeMenuAccel(fileMenu, id, KAccel::Close );

	id=fileMenu->insertItem( i18n( "&Quit"), this, SLOT( file_quit() ) );
	accelerators->changeMenuAccel(fileMenu, id, KAccel::Quit );

	createEditMenu( false );
	
	// Options menu creation
	optionsMenu->insertItem( i18n( "&General..."), this, SLOT( options_general() ) );
	optionsMenu->insertItem( i18n( "&Directories..."), this, SLOT( options_dirs() ) );
	optionsMenu->insertItem( i18n( "&Keys..."), this, SLOT( options_keys() ) );
	optionsMenu->insertSeparator();
	optionsMenu->insertItem( i18n( "&Save settings now..."), this, SLOT( options_saveNow() ) );
	idSaveOnExit=optionsMenu->insertItem( i18n( "Save settings on e&xit..."), this, SLOT( options_saveOnExit() ) );
	optionsMenu->setItemChecked(idSaveOnExit, m_data->isSaveOnExitChecked());

	// Help menu creation
	QString about_ark = i18n(
		"ark version %1 - the KDE archiver\n"
                "\n"
                "Copyright:\n"
                "1997-1999: Robert Palmbos <palm9744@kettering.edu>\n"
                "1999: Francois-Xavier Duranceau <duranceau@kde.org>\n")
		.arg( ARK_VERSION );
	QPopupMenu *helpmenu = helpMenu( about_ark );

	menu->insertItem( i18n( "&File"), fileMenu );
	menu->insertItem( i18n( "&Edit"), editMenu );
	idActionMenu = menu->insertItem( i18n( "&Action"), actionMenu );
	menu->setItemEnabled(idActionMenu, false);
	menu->insertItem( i18n( "&Options"), optionsMenu );
	menu->insertSeparator();
	menu->insertItem( i18n( "&Help" ), helpmenu );

	setMenu( menu );

	pop = new KPopupMenu();
	pop->setTitle( i18n("File Operations") );
	pop->insertItem( i18n("Extract..."), this, SLOT( action_extract() ) );
	pop->insertItem( i18n("View file"), this, SLOT( action_view() ) );
	pop->insertSeparator();
	pop->insertItem( i18n("Delete file"), this, SLOT( action_delete() ) );
}

void ArkWidget::createEditMenu( bool _enabled )
{
	editMenu->clear();
	
	idSelect=editMenu->insertItem( i18n( "&Select..."), this, SLOT( edit_select() ) );
	accelerators->changeMenuAccel(editMenu, idSelect, "Selection" );
	editMenu->setItemEnabled( idSelect, _enabled );

	idSelectAll=editMenu->insertItem( i18n( "&Select all"), this, SLOT( edit_selectAll() ) );
	accelerators->changeMenuAccel(editMenu, idSelectAll, "SelectionAll" );
	editMenu->setItemEnabled( idSelectAll, _enabled );

	idDeselectAll=editMenu->insertItem( i18n( "Dese&lect all"), this, SLOT( edit_deselectAll() ) );
	accelerators->changeMenuAccel(editMenu, idDeselectAll, "DeselectionAll" );
	editMenu->setItemEnabled( idDeselectAll, _enabled );

	idInvertSel=editMenu->insertItem( i18n( "&Invert selection"), this, SLOT( edit_invertSel() ) );
	accelerators->changeMenuAccel(editMenu, idInvertSel, "InvertSel" );
	editMenu->setItemEnabled( idInvertSel, _enabled );

	editMenu->insertSeparator();
	
	idShellOutput=editMenu->insertItem( i18n( "&View last shell output..."), this, SLOT( edit_view_last_shell_output() ) );
	editMenu->setItemEnabled( idShellOutput, _enabled );
}

void ArkWidget::createActionMenu( int _flag )
{
	KASSERT(arch != 0, 0, 1601, "arch is empty !" );
	
//	int flag = arch->getEditFlag();
	
	actionMenu->clear();
	if( _flag & Arch::Add ){
		idAdd=actionMenu->insertItem( i18n( "&Add..."), this, SLOT( action_add() ) );
		accelerators->changeMenuAccel(actionMenu, idAdd, "Add_accel" );
	}
	else idAdd = -1;

        if( _flag & Arch::Delete ){
		idDelete=actionMenu->insertItem( i18n( "&Delete..."), this, SLOT( action_delete() ) );
		accelerators->changeMenuAccel(actionMenu, idDelete, "Delete_accel" );
	}
	else idDelete = -1;
	
        if( _flag & Arch::Extract ){
		idExtract=actionMenu->insertItem( i18n( "E&xtract..."), this, SLOT( action_extract() ) );
		accelerators->changeMenuAccel(actionMenu, idExtract, "Extract_accel" );
	}
	else idExtract = -1;
	
        if( _flag & Arch::View ){
		idView=actionMenu->insertItem( i18n( "&View..."), this, SLOT( action_view() ) );
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
	KStatusBar *sb = statusBar();
	sb->insertItem( "", 0 );

	statusBarTimer = new QTimer(this);
	connect(statusBarTimer, SIGNAL(timeout()), SLOT(slotStatusBarTimeout()));
}

void ArkWidget::setupToolBar()
{
	QPixmap pix;
	QString pixpath;
	KToolBar *tb = toolBar();

	tb->insertButton( BarIcon("fileopen"), OPEN_BUTTON, SIGNAL( clicked() ), this, SLOT( file_open() ), TRUE, i18n("Open"));
	tb->insertButton( BarIcon("home"), FAVORITE_BUTTON, SIGNAL( clicked() ), this, SLOT( showFavorite() ), TRUE, i18n("Goto Archive Dir"));
	tb->insertButton( BarIcon("viewzoom"), EXTRACT_BUTTON, SIGNAL( clicked() ), this, SLOT( action_extract() ), FALSE, i18n("Extract"));
//	tb()->setItemEnabled( EXTRACT_BUTTON, false );

	tb->insertSeparator();
	tb->insertButton( BarIcon("exit"), CLOSE_BUTTON, SIGNAL( clicked() ), this, SLOT( file_close() ), TRUE, i18n("Close"));

	tb->setBarPos( KToolBar::Top );
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

	if( KWM::isMaximized(this->winId()) ){
		kc->writeEntry( "MaxMode", KWM::maximizeMode(this->winId()) );
	}
	else{
		kc->writeEntry( "MaxMode", -1 );
	}

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
		createArchive( file );
	}
}

void ArkWidget::slotCreate( bool _success, QString _filename, int _flag )
{
	if( _success ){
		newCaption( _filename );
		createActionMenu( _flag );
	}
	else
		KMessageBox::error( this, i18n( "Can't create archive of that type") );
}

void ArkWidget::file_newWindow()
{
	ArkWidget *kw = new ArkWidget;
	kw->show();
}

void ArkWidget::file_open()
{
	QString file = KFileDialog::getOpenFileName(m_data->getOpenDir(), m_data->getFilter(), this);
	if( !file.isEmpty() )
		showZip( file );
}

void ArkWidget::file_openRecent(int i)
{
	QString filename = recentPopup->text(i);
	showZip( filename );
}

void ArkWidget::showZip( QString _filename )
{
	kdebug(0, 1601, "+ArkWidget::showZip");

	createFileListView();
	archiverMode = true;
	openArchive( _filename );

	kdebug(0, 1601, "-ArkWidget::showZip");
}

void ArkWidget::slotOpen( bool _success, QString _filename, int _flag )
{
	kdebug(0, 1601, "+ArkWidget::slotOpen");

	if( _success ){
		newCaption( _filename );
		createActionMenu( _flag );

		QFileInfo fi( _filename );
		QString path = fi.dirPath( true );
		m_data->setLastOpenDir( path );
	}
	else{
		archiveContent->clear();
		clearCurrentArchive();	
	}
	
	kdebug(0, 1601, "-ArkWidget::slotOpen");
}

void ArkWidget::file_reload()
{
	QString filename = arch->fileName();
	showZip( filename );
}

void ArkWidget::reload()
{
	file_reload();
}
	
void ArkWidget::file_close()
{
	if( windowList->count() < 2 )
	{
		saveProperties();
		kapp->quit();
	}else{
		saveProperties();
		delete this;
	}
}


void ArkWidget::closeEvent( QCloseEvent * )
{
	file_close();
}


void ArkWidget::file_quit()
{
//	delete this;
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

	KASSERT(!archiveContent->isSelectionEmpty(), 0, 1601, "Nothing to be removed !" );

	if( KMessageBox::questionYesNo(this, i18n("Do you really want to delete the selectionned items?")) == KMessageBox::Yes)
		arch->remove();

	kdebug(0, 1601, "-ArkWidget::action_delete");
}

void ArkWidget::action_extract()
{
	if( arch == 0 )
		arkWarning( "extract should not be available here !");
	else
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
	actionMenu->setItemEnabled( idDelete, !archiveContent->isSelectionEmpty() );
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

	writeStatusMsg( i18n( "Archive Directory") );
}

/**
 * Writes a message in the status bar.
 * This message is visible during 5 seconds.
 */
void ArkWidget::writeStatusMsg(const QString text)
{
	statusBarTimer->stop();
	statusBar()->changeItem(text, 0);
	statusBarTimer->start(5000,true);
}

void ArkWidget::clearStatusBar()
{
	statusBar()->changeItem("",0);
}

void ArkWidget::clearCurrentArchive()
{
	if (!archiveContent)
		archiveContent->clear();

	if (!arch)
		delete arch;
	arch = 0;
	
	setCaption("");
	
	createEditMenu( false );
	menuBar()->setItemEnabled(idActionMenu, false);
	
	toolBar()->setItemEnabled( EXTRACT_BUTTON, false );
}

void ArkWidget::arkWarning(const QString& msg)
{
        QMessageBox::warning(this, i18n("ark"), msg, i18n("OK"));
}

void ArkWidget::slotStatusBarTimeout()
{
	clearStatusBar();
}

void ArkWidget::newCaption(const QString& _filename){

	//TODO: add the item count in the statusbar

//	QString caption;
//	caption = i18n("ark - %1[%2 files]").arg(filename).arg(archiveContent->count());
	setCaption( _filename );

	createEditMenu( true );
	
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


void ArkWidget::createArchive( QString _filename )
{
	switch( getArchType( _filename ) )
	{
/*		case TAR_FORMAT:
		{
			arch = new TarArch( m_data, this, name );
			arch->createArch( name );
			ret = true;
			break;
		}
*/		case ZIP_FORMAT:
		{
			arch = new ZipArch( m_data, this, _filename );
			connect( arch, SIGNAL(sigOpen(bool,QString,int)), this, SLOT(slotOpen(bool,QString,int)) );
			connect( arch, SIGNAL(sigCreate(bool,QString,int)), this, SLOT(slotCreate(bool,QString,int)) );
			arch->create();
			break;
		}
/*		case LHA_FORMAT:
		{
			arch = new LhaArch( m_data );
			arch->createArch( name );
			ret = true;
			break;
		}
		case AA_FORMAT:
		{
			arch = new ArArch( m_data );
			arch->createArch( name );
			ret = true;
			break;
		}
*/		default:
		{
			KMessageBox::error( this, i18n("Unknown archive format or corrupted archive") );
			clearCurrentArchive();
		}
	}
}

void ArkWidget::openArchive( QString _filename )
{
	switch( getArchType( _filename ) )
	{
/*		case TAR_FORMAT:
		{
			arch = new TarArch(  m_data, this, _filename );
			arch->openArch( name );
			ret = true;
			break;
		}
*/		case ZIP_FORMAT:
		{
			arch = new ZipArch( m_data, this, _filename );
			connect( arch, SIGNAL(sigOpen(bool,QString,int)), this, SLOT(slotOpen(bool,QString,int)) );
			connect( arch, SIGNAL(sigCreate(bool,QString,int)), this, SLOT(slotCreate(bool,QString,int)) );
			arch->open();
			break;
		}
/*		case LHA_FORMAT:
		{
			arch = new LhaArch( m_data );
			arch->openArch( name, archiveContent );
			ret = true;
			break;
		}
		case AA_FORMAT:
		{
			arch = new ArArch( m_data );
			arch->openArch( name, archiveContent );
			ret = true;
			break;
		}
*/		default:
		{
			KMessageBox::error( this, i18n("Unknown archive format or corrupted archive") );
			clearCurrentArchive();
		}
	}
}

