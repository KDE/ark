/* (c)1997 Robert Palmbos */
/* Warning:  Uncommented spaghetti code next 500 lines */
/* This is the main ark window widget */
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

// Qt includes
#include <qdir.h>
#include <qpopupmenu.h>
#include <qpixmap.h>
#include <qstrlist.h>
#include <qcursor.h>
#include <qtimer.h>
#include <qstring.h>

// KDE includes
#include <kaccel.h>
#include <ktoolbar.h>
#include <kconfig.h>
#include <kapp.h>
#include <drag.h>
#include <ktmainwindow.h>
#include <kstatusbar.h>
#include <klocale.h>
#include <kfm.h>
#include <kfiledialog.h>
#include <ktablistbox.h>
#include <kmsgbox.h>
#include <kkeydialog.h>

#include "extractdlg.h"
#include "arkwidget.h"
#include "errors.h"
#include "arkwidget.moc"
#include "arkdata.h"
#include "kwm.h"
#include "filelistview.h"

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

	//connect( lb, SIGNAL( highlighted(int, int) ), this, SLOT( showFile(int, int) ) );
	//connect( lb, SIGNAL( popupMenu(int, int) ), this, SLOT( doPopup(int, int) ) );
//	connect( archiveContent, SIGNAL( rightButtonClicked(FileLVI *, QPoint &, int)), this, SLOT( doPopup(FileLVI *) ) );

	KDNDDropZone *dz = new KDNDDropZone( archiveContent, DndURL );
	connect( dz, SIGNAL(dropAction(KDNDDropZone *)),SLOT( fileDrop(KDNDDropZone *)) );

	setCaption( kapp->getCaption() );
	
	setMinimumSize( 600, 400 );  // someday this won't be hardcoded
	
	kfm = new KFM;

	// Creates a temp directory for this ark instance	
	QString ex( "mkdir " + tmpdir + " &>/dev/null" );
	system( ex );

	arch=0;
	flisting=0;
	listing=0;
	fav=0;
	addonlynew = FALSE;
	storefullpath = FALSE;
	contextRow = false;
	
	writeStatus( i18n("Welcome to ark...") );
}
	
ArkWidget::~ArkWidget()
{
	windowList->removeRef( this );
	delete kfm;
	delete archiveContent;
	delete recentPopup;
	delete editMenu;
	delete accelerators;
}

void ArkWidget::setupMenuBar()
{
	// KAccel initialization
	accelerators = new KAccel(this);

	accelerators->insertStdItem(KAccel::New, i18n("New"));
	accelerators->insertStdItem(KAccel::Open, i18n("Open"));
	accelerators->insertStdItem(KAccel::Close, i18n("Close"));
	accelerators->insertStdItem(KAccel::Quit, i18n("Quit"));
	accelerators->insertItem(i18n("Extract to"), "Extraction", "CTRL+E");
	accelerators->insertItem(i18n("Select all"), "SelectionAll", "CTRL+A");
	accelerators->insertItem(i18n("Deselect all"), "DeselectionAll", "CTRL+D");
	accelerators->insertStdItem(KAccel::Help);

	// KAccel connections
	accelerators->connectItem(KAccel::Quit, this, SLOT(quit()));
	accelerators->connectItem(KAccel::Close, this, SLOT(closeZip()));
	accelerators->connectItem(KAccel::New, this, SLOT(createZip()));
	accelerators->connectItem("Extraction", this, SLOT(extractZip()));
	accelerators->connectItem("SelectionAll", this, SLOT(selectAll()));
	accelerators->connectItem("DeselectionAll", this, SLOT(deselectAll()));
	accelerators->connectItem(KAccel::Open, this, SLOT(openZip()));
	
	// KAccel settings
	accelerators->readSettings( data->getKConfig() );
	int id;

	// File menu creation
	QPopupMenu *filemenu = new QPopupMenu;
	recentPopup = new QPopupMenu;
	editMenu = new QPopupMenu;
	QPopupMenu *optionsmenu = new QPopupMenu;

	KMenuBar *menu = menuBar();

	createRecentPopup();

	filemenu->insertItem( i18n( "New &Window..."), this, SLOT( newWindow() ) );
	filemenu->insertSeparator();
	id=filemenu->insertItem( i18n( "&New..." ), this, SLOT( createZip()) );
	accelerators->changeMenuAccel(filemenu, id, KAccel::New );

	id=filemenu->insertItem( i18n( "&Open..." ), this,  SLOT( openZip()) );
	accelerators->changeMenuAccel(filemenu, id, KAccel::Open );
	id=filemenu->insertItem( i18n( "Open &recent" ), recentPopup);
	connect(recentPopup, SIGNAL(activated(int)), SLOT(openRecent(int)));

	filemenu->insertSeparator();
	id=filemenu->insertItem( i18n( "&Extract To..."), this, SLOT( extractZip()) );
	accelerators->changeMenuAccel(filemenu, id, "Extraction" );

	filemenu->insertSeparator();
	id=filemenu->insertItem( i18n( "&Close"), this, SLOT( closeZip() ) );
	accelerators->changeMenuAccel(filemenu, id, KAccel::Close );

	id=filemenu->insertItem( i18n( "&Quit"), this, SLOT( quit() ) );
	accelerators->changeMenuAccel(filemenu, id, KAccel::Quit );

	// Edit menu creation
	editMenu->insertItem( i18n( "E&xtract..."), this, SLOT( extractFile() ) );
	editMenu->insertItem( i18n( "&View file"), this, SLOT( showFile() ) );
	editMenu->insertItem( i18n( "&Delete file"), this, SLOT( deleteFile() ) );
	editMenu->insertSeparator();
	id=editMenu->insertItem( i18n( "&Select all"), this, SLOT( selectAll() ) );
	accelerators->changeMenuAccel(editMenu, id, "SelectionAll" );

	id=editMenu->insertItem( i18n( "Dese&lect all"), this, SLOT( deselectAll() ) );
	accelerators->changeMenuAccel(editMenu, id, "DeselectionAll" );

	// Options menu creation
	optionsmenu->insertItem( i18n( "&Set Archive Directory..."), this, SLOT( getFav() ) );
	optionsmenu->insertItem( i18n( "Set &Tar Executable..."), this, SLOT( getTarExe() ) );
	optionsmenu->insertItem( i18n( "&File Adding Options..."), this, SLOT( getAddOptions() ) );
	optionsmenu->insertItem( i18n( "&Keys..."), this, SLOT( options_keyconf() ) );

	// Help menu creation
	QString about_ark;
	about_ark.sprintf(i18n(
		"ark version %s - the KDE archiver\n"
                "\n"
                "Copyright:\n" 
                "1997-1999: Robert Palmbos <palm9744@kettering.edu\n"
                "1999: Francois-Xavier Duranceau <duranceau@kde.org>\n"),
		ARK_VERSION );
	QPopupMenu *helpmenu = kapp->getHelpMenu( true, about_ark );

	menu->insertItem( i18n( "&File"), filemenu );
	menu->insertItem( i18n( "&Edit"), editMenu );
	menu->insertItem( i18n( "&Options"), optionsmenu );
	menu->insertSeparator();
	menu->insertItem( i18n( "&Help" ), helpmenu );
	setMenu( menu );

	pop = new KPopupMenu();
	pop->setTitle( i18n("File Operations") );
	pop->insertItem( i18n("Extract..."), this, SLOT( extractFile() ) );
	pop->insertItem( i18n("View file"), this, SLOT( showFile() ) );
	pop->insertSeparator();
	pop->insertItem( i18n("Delete file"), this, SLOT( deleteFile() ) );
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

	pixpath = kapp->kde_toolbardir().copy()+"/";

	pix.load( pixpath+"fileopen.xpm" );
	tb->insertButton( pix, 0, SIGNAL( clicked() ), this, SLOT( openZip() ), TRUE, i18n("Open"));

	pix.load( pixpath+"home.xpm" );
	tb->insertButton( pix, 3, SIGNAL( clicked() ), this, SLOT( showFavorite() ), TRUE, i18n("Goto Archive Dir..."));

	pix.load( pixpath+"viewzoom.xpm" );
	tb->insertButton( pix, 1, SIGNAL( clicked() ), this, SLOT( extractZip() ), TRUE, i18n("Extract To.."));
	
	tb->insertSeparator();
	pix.load( pixpath+"exit.xpm" );
	tb->insertButton( pix, 2, SIGNAL( clicked() ), this, SLOT( closeZip() ), TRUE, i18n("Exit"));

	tb->setBarPos( KToolBar::Top );
}

void ArkWidget::newWindow()
{
	ArkWidget *kw = new ArkWidget;
	kw->show();
}

void ArkWidget::doPopup( FileLVI *item )
{
	cerr << "Entered doPopup\n";
	contextRow = true;
	//archiveContent->setCurrentItem( item );
	contextRow = false;

	pop->popup( QCursor::pos(), KPM_FirstItem );
	cerr << "Exited doPopup\n";
	//pop.exec();
}

void ArkWidget::createZip()
{
	int ret;

	QString file = KFileDialog::getSaveFileName(QString::null, data->getFilter());
	if( !file.isEmpty() )
	{
		delete archiveContent;
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

void ArkWidget::getAddOptions()
{
	if( arch )
	{
		AddOptionsDlg *afd = new AddOptionsDlg( this );
		if( afd->exec() )
		{
			addonlynew = afd->onlyUpdate();
			storefullpath = afd->storeFullPath();
			data->setaddPath( storefullpath );
			data->setonlyUpdate( addonlynew );
		}
		delete afd;
		afd = 0;
	}else{
		writeStatus(i18n( "Create or open an archive first"));
	}
}
	

void ArkWidget::fileDrop( KDNDDropZone *dz )
{
	QStrList dlist;
	QString url;
	QString file;
	bool opennew=false;

	dlist = dz->getURLList();

	if( !arch ){
		const char *foo;
		url = dlist.at(0);
		file = url.right( url.length()-5 );
		foo = file.data();
//		arch = new KArchive(data->getTarCommand());
//		if( arch->openArch(file, archiveContent) )
		if( openArchive(file) )
		{
//			showZip( file );
			opennew=true;
		}else{
			//sb->changeItem( i18n( "Create or open an archive first"), 0 );
			//clearCurrentArchive();
			createZip();
		}
	}
	if( arch && !opennew )
	{
		int retcode;
		retcode = arch->addFile( &dlist );
		if( !retcode )
		{
			listing = (QStrList *)arch->getListing();
			archiveContent->clear();
			//lb->clear();
			//setupHeaders();
			//archiveContent->buildList( listing);
			//lb->appendStrList( listing );
		} else {
			if( retcode == UNSUPDIR )
				arkWarning( i18n("Can't add directories with this archive type"));
			else	
				arkError( i18n( "Error saving to archive"));
		}
	}

}


void ArkWidget::getFav()
{
    KDirDialog dd( data->getFavoriteDir().data(), 0, "dirdialog" );
    dd.setCaption(i18n("Archive Dir"));
    if( dd.exec() && (! dd.selectedFile().isEmpty()) )
    {
	data->setFavoriteDir(dd.selectedFile());
    }
}

void ArkWidget::getTarExe()
{
       QString tmp;
       DlgLocation ld( i18n( "What runs GNU tar:"), data->getTarCommand(), this );
       if( ld.exec() )
       {
               tmp = ld.getText();
               if( !tmp.isNull() && !tmp.isEmpty() )
               {
                       data->setTarCommand(tmp);
               }
       }
}

void ArkWidget::openZip()
{
	QString file = KFileDialog::getOpenFileName(QString::null, data->getFilter());
	if( !file.isNull() )
	{
		showZip( file );
		newCaption(file);
	}
}

void ArkWidget::openRecent(int i)
{
	QString filename = recentPopup->text(i);
	showZip( filename );

	//If showZip fails ??
	newCaption(filename);
}

void ArkWidget::showZip( QString name )
{
	bool ret;

	delete archiveContent;
	createFileListView();
	ret = openArchive( name );
	cerr << "openArchive returned " << ret << "\n";
	if( ret )
	{
		listing = (QStrList *)arch->getListing();
	}else{
		arkError( i18n("Unknown archive format") );
		clearCurrentArchive();
	}
}

void ArkWidget::showFavorite()
{
	const QFileInfoList *flist;
	delete fav;
	delete arch;
	
	delete flisting;
	flisting = new QStrList;
	arch = 0;
	
	archiveContent->clear();
	clearCurrentArchive();

	delete archiveContent;
	createFileListView();
	archiveContent->addColumn( i18n("File") );
	archiveContent->addColumn( i18n("Size") );

	fav = new QDir( data->getFavoriteDir() );
	if( !fav->exists() )
	{
		arkError( i18n("Archive directory does not exist."));
		return;
	}
	flist = fav->entryInfoList();
	QFileInfoListIterator flisti( *flist );
	++flisti; // Skip . and ..
	++flisti;
	QString size;
	for( uint i=0; i < flist->count()-2; i++ )
	{
		FileLVI *flvi = new FileLVI(archiveContent);
		flvi->setText(0, (flisti.current())->fileName());
                size.sprintf("%d", (flisti.current())->size());
		flvi->setText(1, size);
		archiveContent->insertItem(flvi);
		++flisti;
	}
	listing = flisting;
	
	writeStatus( i18n( "Archive Directory") );
}

void ArkWidget::extractZip()
{
	QString dir, ex;

	if( arch == 0 )
		return;
/*
	ExtractDlg ld( ExtractDlg::All );
	int mask = arch->setOptions( FALSE, FALSE, FALSE );
	ld.setMask( mask );
	if( ld.exec() )
	{
		dir = ld.getDest();
		if( dir.isNull() || dir=="" || arch==0 )
			return;
		QDir dest( dir );
		if( !dest.exists() ) {
			if( mkdir( (const char *)dir, S_IWRITE | S_IREAD | S_IEXEC ) ) {
				arkWarning( i18n("Unable to create destination directory") );
				return;
			}
		}
		arch->setOptions( ld.doPreservePerms(), ld.doLowerCase(), ld.doOverwrite() );
		switch( ld.extractOp() ) {
			case ExtractDlg::All: {
				arch->extractTo( dir );
				break;
			}
			case ExtractDlg::Selected: {
				if( lb->currentItem() == -1 )
					break;
				arch->unarchFile( lb->currentItem(), dir );
				break;
			}
			case ExtractDlg::Pattern: {
				break;
			}
		}
	}
*/
}

void ArkWidget::closeEvent( QCloseEvent * )
{
	closeZip();
}

void ArkWidget::closeZip()
{
	if( windowList->count() < 2 )
	{
		ArkWidget::quit();
	}else
		delete this;
}

void ArkWidget::help()
{
	kapp->invokeHTMLHelp( "ark/index.html", "" );
}

void ArkWidget::quit()	
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

	QString ex( "rm -rf "+tmpdir );
	system( ex );
	delete this;
	kapp->quit();
}

void ArkWidget::showFile()
{
/*
	if( lb->currentItem() != -1 )
	{
		showFile( lb->currentItem() );
	}
*/
}

void ArkWidget::showFile( int index, int col )
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
		kfm->exec( fullname, 0L );
	}
*/
}


void ArkWidget::extractFile()
{
//	extractFile( lb->currentItem() );
}

void ArkWidget::extractFile( int pos )
{
/*
	int ret;
	QString tmp;
	QString fullname;
	 QString tname;
	if( pos != -1 )
	{
		ExtractDlg *gdest;
		if( !arch )
		{
			arch = new KArchive(data->getTarCommand());
			tmp = listing->at( pos );
			tname = tmp.right( tmp.length() - (tmp.findRev('\t')+1) );
			fullname = fav->path();
			fullname+="/";
			fullname+=tname;
			ret = arch->openArch( fullname );
			if( ret )
			{
				lb->clear();
				writeStatus( listing->at(pos) );
				setupHeaders();
				listing = (QStrList *)arch->getListing();
				lb->appendStrList( listing );
				gdest = new ExtractDlg( ExtractDlg::All );
			}else{
				writeStatus( i18n( "Unknown archive format") );
				delete arch;
				arch = 0;
				return;
			}
		}
		else
			gdest = new ExtractDlg( ExtractDlg::Selected );
		QString name( listing->at( pos ) );
		int mask = arch->setOptions( FALSE, FALSE, FALSE );
		gdest->setMask( mask );
		if( gdest->exec() )
		{
			QString dest(  gdest->getDest() );
			arch->setOptions( gdest->doPreservePerms(), gdest->doLowerCase(), 
				gdest->doOverwrite() );
			switch( gdest->extractOp() ) {
				case ExtractDlg::All: {
					arch->extractTo( dest );
					break;
				}
				case ExtractDlg::Selected: {
					if( lb->currentItem() == -1 )
						break;
					arch->unarchFile( lb->currentItem(), dest );
					break;
				}
				case ExtractDlg::Pattern: {
					break;
				}
			}
		}
	}
*/
}
	

void ArkWidget::deleteFile()
{
//	deleteFile( lb->currentItem() );
} 


void ArkWidget::deleteFile( int pos )
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
}


void ArkWidget::arkWarning(const QString msg)
{
	KMsgBox::message(this, ARK_WARNING, msg);
}

void ArkWidget::arkError(const QString msg)
{
	KMsgBox::message(this, ARK_ERROR, msg, KMsgBox::STOP);
}

void ArkWidget::timeout()
{
	statusBar()->changeItem("",0);
}


void ArkWidget::options_keyconf()
{
	KKeyDialog::configureKeys(accelerators);
}

void ArkWidget::newCaption(const QString& filename){
	
	QString caption;
	caption.sprintf(i18n("ark - %s"), filename.data());
	setCaption(caption);

	data->addRecentFile(filename);
	createRecentPopup();
}

void ArkWidget::createFileListView()
{
	archiveContent = new FileListView(this);                	
	archiveContent->setMultiSelection(true);
	setView(archiveContent);
	updateRects();
	archiveContent->show();
}

void ArkWidget::selectAll()
{
	FileLVI * flvi = (FileLVI*)archiveContent->firstChild();

	while (flvi)
	{
        	archiveContent->setSelected(flvi, true);
		flvi = (FileLVI*)flvi->itemBelow();
	}
}

void ArkWidget::deselectAll()
{
	archiveContent->clearSelection();
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
			return new LhaArch;
			break;
		}
		case AA_FORMAT:
		{
			return new ArArch;
			break;
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
			return new LhaArch;
			break;
		}
		case AA_FORMAT:
		{
			return new ArArch;
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