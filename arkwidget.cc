/* (c)1997 Robert Palmbos
   See main.cc for license details */
/* Warning:  Uncommented spaghetti code next 500 lines */
/* This is the main ark window widget */
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <qaccel.h>
#include <qdir.h>
#include <qfont.h>
#include <qpopmenu.h>
#include <qpixmap.h>
#include <qmsgbox.h>
#include <qstrlist.h>
#include <qcursor.h>
#include <ktoolbar.h>
#include <kconfig.h>
#include <kapp.h>
#include <drag.h>
#include <ktopwidget.h>
#include <kstatusbar.h>
#include <klocale.h>
#include <kfm.h>
#include <kfiledialog.h>
#include <ktablistbox.h>
#include "extractdlg.h"
#include "karchive.h"
#include "arkwidget.h"
#include "errors.h"
#include "arkwidget.moc"

QList<ArkWidget> *ArkWidget::windowList = 0;

ArkWidget::ArkWidget( QWidget *, const char *name )
	: KTopLevelWidget( name )
{
	KConfig *config;

	unsigned int pid = getpid();
	tmpdir.sprintf( "/tmp/ark.%d/", pid );

	config = kapp->getConfig();
        config->setGroup("ark");
	QString fav_key;
	fav_key="ArchiveDirectory";
	fav_dir = config->readEntry( fav_key );
	if( fav_dir.isEmpty() )
		fav_dir = getenv( "HOME" );
	tar_exe = config->readEntry( QString("TarExe") );
	if( tar_exe.isEmpty() )
		tar_exe = "tar";

	if (!windowList)
	    windowList = new QList<ArkWidget>();

	windowList->setAutoDelete( FALSE );
	windowList->append( this );

	QAccel *a = new QAccel( this );
	//a->connectItem( a->insertItem( CTRL+Key_O ), this, SLOT(openZip()) );
	//a->connectItem( a->insertItem( CTRL+Key_N ), this, SLOT(createZip()) );
	//a->connectItem( a->insertItem( CTRL+Key_W ), this, SLOT(closeZip()) );
	//a->connectItem( a->insertItem( CTRL+Key_Q ), this, SLOT(quit()) );
	a->connectItem( a->insertItem( Key_F1 ), this, SLOT(help()) );
	a->connectItem( a->insertItem( CTRL+Key_H ), this, SLOT(showFavorite()) );
	//a->connectItem( a->insertItem( CTRL+Key_E ), this, SLOT(extractZip()) );
	QPopupMenu *filemenu = new QPopupMenu;
	filemenu->insertItem( i18n( "New &Window..."), this, SLOT( newWindow() ) );
	filemenu->insertSeparator();
	filemenu->insertItem( i18n( "&New..." ), this, SLOT( createZip()), CTRL+Key_N );
	filemenu->insertItem( i18n( "&Open..." ), this,  SLOT( openZip()), CTRL+Key_O );
	filemenu->insertSeparator();
	filemenu->insertItem( i18n( "&Extract To..."), this, SLOT( extractZip()), CTRL+Key_E );
	filemenu->insertSeparator();
	filemenu->insertItem( i18n( "&Close"), this, SLOT( closeZip() ), CTRL+Key_W );
	filemenu->insertItem( i18n( "&Quit"), this, SLOT( quit() ), CTRL+Key_Q );
	QPopupMenu *editmenu = new QPopupMenu;
	editmenu->insertItem( i18n( "E&xtract..."), this, SLOT( extractFile() ) );
	editmenu->insertItem( i18n( "&View file"), this, SLOT( showFile() ) );
	editmenu->insertSeparator();
	editmenu->insertItem( i18n( "&Delete file"), this, SLOT( deleteFile() ) );
	QPopupMenu *optionsmenu = new QPopupMenu;
	optionsmenu->insertItem( i18n( "&Set Archive Directory..."), this, SLOT( getFav() ) );
	optionsmenu->insertItem( i18n( "Set &Tar Executable..."), this, SLOT( getTarExe() ) );
	optionsmenu->insertItem( i18n( "&File Adding Options..."), this, SLOT( getAddOptions() ) );
	QPopupMenu *helpmenu = kapp->getHelpMenu( true, "ark v0.5\n (c) 1997 Robert Palmbos" );

	menu = new KMenuBar( this );
	menu->insertItem( i18n( "&File"), filemenu );
	menu->insertItem( i18n( "&Edit"), editmenu );
	menu->insertItem( i18n( "&Options"), optionsmenu );
	menu->insertSeparator();
	menu->insertItem( i18n( "&Help" ), helpmenu );
	setMenu( menu );

	pop = new KPopupMenu( i18n("File Operations") );
	// QPopupMenu pop;
	pop->insertItem( i18n("Extract..."), this, SLOT( extractFile() ) );
	pop->insertItem( i18n("View file"), this, SLOT( showFile() ) );
	pop->insertSeparator();
	pop->insertItem( i18n("Delete file"), this, SLOT( deleteFile() ) );

	QPixmap pix;
	QString pixpath;

	pixpath = kapp->kde_toolbardir().copy()+"/";
	tb = new KToolBar( this, "toolbar" );

	pix.load( pixpath+"fileopen.xpm" );
	tb->insertButton( pix, 0, SIGNAL( clicked() ), this, SLOT( openZip() ), TRUE, i18n("Open"));

	pix.load( pixpath+"home.xpm" );
	tb->insertButton( pix, 3, SIGNAL( clicked() ), this, SLOT( showFavorite() ), TRUE, i18n("Goto Archive Dir..."));

	pix.load( pixpath+"viewzoom.xpm" );
	tb->insertButton( pix, 1, SIGNAL( clicked() ), this, SLOT( extractZip() ), TRUE, i18n("Extract To.."));

	tb->insertSeparator();
	pix.load( pixpath+"exit.xpm" );
	tb->insertButton( pix, 2, SIGNAL( clicked() ), this, SLOT( closeZip() ), TRUE, i18n("Exit"));

	addToolBar( tb );
	tb->setBarPos( KToolBar::Top );
	enableToolBar( KToolBar::Show );

	sb = new KStatusBar( this );
	sb->insertItem( i18n( "Welcome to ark..." ), 0 );
	setStatusBar( sb );

	//f_main = new QFrame( this, "frame_0" );
	lb = new KTabListBox( this );
	lb->setSeparator( '\t' );
	QFont f( "courier", 12 );
	//lb->setFont( f );
	setView( lb );
	connect( lb, SIGNAL( highlighted(int, int) ), this, SLOT( showFile(int, int) ) );
	connect( lb, SIGNAL( popupMenu(int, int) ), this, SLOT( doPopup(int, int) ) );
	KDNDDropZone *dz = new KDNDDropZone( lb, DndURL );
	connect( dz, SIGNAL(dropAction(KDNDDropZone *)),SLOT( fileDrop(KDNDDropZone *)) );

	setCaption( kapp->getCaption() );

	this->resize( 600, 400 );  // someday this won't be hardcoded
	tb->show();
	lb->show();
	sb->show();
	menu->show();
	updateRects();

	kfm = new KFM;

	QString ex( "mkdir " + tmpdir + " &>/dev/null" );
	system( ex );
	arch=0;
	flisting=0;
	listing=0;
	fav=0;
	addonlynew = FALSE;
	storefullpath = FALSE;
	contextRow = false;
}

ArkWidget::~ArkWidget()
{
	windowList->removeRef( this );
	delete kfm;
	delete menu;
	delete sb;
	delete lb;
	delete tb;
}

void ArkWidget::saveProperties( KConfig *kc ) {
	QString loc_key( "CurrentLocation" );

	if( arch != 0 )
		kc->writeEntry( loc_key, arch->getName() );
	else
		if( listing != 0 )
			kc->writeEntry( loc_key, "Favorites" );
		else
			kc->writeEntry( loc_key, "None" );

	// I would prefer to just delete all the widgets, but kwm gets confused
	// if ark quits in the middle of session management
	QString ex( "rm -rf "+tmpdir );
	system( ex );
}

void ArkWidget::readProperties( KConfig *kc ) {
	QString startpoint;
	startpoint = kc->readEntry( "CurrentLocation" );

	if( startpoint == "Favorites" )
		showFavorite();
	else
		if( startpoint != "None" )
			showZip( startpoint );
}
void ArkWidget::newWindow()
{
	ArkWidget *kw = new ArkWidget;
	kw->show();
}

void ArkWidget::doPopup( int row, int col )
{
	contextRow = true;
	lb->setCurrentItem( row, col );
	contextRow = false;

	pop->popup( QCursor::pos(), KPM_FirstItem );
	//pop.exec();
}

void ArkWidget::createZip()
{
	int ret;
	if( arch )
	{
		lb->clear();
	}
	QString file = KFileDialog::getSaveFileName();
	if( !file.isEmpty() )
	{
		lb->clear();
		lb->repaint();
		arch = new KArchive(tar_exe);
		ret = arch->createArch( file );
		if( ret )
			sb->changeItem( file.data(), 0 );
		else
		{
			sb->changeItem( i18n( "Can't create archive of that type"), 0 );
			delete arch;
			arch = 0;
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
			arch->addPath( storefullpath );
			arch->onlyUpdate( addonlynew );
		}
		delete afd;
		afd = 0;
	}else{
		sb->changeItem( i18n( "Create or open an archive first"), 0 );
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
		url = dlist.at(0);
		file = url.right( url.length()-5 );
		arch = new KArchive(tar_exe);
		if( arch->openArch( file ) )
		{
			showZip( file );
			opennew=true;
		}else{
			//sb->changeItem( (char *)i18n( "Create or open an archive first"), 0 );
			delete arch;
			arch = 0;
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
			lb->clear();
			setupHeaders();
			lb->appendStrList( listing );
		} else {
			if( retcode == UNSUPDIR )
				sb->changeItem( i18n("Can't add directories with this archive type"), 0 );
			else
				sb->changeItem( i18n( "Error saving to archive"), 0 );
		}
	}

}


void ArkWidget::getFav()
{
    KDirDialog dd( fav_dir.data(), 0, "dirdialog" );
    dd.setCaption(i18n("Archive Dir"));
    if( dd.exec() && (! dd.selectedFile().isEmpty()) )
    {
        KConfig *config = kapp->getConfig();
        config->setGroup("ark");
        fav_dir = dd.selectedFile();
        config->writeEntry( "ArchiveDirectory", fav_dir );
    }
}

void ArkWidget::getTarExe()
{
       QString tmp;
       DlgLocation ld( i18n( "What runs GNU tar:"), tar_exe, this );
       if( ld.exec() )
       {
               tmp = ld.getText();
               if( !tmp.isNull() && !tmp.isEmpty() )
               {
                       KConfig *config;
                       config = kapp->getConfig();
                       tar_exe = tmp;
                       config->writeEntry( "TarExe", tar_exe );
               }
       }
/*
    The following doesn't work because the file "tar" without directory can't
    be found by KFileDialog... Hum.

    KFileDialog fd( tar_exe.data() );
    fd.setCaption(i18n("What runs GNU tar"));
    if( fd.exec() && (! fd.selectedFile().isEmpty()) )
    {
        KConfig *config = kapp->getConfig();
        config->setGroup("ark");
        tar_exe = fd.selectedFile();
        config->writeEntry( "TarExe", tar_exe );
    }
*/
}

void ArkWidget::openZip()
{
	QString name = KFileDialog::getOpenFileName();
	if( !name.isNull() )
		showZip( name );
}

void ArkWidget::showZip( QString name )
{
	bool ret;

	lb->clear();
	delete arch;
	arch = new KArchive(tar_exe);

	ret = arch->openArch( name );
	if( ret )
	{
		setupHeaders();
		listing = (QStrList *)arch->getListing();
		lb->appendStrList( listing );
		sb->changeItem( name.data(), 0 );
	}else{
		sb->changeItem( i18n( "Unknown archive format"), 0 );
		lb->repaint();
		delete arch;
		arch = 0;
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

	lb->clear();
	lb->setNumCols( 2 );
	lb->setColumn( 0, i18n( "Size" ), 80 );
	lb->setColumn( 1, i18n( "File" ), this->width()-80 );
	fav = new QDir( fav_dir );
	if( !fav->exists() )
	{
		sb->changeItem( i18n("Archive directory does not exist."), 0 );
		return;
	}
	flist = fav->entryInfoList();
	QFileInfoListIterator flisti( *flist );
	++flisti; // Skip . and ..
	++flisti;
	QString line;
	for( uint i=0; i < flist->count()-2; i++ )
	{
		line.sprintf( "%d\t%s",(flisti.current())->size(), (const char *)(flisti.current())->fileName() );
		flisting->append( line );
		++flisti;
	}
	listing = flisting;
	lb->appendStrList( listing );
	sb->changeItem( i18n( "Archive Directory"), 0 );
}

void ArkWidget::extractZip()
{
	QString dir, ex;

	if( arch == 0 )
		return;
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
				QMessageBox::warning( this, "Can't mkdir", "Unable to create destination directory", "OK" );
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

void ArkWidget::about()
{
	QMessageBox aboutmsg;
	aboutmsg.information( this, "ark", "ark v0.5\n (c) 1997 Robert Palmbos", "OK" );

}

void ArkWidget::aboutQt()
{
	QMessageBox aboutmsg;
	aboutmsg.aboutQt(this);
}

void ArkWidget::help()
{
	kapp->invokeHTMLHelp( "ark/index.html", "" );
}

void ArkWidget::quit()
{
	QString ex( "rm -rf "+tmpdir );
	system( ex );
	delete this;
	kapp->quit();
}

void ArkWidget::showFile()
{
	if( lb->currentItem() != -1 )
	{
		showFile( lb->currentItem() );
	}
}

void ArkWidget::showFile( int index, int col )
{
	QString tmp;
	QString tname;
	QString name;
	QString fullname;

	if( contextRow )  // Warning: ugly hack
		return;

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
}

void ArkWidget::resizeEvent( QResizeEvent *re )
{
	int col = lb->numCols()-1;
	int colpos;
	KTopLevelWidget::resizeEvent( re );
	lb->resize( lb->width(), lb->height() );
	if( lb->colXPos( col, &colpos ) )
		lb->setColumnWidth( col, ((this->width())-colpos) );
}

void ArkWidget::extractFile()
{
	extractFile( lb->currentItem() );
}

void ArkWidget::extractFile( int pos )
{
	int ret;
	QString tmp;
	QString fullname;
	 QString tname;
	if( pos != -1 )
	{
		ExtractDlg *gdest;
		if( !arch )
		{
			arch = new KArchive(tar_exe);
			tmp = listing->at( pos );
			tname = tmp.right( tmp.length() - (tmp.findRev('\t')+1) );
			fullname = fav->path();
			fullname+="/";
			fullname+=tname;
			ret = arch->openArch( fullname );
			if( ret )
			{
				lb->clear();
				sb->changeItem( listing->at(pos), 0 );
				setupHeaders();
				listing = (QStrList *)arch->getListing();
				lb->appendStrList( listing );
				gdest = new ExtractDlg( ExtractDlg::All );
			}else{
				sb->changeItem( i18n( "Unknown archive format"), 0 );
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
}


void ArkWidget::deleteFile()
{
	deleteFile( lb->currentItem() );
}

void ArkWidget::deleteFile( int pos )
{
	if( pos != -1 && arch )
	{
		arch->deleteFile( pos ); // This will be better for the future
		listing = (QStrList *)arch->getListing();
		lb->clear();
		setupHeaders();
		lb->appendStrList( listing );
	}
}

void ArkWidget::setupHeaders()
{
	char *h = strdup( arch->getHeaders() );
	char *hdrs=h;
	char *tmp;
	char *hdr;
	int i=0;
	int cols=0;
	while( h[i]!='\0' )
	{
		if( h[i] == '\t' )
			cols++;
		i++;
	}
	lb->setNumCols( cols );
	i=0;
	while( (tmp=strstr( hdrs, "\t" ))!=0 )
	{
		hdr=hdrs;
		hdrs=tmp+1;
		tmp[0]='\0';
		lb->setColumn( i, hdr, (lb->fontMetrics()).width( hdr )+20 );
		i++;
	}
	int twidth=0;
	for( int ii=0; ii<i-1; ii++ )
		twidth+=lb->cellWidth( ii );
	lb->setColumnWidth( i-1, ((this->width())-twidth) );  // Is there a way to set to fill to right initially
	free( h );
}
