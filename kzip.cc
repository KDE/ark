/* (c)1997 Robert Palmbos
   See main.cc for license details */
/* This is the main kzip window widget */
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <qaccel.h>
#include <qfont.h>
#include <qpopmenu.h>
#include <qpixmap.h>
#include <qmsgbox.h>
#include <qfiledlg.h>
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
#include "ktablistbox.h"
#include "extractdlg.h"
#include "karch.h"
#include "kzip.h"
#include "errors.h"
#include "kzip.moc"

QList<KZipWidget> KZipWidget::windowList;

KZipWidget::KZipWidget( QWidget *, const char *name )
	: KTopLevelWidget( name )
{
	KConfig *config;
	
	unsigned int pid = getpid();
	tmpdir.sprintf( "/tmp/kzip.%d/", pid );

	config = kapp->getConfig();
	QString fav_key;
	fav_key="ArchiveDirectory";
	fav_dir = config->readEntry( fav_key );
	if( fav_dir.isEmpty() )
		fav_dir = getenv( "HOME" );

	windowList.setAutoDelete( FALSE );
	windowList.append( this );

	QAccel *a = new QAccel( this );
	//a->connectItem( a->insertItem( CTRL+Key_O ), this, SLOT(openZip()) );
	//a->connectItem( a->insertItem( CTRL+Key_N ), this, SLOT(createZip()) );
	//a->connectItem( a->insertItem( CTRL+Key_W ), this, SLOT(closeZip()) );
	//a->connectItem( a->insertItem( CTRL+Key_Q ), this, SLOT(quit()) );
	a->connectItem( a->insertItem( Key_F1 ), this, SLOT(help()) );
	a->connectItem( a->insertItem( CTRL+Key_H ), this, SLOT(showFavorite()) );
	//a->connectItem( a->insertItem( CTRL+Key_E ), this, SLOT(extractZip()) );
	QPopupMenu *filemenu = new QPopupMenu;
	filemenu->insertItem( klocale->translate( "New &Window..."), this, SLOT( newWindow() ) );
	filemenu->insertSeparator();
	filemenu->insertItem( klocale->translate( "&New..." ), this, SLOT( createZip()), CTRL+Key_N );
	filemenu->insertItem( klocale->translate( "&Open..." ), this,  SLOT( openZip()), CTRL+Key_O );
	filemenu->insertSeparator();
	filemenu->insertItem( klocale->translate( "&Extract To..."), this, SLOT( extractZip()), CTRL+Key_E );
	filemenu->insertSeparator();
	filemenu->insertItem( klocale->translate( "&Close"), this, SLOT( closeZip() ), CTRL+Key_W );
	filemenu->insertItem( klocale->translate( "&Quit"), this, SLOT( quit() ), CTRL+Key_Q );
	QPopupMenu *editmenu = new QPopupMenu;
	editmenu->insertItem( klocale->translate( "E&xtract..."), this, SLOT( extractFile() ) );
	editmenu->insertItem( klocale->translate( "&View"), this, SLOT( showFile() ) );
	editmenu->insertSeparator();
	editmenu->insertItem( klocale->translate( "&Delete"), this, SLOT( deleteFile() ) );
	QPopupMenu *optionsmenu = new QPopupMenu;
	optionsmenu->insertItem( klocale->translate( "&Set Archive Directory..."), this, SLOT( getFav() ) );
	optionsmenu->insertItem( klocale->translate( "&File Adding Options..."), this, SLOT( getAddOptions() ) );
	QPopupMenu *helpmenu = kapp->getHelpMenu( true, "KZip v0.5\n (c) 1997 Robert Palmbos" );
	//QPopupMenu *helpmenu = new QPopupMenu;
	//helpmenu->insertItem( klocale->translate( "&Contents..." ), this, SLOT( help() ) );
	//helpmenu->insertSeparator();
	//helpmenu->insertItem( klocale->translate( "About &Qt..." ), this, SLOT(aboutQt()) );
	//helpmenu->insertItem( klocale->translate( "&About..." ), this, SLOT( about() ) );
	menu = new KMenuBar( this );
	menu->insertItem( klocale->translate( "&File"), filemenu );
	menu->insertItem( klocale->translate( "&Edit"), editmenu );
	menu->insertItem( klocale->translate( "&Options"), optionsmenu );
	menu->insertSeparator();
	menu->insertItem( klocale->translate( "&Help" ), helpmenu );
	setMenu( menu );

	pop = new KPopupMenu( "File Operations" );
	// QPopupMenu pop;
	pop->insertItem( "Extract...", this, SLOT( extractFile() ) );
	pop->insertItem( "View", this, SLOT( showFile() ) );
	pop->insertSeparator();
	pop->insertItem( "Delete", this, SLOT( deleteFile() ) );

	QPixmap pix;
	QString pixpath;
	
	pixpath = kapp->kdedir() + QString("/share/toolbar/");
	tb = new KToolBar( this, "toolbar" );

	pix.load( pixpath+"fileopen.xpm" );
	tb->insertButton( pix, 0, SIGNAL( clicked() ), this, SLOT( openZip() ), TRUE, "Open" );

	pix.load( pixpath+"home.xpm" );
	tb->insertButton( pix, 3, SIGNAL( clicked() ), this, SLOT( showFavorite() ), TRUE, "Goto Archive Dir..." );

	pix.load( pixpath+"viewzoom.xpm" );
	tb->insertButton( pix, 1, SIGNAL( clicked() ), this, SLOT( extractZip() ), TRUE, "Extract To.." );
	
	tb->insertSeparator();
	pix.load( pixpath+"exit.xpm" );
	tb->insertButton( pix, 2, SIGNAL( clicked() ), this, SLOT( closeZip() ), TRUE, "Exit" );

	addToolBar( tb );
	tb->setBarPos( KToolBar::Top );
	enableToolBar( KToolBar::Show );

	sb = new KStatusBar( this );
	sb->insertItem( (char *)klocale->translate( "Welcome to KZip..." ), 0 );
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
	
KZipWidget::~KZipWidget()
{
	windowList.removeRef( this );
	delete kfm;
	delete menu;
	delete sb;
	delete lb;
	delete tb;
}

void KZipWidget::saveProperties( KConfig *kc ) {
	QString loc_key( "CurrentLocation" );
	
	if( arch != 0 )
		kc->writeEntry( loc_key, arch->getName() );
	else
		if( listing != 0 )
			kc->writeEntry( loc_key, "Favorites" );
		else
			kc->writeEntry( loc_key, "None" );
	
	// I would prefer to just delete all the widgets, but kwm gets confused
	// if kzip quits in the middle of session management
	QString ex( "rm -rf "+tmpdir );
	system( ex );
}

void KZipWidget::readProperties( KConfig *kc ) {
	QString startpoint;
	startpoint = kc->readEntry( "CurrentLocation" );
	
	if( startpoint == "Favorites" )
		showFavorite();
	else
		if( startpoint != "None" )
			showZip( startpoint );
}
void KZipWidget::newWindow()
{
	KZipWidget *kw = new KZipWidget;
	kw->show();
}

void KZipWidget::doPopup( int row, int col )
{
	contextRow = true;
	lb->setCurrentItem( row, col );
	contextRow = false;

	pop->popup( QCursor::pos(), KPM_FirstItem );
	//pop.exec();
}

void KZipWidget::createZip()
{
	int ret;
	if( arch )
	{
		lb->clear();
	}
	QString file = QFileDialog::getSaveFileName();
	if( !file.isEmpty() )
	{
		lb->clear();
		lb->repaint();
		arch = new KZipArch;
		ret = arch->createArch( file );	
		if( ret )
			sb->changeItem( file.data(), 0 );
		else
		{
			sb->changeItem( (char *)klocale->translate( "Can't create archive of that type"), 0 );
			delete arch;
		}
	}
}

void KZipWidget::getAddOptions()
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
		sb->changeItem((char *) klocale->translate( "Create or open an archive first"), 0 );
	}
}
	

void KZipWidget::fileDrop( KDNDDropZone *dz )
{
	QStrList dlist;
	QString url;
	QString file;

	dlist = dz->getURLList();
	if( arch )
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
				sb->changeItem( (char *)klocale->translate("Can't add directorys with this archive type"), 0 );
			else	
				sb->changeItem( (char *)klocale->translate( "Error saving to archive"), 0 );
		}
	}
	else
	{
		char *foo;
		url = dlist.at(0);
		file = url.right( url.length()-5 );
		foo = file.data();
		if( foo[strlen(foo)-1] == '/' )
		{
			sb->changeItem( (char *)klocale->translate( "Create or open an archive first"), 0 );
			return;
		}
		arch = new KZipArch;
		if( arch->openArch( file ) )
			showZip( file );
		else
		{
			sb->changeItem( (char *)klocale->translate( "Create or open an archive first"), 0 );
			delete arch;
			arch = 0;
		}
	}
}


void KZipWidget::getFav()
{
	QString tmp;
	DlgLocation ld( klocale->translate( "Archive Dir:"), "", this );
	if( ld.exec() )
	{
		tmp = ld.getText();
		if( !tmp.isNull() && !tmp.isEmpty() )
		{
			KConfig *config;
			config = kapp->getConfig();
			fav_dir = tmp;
			config->writeEntry( "ArchiveDirectory", fav_dir );
		}
	}
}

void KZipWidget::openZip()
{
	QString name = QFileDialog::getOpenFileName();
	if( !name.isNull() ) 
		showZip( name ); 
}

void KZipWidget::showZip( QString name )
{
	bool ret;

	lb->clear();
	delete arch;
	arch = new KZipArch;

	ret = arch->openArch( name );
	if( ret )
	{
		setupHeaders();
		listing = (QStrList *)arch->getListing();
		lb->appendStrList( listing );
		sb->changeItem( name.data(), 0 );
	}else{
		sb->changeItem( (char *)klocale->translate( "Unknown archive format"), 0 );
		lb->repaint();
		delete arch;
		arch = 0;
	}
}

void KZipWidget::showFavorite()
{
	const QFileInfoList *flist;
	
	delete fav;
	delete arch;
	
	delete flisting;
	flisting = new QStrList;
	arch = 0;
	
	lb->clear();
	lb->setNumCols( 2 );
	lb->setColumn( 0, klocale->translate( "Size" ), 80 );
	lb->setColumn( 1, klocale->translate( "File" ), 180 );
	fav = new QDir( fav_dir );
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
	sb->changeItem( (char *)klocale->translate( "Archive Directory"), 0 );
}

void KZipWidget::extractZip()
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
				QMessageBox::warning( this, "Can't mkdir", "Unable to create destination directory", "Ok" );
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

void KZipWidget::closeEvent( QCloseEvent * )
{
	closeZip();
}

void KZipWidget::closeZip()
{
	if( windowList.count() < 2 )
	{
		KZipWidget::quit();
	}else
		delete this;
}

void KZipWidget::about()
{
	QMessageBox aboutmsg;
	aboutmsg.information( this, "Zip", "KZip v0.5\n (c) 1997 Robert Palmbos", "Ok" );
	
}

void KZipWidget::aboutQt()
{
	QMessageBox aboutmsg;
	aboutmsg.aboutQt(this);
}

void KZipWidget::help()
{
	kapp->invokeHTMLHelp( "kzip/index.html", "" );
}

void KZipWidget::quit()	
{
	QString ex( "rm -rf "+tmpdir );
	system( ex );
	delete this;
	kapp->quit();
}

void KZipWidget::showFile()
{
	if( lb->currentItem() != -1 )
	{
		showFile( lb->currentItem() );
	}
}

void KZipWidget::showFile( int index, int col )
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

void KZipWidget::resizeEvent( QResizeEvent *re )
{
	KTopLevelWidget::resizeEvent( re );
	lb->resize( lb->width(), lb->height() );
}

void KZipWidget::extractFile()
{
	extractFile( lb->currentItem() );
}

void KZipWidget::extractFile( int pos )
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
			arch = new KZipArch;
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
				sb->changeItem( (char *)klocale->translate( "Unknown archive format"), 0 );
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
	

void KZipWidget::deleteFile()
{
	deleteFile( lb->currentItem() );
} 

void KZipWidget::deleteFile( int pos )
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

void KZipWidget::setupHeaders()
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
