/* (c)1997 Robert Palmbos
   See main.cc for license details */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "tar.h"
#include "text.h"

TarArch::TarArch()
	: Arch()
{
	listing = new QStrList;
	compressed = TRUE;
	onlyupdate = FALSE;
	storefullpath = FALSE;
	tmpfile.sprintf( "/tmp/kzip.%d/tmpfile.tar ", getpid() );
}

TarArch::~TarArch()
{
	updateArch();
	delete listing;
}

int TarArch::updateArch()
{
	int retcode = 0;
	if( !compressed )
	{
		compressed = TRUE;
		QString ex;
		ex = "gzip -c ";
		ex += tmpfile;
		ex += " > ";
		ex += archname;
		retcode = system( ex );

		if( !retcode )
		{
			listing->clear();
			openArch( archname );
		}
	}
	return retcode;
}

unsigned char TarArch::setOptions( bool p, bool l, bool o )
{
	perms = p;
	tolower = l;
	overwrite = o;
	return 5;
}

void TarArch::onlyUpdate( bool up )
{
	onlyupdate = up;
}

void TarArch::addPath( bool p )
{
	storefullpath = p;
}

void TarArch::openArch( QString name )
{
	FILE *fd;
	char line[4096];
	char *tmp;

	archname = name;
	QString ex;
	ex = "tar ztvf "+name;
	fd = popen( ex, "r" );
	while(1)
	{
		fgets(line, 4096, fd );
		if( feof( fd ) )
			break;
		line[strlen(line)-1] = '\0';
		for( int i=0; i<5; i++ )
		{
			if( i==3 )
			{
				for( int ii=0; ii<1;ii++ )
				{
					tmp = strstr( line, " " );
					tmp[0] = '\a'; // :) Kludge Alert :)
				}
			}
			else{
				tmp = strstr( line, " " );
				tmp[0]='\t';
			}
			while( tmp[1]==' ' )
				strshort( tmp+1, 1 );
		}
		while( (tmp = strstr( line, "\a" ))!=0 )
			tmp[0] = ' ';
		listing->append( line );
	}
	pclose( fd );
}

void TarArch::createArch( QString file )
{
	/*QString ex;
	ex = "tar cf ";
	ex += file;
	system( ex );*/
	archname = file;
}

const QStrList * TarArch::getListing()
{
	return listing;
}

void TarArch::createTmp()
{
	if( compressed )
	{
		compressed = FALSE;
		QString ex( "gunzip -c " );
		ex += archname;
		ex += " > " + tmpfile;
		system( ex );
	}
}

int TarArch::addFile( QStrList* urls )
{
	int retcode;
	QString ex, file, url, tmp;

	
	createTmp();

	url = urls->first();
	file = url.right( url.length()-5 );
	if( file[file.length()-1]=='/' )
		file[file.length()-1]='\0';
	ex = "tar ";
	if( onlyupdate )
		ex += "uvf ";
	else
		ex += "rvf ";
	ex += tmpfile;
	QString base;
	if( !storefullpath )
	{
		int pos;
		pos = file.findRev( '/', -1, FALSE );
		base = file.left( pos );
		pos++;
		tmp = file.right( file.length()-pos );
		file = tmp;
		chdir( base );
	}
	while(1)
	{
		int pos;
		ex = ex+"\""+file+"\" ";
		url = urls->next();
		if( url.isNull() )
			break;
		file = url.right( url.length()-5 );
		if( file[file.length()-1]=='/' )
			file[file.length()-1]='\0';
		pos = file.findRev( '/', -1, FALSE );
		pos++;
		tmp = file.right( file.length()-pos );
		file = tmp;
	}	
	FILE *fd;
	char inp[4096];
	fd = popen( ex, "r" );
	newProgressDialog( 1, urls->count() );
	for( long int i=1; !feof( fd ); i++ )
	{
		if( Arch::isCanceled() )
			break;
		fgets( inp, 4096, fd );
		setProgress( i );
 	}
	pclose( fd );
	if( storefullpath )
		file.remove( 0, 1 );  // Get rid of leading /
	retcode = updateArch();
	return retcode;
}

void TarArch::extractTo( QString dir )
{
	QString ex;
	FILE *fd;
	
	if( perms )
		ex = "tar zxvpf ";
	else
		ex = "tar zxvf ";

	ex += archname;
	ex += " -C ";
	ex += dir;
	fd = popen( ex, "r" );
	char tmp[4096];
	Arch::newProgressDialog( 1, listing->count() );
	for( long int i=1; !feof( fd ); i++ )
	{
		if( Arch::isCanceled() )
			break;
		fgets( tmp, 4096, fd );
		setProgress( i );
 	}
	pclose( fd );
}

QString TarArch::unarchFile( int index, QString dest )
{
	int pos;
	QString ex, tmp, name;
	QString fullname;
	
	updateArch();
	
	tmp = listing->at( index );
	pos = tmp.findRev( '\t', -1, FALSE );
	pos++;
	name = tmp.right( tmp.length()-pos );

	if( perms )
		ex = "tar zxpf ";
	else
		ex = "tar zxf ";
	ex += archname;
	ex += " -C ";
	ex += dest;
	ex = ex + " " + name;
	system( ex );
	fullname = dest + name;
	return fullname;
}

void TarArch::deleteFile( int indx )
{
	QString ex, name, tmp;
	
	createTmp();
	
	tmp = listing->at(indx);
	name = tmp.right( tmp.length() - (tmp.findRev('\t')+1) );
	ex = "tar --delete -f ";
	ex += tmpfile;
	ex += " ";
	ex += name;
	system( ex );
	
	updateArch();
}

