/* (c)1997 Robert Palmbos
   See main.cc for license details */
#include <stdio.h>
#include <stdlib.h>
#include <kprogress.h>
#include <qdialog.h>
#include <qpushbt.h>
#include <qobject.h>
#include <kfm.h>
#include "karch.h"

KZipArch::KZipArch()
{
	arch = 0;
}

KZipArch::~KZipArch()
{
	delete arch;
}

int KZipArch::getArchType( QString archname )
{
	if( archname.contains(".tgz", FALSE) || archname.contains(".tar.gz", FALSE) )
		return Tar;
	if( archname.contains(".lha", FALSE) || archname.contains(".lzh", FALSE ))
		return Lha;
	if( archname.contains(".zip", FALSE) )
		return Zip;
	if( archname.contains(".a", FALSE ) )
		return AA;
	return -1;
}

unsigned char KZipArch::setOptions( bool p, bool l, bool o )
{
	return arch->setOptions( p, l, o );
}

void KZipArch::onlyUpdate( bool up )
{
	arch->onlyUpdate( up );
}

void KZipArch::addPath( bool p )
{
	arch->onlyUpdate( p );
}

bool KZipArch::openArch( QString name )
{
	switch( getArchType( name ) )
	{
		case Tar:
		{
			arch = new TarArch( "tar" );
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

bool KZipArch::createArch( QString file )
{
	switch( getArchType( file ) )
	{
		case Tar:
		{
			arch = new TarArch( "tar" );
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

const QStrList *KZipArch::getListing()
{
	return arch->getListing();
}

int KZipArch::addFile( QStrList *urls )
{
	return arch->addFile( urls );
}

void KZipArch::extractTo( QString dir )
{
	arch->extractTo( dir );
}

QString KZipArch::unarchFile( int index, QString dest )
{
	return arch->unarchFile( index, dest );
}

void KZipArch::deleteFile( int indx )
{
	arch->deleteFile( indx );
}

