/* (c)1997 Robert Palmbos
   See main.cc for license details */
#include <stdio.h>
#include <stdlib.h>
#include <kprogress.h>
#include <qdialog.h>
#include <qpushbt.h>
#include <qobject.h>
#include <kfm.h>
#include "karchive.h"

KArchive::KArchive( QString te )
{
	tar_exe = te;  // *sigh*
	arch = 0;
}

KArchive::~KArchive()
{
	delete arch;
}

int KArchive::getArchType( QString archname )
{
	if( archname.contains(".tgz", FALSE) || archname.contains(".tar.gz", FALSE) 
			|| archname.contains( ".tar.Z", FALSE ) || archname.contains(".tar.bz", FALSE)
			|| archname.contains( ".tar.bz2", FALSE ) || archname.contains(".tar.lzo", FALSE)
			|| archname.contains( ".tbz", FALSE ) || archname.contains(".tzo", FALSE)
			|| archname.contains( ".taz", FALSE) )
		return Tar;
	if( archname.contains(".lha", FALSE) || archname.contains(".lzh", FALSE ))
		return Lha;
	if( archname.contains(".zip", FALSE) )
		return Zip;
	if( archname.contains(".a", FALSE ) )
		return AA;
	return -1;
}

unsigned char KArchive::setOptions( bool p, bool l, bool o )
{
	return arch->setOptions( p, l, o );
}

void KArchive::onlyUpdate( bool up )
{
	arch->onlyUpdate( up );
}

void KArchive::addPath( bool p )
{
	arch->onlyUpdate( p );
}

bool KArchive::openArch( QString name )
{
	switch( getArchType( name ) )
	{
		case Tar:
		{
			arch = new TarArch( tar_exe );
			arch->openArch( name );
			break;
		}
		case Zip:
		{
			arch = new ZipArch;
			arch->openArch( name );
			break;
		}
		case Lha:
		{
			arch = new LhaArch;
			arch->openArch( name );
			break;
		}
		case AA:
		{
			arch = new ArArch;
			arch->openArch( name );
			break;
		}
		case -1:
		{
			return FALSE;
		}
	}
	return TRUE;
}

bool KArchive::createArch( QString file )
{
	switch( getArchType( file ) )
	{
		case Tar:
		{
			arch = new TarArch( tar_exe );
			arch->createArch( file );
			break;
		}
		case Lha:
		{
			arch = new LhaArch;
			arch->createArch( file );
			break;
		}
		case Zip:
		{
			arch = new ZipArch;
			arch->createArch( file );
			break;
		}
		case AA:
		{
			arch = new ArArch;
			arch->createArch( file );
			break;
		}
		case -1:
		{
			return FALSE;
		}
	}
	return TRUE;
}

const QStrList *KArchive::getListing()
{
	return arch->getListing();
}

int KArchive::addFile( QStrList *urls )
{
	return arch->addFile( urls );
}

void KArchive::extractTo( QString dir )
{
	arch->extractTo( dir );
}

QString KArchive::unarchFile( int index, QString dest )
{
	return arch->unarchFile( index, dest );
}

void KArchive::deleteFile( int indx )
{
	arch->deleteFile( indx );
}

