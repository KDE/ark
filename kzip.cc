/* This is the main kzip window widget */
#include <stdio.h>
#include <stdlib.h>
#include <qfont.h>
#include <qpopmenu.h>
#include <qpixmap.h>
#include <qmsgbox.h>
#include <qfiledlg.h>
#include <qstrlist.h>
#include <ktoolbar.h>
#include <kconfig.h>
#include <kapp.h>
#include <drag.h>
#include <ktopwidget.h>
#include <kstatusbar.h>
#include <klocale.h>
#include "ktablistbox.h"
#include "extractdlg.h"
#include "karch.h"
#include "kzip.h"
#include "kzip.moc"

KZipWidget::KZipWidget( QWidget *, const char *name )
	: KTopLevelWidget( name )
{
	KConfig *config;

	unsigned int pid = getpid();
	tmpdir.sprintf( "/tmp/kzip.%d/", pid );
	printf( "%s\n", (const char *)tmpdir );

	config = kapp->getConfig();
	QString fav_key;
	fav_key="ArchiveDirectory";
	fav_dir = config->readEntry( fav_key );
	if( fav_dir.isEmpty() )
		fav_dir = getenv( "HOME" );

	QPopupMenu *filemenu = new QPopupMenu;
	filemenu->insertItem( klocale->translate( "&New" ), this, SLOT( createZip()) );
	filemenu->insertItem( klocale->translate( "&Open..." ), this,  SLOT( openZip()) );
	filemenu->insertSeparator();
	filemenu->insertItem( klocale->translate( "&Extract To..."), this, SLOT( extractZip()) );
	filemenu->insertSeparator();
	filemenu->insertItem( klocale->translate( "&Close"), this, SLOT( closeZip() ) );
	filemenu->insertItem( klocale->translate( "E&xit"), this, SLOT( quit() ) );
	QPopupMenu *editmenu = new QPopupMenu;
	editmenu->insertItem( klocale->translate( "E&xtract..."), this, SLOT( extractFile() ) );
	editmenu->insertItem( klocale->translate( "&View"), this, SLOT( showFile() ) );
	editmenu->insertSeparator();
	editmenu->insertItem( klocale->translate( "&Delete"), this, SLOT( deleteFile() ) );
	QPopupMenu *optionsmenu = new QPopupMenu;
	optionsmenu->insertItem( klocale->translate( "&Set Archive Directory..."), this, SLOT( getFav() ) );
	optionsmenu->insertItem( klocale->translate( "&File Adding Options..."), this, SLOT( getAddOptions() ) );
	QPopupMenu *helpmenu = new QPopupMenu;
	helpmenu->insertItem( klocale->translate( "&Help..." ), this, SLOT( help() ) );
	helpmenu->insertSeparator();
	helpmenu->insertItem( klocale->translate( "About &Qt..." ), this, SLOT(aboutQt()) );
	helpmenu->insertItem( klocale->translate( "&About..." ), this, SLOT( about() ) );
	menu = new KMenuBar( this );
	menu->insertItem( klocale->translate( "&File"), filemenu );
	menu->insertItem( klocale->translate( "&Edit"), editmenu );
	menu->insertItem( klocale->translate( "&Options"), optionsmenu );
	menu->insertSeparator();
	menu->insertItem( klocale->translate( "&Help" ), helpmenu );
	setMenu( menu );

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
	tb->insertButton( pix, 2, SIGNAL( clicked() ), this, SLOT( quit() ), TRUE, "Exit" );

	addToolBar( tb );
	tb->setBarPos( KToolBar::Top );
	enableToolBar( KToolBar::Show );

	sb = new KStatusBar( this );
	sb->insertItem( (char *)klocale->translate( "Welcome to KZip..." ), 0 );
	setStatusBar( sb );

	f_main = new QFrame( this, "frame_0" );
	lb = new KTabListBox( f_main );
	lb->setSeparator( '\t' );
	QFont f( "courier", 12 );
	//lb->setFont( f );
	setView( f_main );
	connect( lb, SIGNAL( selected(int, int) ), this, SLOT( showFile(int, int) ) );
	KDNDDropZone *dz = new KDNDDropZone( lb, DndURL );
	connect( dz, SIGNAL(dropAction(KDNDDropZone *)),SLOT( fileDrop(KDNDDropZone *)) );

	setCaption( kapp->getCaption() );
	
	tb->show();
	f_main->show();
	sb->show();
	menu->show();
	updateRects();
	
	QString ex( "mkdir " + tmpdir );
	system( ex );
	arch=NULL;
	flisting=NULL;
	listing=NULL;
	fav=NULL;
	addonlynew = FALSE;
	storefullpath = FALSE;
}
	
KZipWidget::~KZipWidget()
{
	delete menu;
	delete sb;
	delete lb;
	delete tb;
	QString ex( "rm -r "+tmpdir );
	system( ex );
}

void KZipWidget::createZip()
{
	int ret;
	if( arch )
		closeZip();
	QString file = QFileDialog::getSaveFileName();
	if( !file.isEmpty() )
	{
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
		afd = NULL;
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
		}
		else
			sb->changeItem( (char *)klocale->translate( "Error saving to archive"), 0 );
	}
	else
	{
		char *foo;
		arch = new KZipArch;
		url = dlist.at(0);
		file = url.right( url.length()-5 );
		foo = file.data();
		if( foo[strlen(foo)-1] == '/' )
		{
			sb->changeItem( (char *)klocale->translate( "Create or open an archive first"), 0 );
			return;
		}
		if( arch->openArch( file ) )
			showZip( file );
		else
		{
			sb->changeItem( (char *)klocale->translate( "Create or open an archive first"), 0 );
			delete arch;
			arch = NULL;
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
	delete arch;
	arch = new KZipArch;
	lb->clear();

	ret = arch->openArch( name );
	if( ret )
	{
		setupHeaders();
		listing = (QStrList *)arch->getListing();
		lb->clear();
		lb->appendStrList( listing );
		sb->changeItem( name.data(), 0 );
	}else{
		lb->setNumCols( 0 );
		lb->clear();
		sb->changeItem( (char *)klocale->translate( "Unknown archive format"), 0 );
		delete arch;
		arch = NULL;
	}
}

void KZipWidget::showFavorite()
{
	const QFileInfoList *flist;
	
	delete fav;
	delete arch;
	
	delete flisting;
	flisting = new QStrList;
	arch = NULL;
	
	lb->clear();
	lb->setNumCols( 2 );
	lb->setColumn( 0, "Size", 80 );
	lb->setColumn( 1, "File", 180 );
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
	QString dir;

	if( arch == NULL )
		return;
	ExtractDlg ld( ExtractDlg::All );
	int mask = arch->setOptions( FALSE, FALSE, FALSE );
	ld.setMask( mask );
	if( ld.exec() )
	{
		dir = ld.getDest();
		if( dir.isNull() || dir=="" || arch==NULL )
			return;
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

void KZipWidget::closeZip()
{
	quit();
	delete arch;	
	arch = NULL;
	lb->clear();
	lb->setNumCols(0);
	sb->changeItem( (char *)klocale->translate( "None"), 0 );
}

void KZipWidget::about()
{
	QMessageBox aboutmsg;
	aboutmsg.information( this, "Zip", "KZip v0.4\n (c) 1997 Robert Palmbos", "Ok" );
	
}

void KZipWidget::aboutQt()
{
	QMessageBox aboutmsg;
	aboutmsg.aboutQt(this);
}

void KZipWidget::help()
{
	kapp->invokeHTMLHelp( "kzip/kzip.html", "" );
}

void KZipWidget::quit()	
{
	delete arch;
	kapp->quit();
}

void KZipWidget::showFile()
{
	if( lb->currentItem() != -1 )
	{
		showFile( lb->currentItem() );
	}
}

void KZipWidget::showFile( int index, int col=0  )
{
	QString tmp;
	QString tname;
	QString name;
	QString fullname;
	DlgLocation prog( klocale->translate( "Open With: (program)" ), "", this );
	
	col++; // Don't ask.
	tmp = listing->at( index );
	tname = tmp.right( tmp.length() - (tmp.findRev('\t')+1) );
	
	if( arch == NULL )
	{
		fullname = fav->path();
		fullname+= "/";
		fullname+= tname;
		showZip( fullname );
	}else{
		if( prog.exec() )
		{
			fullname = arch->unarchFile( index, tmpdir );
			name = prog.getText();
			if( name.isNull() || name.isEmpty() )
				return;
			name += " ";
			name += fullname;
			name += " &";
			system( (const char *)name );
		}
	}
}

void KZipWidget::resizeEvent( QResizeEvent *re )
{
	KTopLevelWidget::resizeEvent( re );
	lb->resize( f_main->width(), f_main->height() );
}

void KZipWidget::extractFile()
{
	int ret;
	QString tmp;
	QString name;
	int pos = lb->currentItem();
	if( pos != -1 )
	{
		ExtractDlg *gdest;
		if( !arch )
		{
			arch = new KZipArch;
			tmp = listing->at(pos);
			name = fav->path();
			name+="/";
			name+=tmp;
			puts( (const char *)name );
			ret = arch->openArch( name );
			if( ret )
			{
				sb->changeItem( listing->at(pos), 0 );
				setupHeaders();
				listing = (QStrList *)arch->getListing();
				lb->clear();
				lb->appendStrList( listing );
				gdest = new ExtractDlg( ExtractDlg::All );
			}else{
				sb->changeItem( (char *)klocale->translate( "Unknown archive format"), 0 );
				delete arch;
				arch = NULL;
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
	int pos = lb->currentItem();
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
	while( (tmp=strstr( hdrs, "\t" ))!=NULL )
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
