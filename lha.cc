#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "lha.h"
#include "text.h"

LhaArch::LhaArch()
  : Arch()
{
	listing = new QStrList;
	storefullpath = FALSE;
	onlyupdate = FALSE;
}

LhaArch::~LhaArch()
{
	delete listing;
}

unsigned char LhaArch::setOptions( bool p, bool l, bool o )
{
	perms = p;
	tolower = l;
	overwrite = o;
	return 0;
}

void LhaArch::onlyUpdate( bool in )
{
	onlyupdate = in;
}

void LhaArch::addPath( bool in )
{
	storefullpath = in;
}

void LhaArch::openArch( QString file )
{
	QString ex;
	char line[4096];
	FILE *fd;
	
	int idx, c;
	archname = file;
	ex = "lha -v " + file;
	fd = popen( ex, "r" );
	bool i=FALSE;
	char *nl;
	char *tmp;
	
	while( 1 )
	{
		fgets( line, 4096, fd );
		if( feof(fd) )
			break;
		if(i)
		{
			if( strstr( line, "----" ) )
				break;
			nl = strstr( line, "\n" );
			*nl = '\0';
			for(idx=0; idx<9; idx++ )
			{
				if( idx==5 || idx==7 )
				{
					idx == 5 ? c=1 : c=2;
					for( int ii=0; ii<c;ii++ )
					{
						tmp = strstr( line, " " );
						tmp[0] = '\a'; // :) Kludge Alert :)
						if( tmp[1]==' ' )
							tmp[1]='\a';
					}
				}
				else{
					tmp = strstr( line, " " );
					tmp[0]='\t';
				}
				while( tmp[1]==' ' )
					strshort( tmp+1, 1 );
			}
			while( (tmp = strstr( line, "\a" ))!=NULL )
				tmp[0] = ' ';
			listing->append( line );
		}
		if( strstr( line, "----" ) )
			i=TRUE;
	}
	pclose( fd );
}

void LhaArch::createArch( QString file )
{
	archname = file;
}

const QStrList *LhaArch::getListing()
{
	return listing;
}


int LhaArch::addFile( QStrList *urls )
{
	QString ex( "lha a " );
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
	system( ex );
	int pos = archname.findRev( ".lha" );
	if( pos != -1 )
		archname.replace( pos, 4, ".lzh" );  // My lha makes it end with lzh :(
	listing->clear();
	openArch( archname );
	return 0;
}

void LhaArch::extractTo( QString dest )
{
	QString ex;
	FILE *fd;
	char line[4096];
	ex = "lha xfw=" + dest + " " + archname;
	newProgressDialog( 1, listing->count() );
	fd = popen( ex, "r" );
	for( long int i=0; !feof(fd); i++ )
	{
		fgets( line, 4096, fd );
		if( Arch::isCanceled() )
			break;
		setProgress( i );
	}
}

QString LhaArch::unarchFile( int pos, QString dest )
{
	QString ex, tmp, tmp2;
	tmp = listing->at( pos );
	tmp2 = tmp.right( (tmp.length())-(tmp.findRev('\t')+1) );
	ex = "lha xfw=" + dest + " " + archname + " " + tmp2;
	system( ex );
	return (dest+tmp2);
}

void LhaArch::deleteFile( int pos )
{
	QString ex, name, tmp;
	tmp = listing->at( pos );
	name = tmp.right( (tmp.length())-(tmp.findRev('\t')+1) );
	ex = "lha df " + archname + " " + name;
	system( ex );
	listing->clear();
	openArch( archname );
}

