/* (c)1997 Robert Palmbos
   See main.cc for license details */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "zip.h"
#include "text.h"

ZipArch::ZipArch()
  : Arch()
{
	listing = new QStrList;
	storefullpath = FALSE;
	onlyupdate = FALSE;
}

ZipArch::~ZipArch()
{
	delete listing;
}

unsigned char ZipArch::setOptions( bool p, bool l, bool o )
{
	perms = p;
	tolower = l;
	overwrite = o;
	return 2;
}

void ZipArch::onlyUpdate( bool in )
{
	onlyupdate = in;
}

void ZipArch::addPath( bool in )
{
	storefullpath = in;
}

void ZipArch::openArch( QString file )
{
	QString ex;
	char line[4096];
	FILE *fd;
	int idx;
	char *tmp;
	
	archname = file;
	ex = "unzip -v " + file;
	fd = popen( ex, "r" );
	bool i=FALSE;
	char *nl;
	while( 1 )
	{
		fgets( line, 4096, fd );
		if( feof(fd) )
			break;
		if(i)
		{
			if( strstr( line, "----" ) )
				break;
			while( line[0] == ' ' )
				strshort( line, 1 );
			nl = strstr( line, "\n" );
			*nl = '\0';
			for(idx=0; idx<7; idx++ )
			{
				tmp = strstr( line, " " );
					tmp[0]='\t';
				while( tmp[1]==' ' )
					strshort( tmp+1, 1 );
			}
			listing->append( line );
		}
		if( strstr( line, "----" ) )
			i=TRUE;
	}
	pclose( fd );
}

void ZipArch::createArch( QString file )
{
	archname = file;
}

const QStrList *ZipArch::getListing()
{
	return listing;
}


int ZipArch::addFile( QStrList *urls )
{
	QString ex( "zip -r " );
	QString base;
	QString url;
	QString file;
	
	if( onlyupdate )
		ex += "-u ";
	ex = ex+archname+" ";
	
	url = urls->first();
	do
	{
		file = url.right( url.length()-5);
		if( file[file.length()-1]=='/' )
			file[file.length()-1]='\0';
		if( !storefullpath )
		{
			int pos;
			pos = file.findRev( '/' );
			base = file.left( pos );
			pos++;
			chdir( base );
			base = file.right( file.length()-pos );
			file = base;
		}
		ex = ex +" "+file;
		url = urls->next();
	}while( !url.isNull() );
	printf( "ex: %s\n", (const char *) ex );
	system( ex );
	listing->clear();
	openArch( archname );
	return 0;
}

void ZipArch::extractTo( QString dest )
{
	FILE *fd;
	QString ex;
	char line[4096];
	ex = "unzip -o ";
	if( tolower )
		ex += " -L ";
	ex = ex + archname + " -d " + dest;
	fd = popen( ex, "r" );
	newProgressDialog( 1, listing->count() );
	for( long int i=0; !feof(fd); i++ )
	{
		fgets( line, 4096, fd );
		if( Arch::isCanceled() )
			break;
		setProgress( i );
	}
}

QString ZipArch::unarchFile( int pos, QString dest )
{
	QString ex, tmp, tmp2;
	tmp = listing->at( pos );
	tmp2 = tmp.right( (tmp.length())-(tmp.findRev('\t')+1) );
	ex = "unzip -o "+archname+" "+tmp2+" -d "+dest;
	system( ex );
	return (dest+tmp2);
}

void ZipArch::deleteFile( int pos )
{
	QString ex, name, tmp;
	tmp = listing->at( pos );
	name = tmp.right( (tmp.length())-(tmp.findRev('\t')+1) );
	ex = "zip -d " + archname + " " + name ;
	system( ex );
	listing->clear();
	openArch( archname );
}

