/* (c)1997 Robert Palmbos */
/* Warning:  Uncommented spaghetti code next 500 lines */
/* This is the main ark window widget */

// Qt includes
#include <qdragobject.h>
#include <qevent.h>
#include <qmessagebox.h>
#include <qdir.h>
#include <qcursor.h>

// KDE includes
#include <kapp.h>
#include <kconfig.h>
#include <kfiledialog.h>
#include <kkeydialog.h>
#include <kiconloader.h>
#include <klocale.h>
#include <krun.h>
#include <kstatusbar.h>
#include <ktoolbar.h>
#include <kwm.h>

// ark includes
#include "extractdlg.h"
#include "arkwidget.h"
#include "arkwidget.moc"
#include "dirDlg.h"
#include "generalOptDlg.h"
#include "zipExtractDlg.h"
#include <kglobal.h>

#define FILE_OPEN_XPM "fileopen.xpm"
#define FILE_CLOSE_XPM "exit.xpm"
#define EDIT_EXTRACT_XPM "viewzoom.xpm"
#define FAVORITE_XPM "home.xpm"

enum Buttons { OPEN_BUTTON= 1000, FAVORITE_BUTTON, EXTRACT_BUTTON,
	       CLOSE_BUTTON };

QList<ArkWidget> *ArkWidget::windowList = 0;

ArkWidget::ArkWidget( QWidget *, const char *name )
	: KTMainWindow( name )
{
	data = new ArkData();

	unsigned int pid = getpid();
	tmpdir.sprintf( "/tmp/ark.%d/", pid );

	if (!windowList)
	    windowList = new QList<ArkWidget>();

	windowList->setAutoDelete( FALSE );
	windowList->append( this );

	// Build the ark UI
	setupMenuBar();
	setupStatusBar();
	setupToolBar();
  	createFileListView();

        // enable DnD
        setAcceptDrops(true);
        
	setCaption( kapp->getCaption() );

	setMinimumSize( 300, 200 );  // someday this won't be hardcoded

	// Creates a temp directory for this ark instance
	QString ex( "mkdir " + tmpdir + " &>/dev/null" );
	system( ex.ascii() );

	arch=0;
	listing=0;
	contextRow = false;

	writeStatus( i18n("Welcome to ark...") );
}

ArkWidget::~ArkWidget()
{
	windowList->removeRef( this );
//	delete archiveContent;
	delete recentPopup;
	delete accelerators;
	delete pop;
	delete data;
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
	accelerators->insertItem(i18n("Select all"), "SelectionAll", "CTRL+A");
	accelerators->insertItem(i18n("Deselect all"), "DeselectionAll", "CTRL+D");
	accelerators->insertItem(i18n("Invert selection"), "InvertSel", "CTRL+I");

	accelerators->insertStdItem(KAccel::Help);

	// KAccel connections
	accelerators->connectItem(KAccel::New, this, SLOT(file_new()));
	accelerators->connectItem(KAccel::Open, this, SLOT(file_open()));
	accelerators->connectItem(KAccel::Close, this, SLOT(file_close()));
	accelerators->connectItem(KAccel::Quit, this, SLOT(file_quit()));

	accelerators->connectItem("Add_accel", this, SLOT(edit_add()));
	accelerators->connectItem("Delete_accel", this, SLOT(edit_delete()));
	accelerators->connectItem("Extract_accel", this, SLOT(edit_extract()));
	accelerators->connectItem("View_accel", this, SLOT(edit_view()));
	accelerators->connectItem("SelectionAll", this, SLOT(edit_selectAll()));
	accelerators->connectItem("DeselectionAll", this, SLOT(edit_deselectAll()));
	accelerators->connectItem("InvertSel", this, SLOT(edit_invertSel()));

	// KAccel settings
	accelerators->readSettings( data->getKConfig() );
	int id;

	// File menu creation
	QPopupMenu *fileMenu = new QPopupMenu;
	recentPopup = new QPopupMenu;
	editMenu = new QPopupMenu;
	QPopupMenu *optionsmenu = new QPopupMenu;

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

	fileMenu->insertSeparator();
	id=fileMenu->insertItem( i18n( "&Close"), this, SLOT( file_close() ) );
	accelerators->changeMenuAccel(fileMenu, id, KAccel::Close );

	id=fileMenu->insertItem( i18n( "&Quit"), this, SLOT( file_quit() ) );
	accelerators->changeMenuAccel(fileMenu, id, KAccel::Quit );

	// Edit menu creation
	idAdd=editMenu->insertItem( i18n( "&Add..."), this, SLOT( edit_add() ) );
	accelerators->changeMenuAccel(editMenu, idAdd, "Add_accel" );
	editMenu->setItemEnabled( idAdd, false );

	idDelete=editMenu->insertItem( i18n( "&Delete..."), this, SLOT( edit_delete() ) );
	accelerators->changeMenuAccel(editMenu, idDelete, "Delete_accel" );
	editMenu->setItemEnabled( idDelete, false );

	idExtract=editMenu->insertItem( i18n( "E&xtract..."), this, SLOT( edit_extract() ) );
	accelerators->changeMenuAccel(editMenu, idExtract, "Extract_accel" );
	editMenu->setItemEnabled( idExtract, false );

	idView=editMenu->insertItem( i18n( "&View..."), this, SLOT( edit_view() ) );
	accelerators->changeMenuAccel(editMenu, idView, "View_accel" );
	editMenu->setItemEnabled( idView, false );


	editMenu->insertSeparator();
	idSelectAll=editMenu->insertItem( i18n( "&Select all"), this, SLOT( edit_selectAll() ) );
	accelerators->changeMenuAccel(editMenu, id, "SelectionAll" );
	editMenu->setItemEnabled( idSelectAll, false );

	idDeselectAll=editMenu->insertItem( i18n( "Dese&lect all"), this, SLOT( edit_deselectAll() ) );
	accelerators->changeMenuAccel(editMenu, id, "DeselectionAll" );
	editMenu->setItemEnabled( idDeselectAll, false );

	idInvertSel=editMenu->insertItem( i18n( "&Invert selection"), this, SLOT( edit_invertSel() ) );
	accelerators->changeMenuAccel(editMenu, id, "InvertSel" );
	editMenu->setItemEnabled( idInvertSel, false );

//	idInvertSel=editMenu->insertItem( i18n( "Single selection"), this, SLOT( edit_invertSel() ) );
//	accelerators->changeMenuAccel(editMenu, id, "InvertSel" );
//	editMenu->setItemEnabled( idInvertSel, false );


	// Options menu creation
	optionsmenu->insertItem( i18n( "&General..."), this, SLOT( options_general() ) );
	optionsmenu->insertItem( i18n( "&Directories..."), this, SLOT( options_dirs() ) );
	optionsmenu->insertItem( i18n( "&Keys..."), this, SLOT( options_keys() ) );
	optionsmenu->insertItem( i18n( "&File Adding Options..."), this, SLOT( getAddOptions() ) );
//	optionsmenu->insertItem( i18n( "&Test dialog..."), this, SLOT( testdlg() ) );
//	optionsmenu->insertItem( i18n( "&Save options now..."), this, SLOT( options_saveNow() ) );
//	optionsmenu->insertItem( i18n( "Save options on e&xit..."), this, SLOT( options_saveOnExit() ) );

	// Help menu creation
	QString about_ark = i18n(
		"ark version %1 - the KDE archiver\n"
                "\n"
                "Copyright:\n"
                "1997-1999: Robert Palmbos <palm9744@kettering.edu>\n"
                "1999: Francois-Xavier Duranceau <duranceau@kde.org>\n")
		.arg( ARK_VERSION );
	QPopupMenu *helpmenu = kapp->getHelpMenu( true, about_ark );

	menu->insertItem( i18n( "&File"), fileMenu );
	menu->insertItem( i18n( "&Edit"), editMenu );
	menu->insertItem( i18n( "&Options"), optionsmenu );
	menu->insertSeparator();
	menu->insertItem( i18n( "&Help" ), helpmenu );
	setMenu( menu );

	pop = new KPopupMenu();
	pop->setTitle( i18n("File Operations") );
	pop->insertItem( i18n("Extract..."), this, SLOT( edit_extract() ) );
	pop->insertItem( i18n("View file"), this, SLOT( edit_view() ) );
	pop->insertSeparator();
	pop->insertItem( i18n("Delete file"), this, SLOT( edit_delete() ) );
}

void ArkWidget::createRecentPopup()
{
	// removes the current enries in the recent popup
	recentPopup->clear();

	QStrList *recentFiles = data->getRecentFiles();
	for (uint i=0; i<recentFiles->count(); i++)
	{
        	recentPopup->insertItem(recentFiles->at(i), -1, i);
	}
}

void ArkWidget::setupStatusBar()
{
	KStatusBar *sb = statusBar();
	sb->insertItem( "", 0 );

	statusBarTimer = new QTimer(this);
	connect(statusBarTimer, SIGNAL(timeout()), SLOT(timeout()));
}

void ArkWidget::setupToolBar()
{
	QPixmap pix;
	QString pixpath;
	KToolBar *tb = toolBar();

	tb->insertButton( ICON(FILE_OPEN_XPM), OPEN_BUTTON, SIGNAL( clicked() ), this, SLOT( file_open() ), TRUE, i18n("Open"));
	tb->insertButton( ICON(FAVORITE_XPM), FAVORITE_BUTTON, SIGNAL( clicked() ), this, SLOT( showFavorite() ), TRUE, i18n("Goto Archive Dir"));
	tb->insertButton( ICON(EDIT_EXTRACT_XPM), EXTRACT_BUTTON, SIGNAL( clicked() ), this, SLOT( edit_extract() ), FALSE, i18n("Extract"));
//	tb()->setItemEnabled( EXTRACT_BUTTON, false );

	tb->insertSeparator();
	tb->insertButton( ICON(FILE_CLOSE_XPM), CLOSE_BUTTON, SIGNAL( clicked() ), this, SLOT( file_close() ), TRUE, i18n("Close"));

	tb->setBarPos( KToolBar::Top );
}


//////////////////////////////////////////////////////////////////////
//
// ArkWidget slots
//
//////////////////////////////////////////////////////////////////////


// File menu /////////////////////////////////////////////////////////

void ArkWidget::file_new()
{
	int ret;

	QString file = KFileDialog::getSaveFileName(QString::null, data->getFilter());
	if( !file.isEmpty() )
	{
		createFileListView();
		ret = createArchive( file );
		if( ret )
			newCaption(file);
		else
		{
			arkError( i18n( "Can't create archive of that type") );
		}
	}
}

void ArkWidget::file_newWindow()
{
	ArkWidget *kw = new ArkWidget;
	kw->show();
}

void ArkWidget::file_open()
{
	QString file = KFileDialog::getOpenFileName(data->getOpenDir(), data->getFilter(), this);
	if( !file.isEmpty() )
	{
		cerr << "file selected: " << file.ascii() << "\n";

		showZip( file );
	}
}

void ArkWidget::file_openRecent(int i)
{
	QString filename = recentPopup->text(i);
	showZip( filename );
}

void ArkWidget::showZip( QString name )
{
	bool ret;

	createFileListView();

	archiverMode = true;

	cerr << "Chow Archive: " << name.ascii() << "\n";
	ret = openArchive( name );
	cerr << "openArchive returned " << ret << "\n";
	if( ret )
	{
		listing = (QStrList *)arch->getListing();
		newCaption( name );

		QFileInfo fi( name );
		QString path = fi.dirPath( true );
		data->setLastOpenDir( path );
	}else{
		arkError( i18n("Unknown archive format or corrupted archive") );
		clearCurrentArchive();
	}
}

void ArkWidget::file_close()
{
	if( windowList->count() < 2 )
	{
		file_quit();
	}else
		delete this;
}

void ArkWidget::closeEvent( QCloseEvent * )
{
	file_close();
}

void ArkWidget::file_quit()
{
	KConfig *config;

	config = kapp->getConfig();
        config->setGroup("ark");

	if( KWM::isMaximized(this->winId()) ){
		config->writeEntry( "MaxMode", KWM::maximizeMode(this->winId()) );
	}
	else{
		config->writeEntry( "MaxMode", -1 );
	}

	accelerators->writeSettings( data->getKConfig() );
	data->writeConfiguration();
	cerr << "configuration written\n";

	QString ex( "rm -rf "+tmpdir );
	system( ex.ascii() );
	delete this;
	kapp->quit();
}

// Edit menu /////////////////////////////////////////////////////////

void ArkWidget::edit_add()
{

}

void ArkWidget::edit_delete()
{
//	deleteFile( lb->currentItem() );
}


void ArkWidget::deleteFile( int /*pos*/ )
{
/*
	if( pos != -1 && arch )
	{
		arch->deleteFile( pos ); // This will be better for the future
		listing = (QStrList *)arch->getListing();
		lb->clear();
		setupHeaders();
		lb->appendStrList( listing );
	}
*/
}

void ArkWidget::edit_extract()
{
	if( arch == 0 )
		arkWarning( "extract should not be available here !");
	else
		arch->extraction();
}

void ArkWidget::edit_view()
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

	if( contextRow )  // Warning: ugly hack
		return;
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

// Options menu //////////////////////////////////////////////////////

void ArkWidget::options_general()
{
	GeneralDlg *gd = new GeneralDlg( data, this );
	gd->exec();
	delete gd;
}

void ArkWidget::options_dirs()
{
	DirDlg *dd = new DirDlg( data, this );
	dd->exec();
	delete dd;
}

void ArkWidget::options_keys()
{
	KKeyDialog::configureKeys(accelerators, this);
}

void ArkWidget::getAddOptions()
{
	if( arch )
	{
		AddOptionsDlg *afd = new AddOptionsDlg( this );
		if( afd->exec() )
		{
			data->setaddPath( afd->storeFullPath() );
			data->setonlyUpdate( afd->onlyUpdate() );
		}
		delete afd;
		afd = 0;
	}else{
		writeStatus(i18n( "Create or open an archive first"));
	}
}

// Help menu /////////////////////////////////////////////////////////

void ArkWidget::help()
{
	kapp->invokeHTMLHelp( "ark/index.html", "" );
}


// Service functions /////////////////////////////////////////////////

void ArkWidget::doPopup( QListViewItem * /*item*/ )
{
	cerr << "Entered doPopup\n";
	contextRow = true;
	//archiveContent->setCurrentItem( item );
	contextRow = false;

//	pop->popup( QCursor::pos(), KPM_FirstItem );
	cerr << "Exited doPopup\n";
	//pop.exec();
}

// Drag & Drop ////////////////////////////////////////////////////////

void ArkWidget::dragEnterEvent(QDragEnterEvent* event)
{
  event->accept(QUrlDrag::canDecode(event));
}

void ArkWidget::dropEvent(QDropEvent* event )
{
	QStrList dlist;
	QString url;
	QString file;
	bool opennew=false;

        if(!QUrlDrag::decode(event, dlist))
          return;
        
	if( !arch ){	/* No archive is currently loaded */
		const char *foo;
		url = dlist.at(0);
		file = url.right( url.length()-5 );
		foo = file.data();
		if( openArchive(file) )
		{
			opennew=true;
		}else{
			file_new();
		}
	}
	if( arch && !opennew )  /* An archive was open or we just created one */
	{		        /* so we add the url list in the current one */
		int retcode;
		retcode = arch->addFile( &dlist );
		if( !retcode )
		{
			listing = (QStrList *)arch->getListing();
			archiveContent->clear();
		} else {
			if( retcode == UNSUPDIR )
				arkWarning( i18n("Can't add directories with this archive type"));
			else
				arkError( i18n( "Error saving to archive"));
		}
	}

}


void ArkWidget::showFavorite()
{
	const QFileInfoList *flist;
	QDir *fav;
	QStrList *flisting = new QStrList;

	clearCurrentArchive();

	archiverMode = false;

//	delete archiveContent;
	createFileListView();

	archiveContent->addColumn( i18n("File") );
	archiveContent->addColumn( i18n("Size") );
	archiveContent->setColumnAlignment(1, AlignRight);
	archiveContent->setMultiSelection( false );

	fav = new QDir( data->getFavoriteDir() );
	if( !fav->exists() )
	{
		arkError( i18n("Archive directory does not exist."));
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

	QString caption;
	caption = i18n("ark - %1").arg(data->getFavoriteDir());
	setCaption( caption );

	listing = flisting;
	delete fav;
	delete flisting;

	writeStatus( i18n( "Archive Directory") );
}

/**
 * Writes a message in the status bar.
 * This message is visible during 5 seconds.
 */
void ArkWidget::writeStatus(const QString text)
{
	statusBarTimer->stop();
	statusBar()->changeItem(text, 0);
	statusBarTimer->start(5000,true);
}


void ArkWidget::clearCurrentArchive()
{
	if (!arch)
		delete arch;
	arch = 0;
	setCaption("ark");
	editMenu->setItemEnabled( idAdd, false );
	editMenu->setItemEnabled( idDelete, false );
	editMenu->setItemEnabled( idExtract, false );
	editMenu->setItemEnabled( idView, false );

	editMenu->setItemEnabled( idSelectAll, false );
	editMenu->setItemEnabled( idDeselectAll, false );
	editMenu->setItemEnabled( idInvertSel, false );

	toolBar()->setItemEnabled( EXTRACT_BUTTON, false );
}


void ArkWidget::arkWarning(const QString& msg)
{
        QMessageBox::warning(this, i18n("ark"), msg);
}

void ArkWidget::arkError(const QString& msg)
{
        QMessageBox::critical(this, i18n("ark"), msg);
}

void ArkWidget::timeout()
{
	statusBar()->changeItem("",0);
}

void ArkWidget::newCaption(const QString& filename){

	QString caption;
	caption = i18n("ark - %1").arg(filename);
	setCaption(caption);

//	editMenu->setItemEnabled( idAdd, true );
//	editMenu->setItemEnabled( idDelete, true );
	editMenu->setItemEnabled( idExtract, true );
//	editMenu->setItemEnabled( idView, true );

	editMenu->setItemEnabled( idSelectAll, true );
	editMenu->setItemEnabled( idDeselectAll, true );
	editMenu->setItemEnabled( idInvertSel, true );

	toolBar()->setItemEnabled( EXTRACT_BUTTON, true );

	data->addRecentFile(filename);
	createRecentPopup();
}

void ArkWidget::createFileListView()
{
	archiveContent = new FileListView(this);
	archiveContent->setMultiSelection(false);
	setView(archiveContent);
	updateRects();
	archiveContent->show();

	connect( archiveContent, SIGNAL( selectionChanged(QListViewItem*)), this, SLOT( doPopup(QListViewItem*) ) );
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


bool ArkWidget::createArchive( QString name )
{
	int ret;

	switch( getArchType( name ) )
	{
		case TAR_FORMAT:
		{
			arch = new TarArch( data );
			arch->createArch( name );
			ret = true;
		}
		case ZIP_FORMAT:
		{
			arch = new ZipArch( data );
			arch->createArch( name );
			ret = true;
		}
/*		case LHA_FORMAT:
		{
			arch = new LhaArch( data );
			arch->createArch( name );
			ret = true;
		}
		case AA_FORMAT:
		{
			arch = new ArArch( data );
			arch->createArch( name );
			ret = true;
		}
*/		default:
		{
			return false;
		}
	}
	return ret;
}

bool ArkWidget::openArchive( QString name )
{
	int ret;

	switch( getArchType( name ) )
	{
		case TAR_FORMAT:
		{
			arch = new TarArch(  data );
			arch->openArch( name, archiveContent );
			ret = true;
			break;
		}
		case ZIP_FORMAT:
		{
			arch = new ZipArch( data );
			arch->openArch( name, archiveContent );
			ret = true;
			break;
		}
/*		case LHA_FORMAT:
		{
			arch = new LhaArch( data );
			arch->openArch( name, archiveContent );
			ret = true;
			break;
		}
		case AA_FORMAT:
		{
			arch = new ArArch( data );
			arch->openArch( name, archiveContent );
			ret = true;
			break;
		}
*/		default:
		{
			ret = false;
			break;
		}
	}
	return ret;
}

void ArkWidget::testdlg()
{
	cerr << "Entered testdlg\n";
 	ZipExtractDlg zed( QString::null, this );
 	zed.exec();
}

