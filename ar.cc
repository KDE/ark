#include <iostream.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "ar.h"
#include "text.h"

ArArch::ArArch()
  : Arch()
{
	listing = new QStrList;
	storefullpath = FALSE;
	onlyupdate = FALSE;
}

ArArch::~ArArch()
{
	delete listing;
}

unsigned char ArArch::setOptions( bool p, bool l, bool o )
{
	perms = p;
	tolower = l;
	overwrite = o;
	return 0;
}

void ArArch::onlyUpdate( bool in )
{
	onlyupdate = in;
}

void ArArch::addPath( bool in )
{
	storefullpath = in;
}

void ArArch::openArch( QString file )
{
	QString ex;
	char line[4096];
	FILE *fd;
	char *tmp;
	
	archname = file;
	ex = "ar vt " + file;
	fd = popen( ex, "r" );
	char *nl;
	while( 1 )
	{
		fgets( line, 4096, fd );
		if( feof(fd) )
			break;
		nl = strstr( line, "\n" );
		*nl = '\0';
		for( int i=0; i<5; i++ )
		{
			if( i==3 )
			{
				for( int ii=0; ii<3;ii++ )
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
	pclose( fd );
}

void ArArch::createArch( QString file )
{
	archname = file;
}

const QStrList *ArArch::getListing()
{
	return listing;
}


int ArArch::addFile( QStrList *urls )
{
	QString ex( "ar q " );
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
	listing->clear();
	openArch( archname );
	return 0;
}

void ArArch::extractTo( QString dest )
{
	FILE *fd;
	QString ex;
	char line[4096];
	ex = "cp " + archname + " " + dest; // Ar doesn't extract to a dir, so fake it
	system( ex );
	char pwd[4096];  // Potential bug, but unlikely
	getcwd( pwd, 4096 );
	chdir( dest );
	ex = "ar vx ";
	ex = ex + archname;
	fd = popen( ex, "r" );
	newProgressDialog( 1, listing->count() );
	for( long int i=0; !feof(fd); i++ )
	{
		fgets( line, 4096, fd );
		if( Arch::isCanceled() )
			break;
		setProgress( i );
	}
	QString curarch = archname.right( archname.length()-(archname.findRev( '/' )+1) );
	unlink( curarch );
	chdir( pwd );
}

QString ArArch::unarchFile( int pos, QString dest )
{
	QString ex, tmp, tmp2;
	tmp = listing->at( pos );
	tmp2 = tmp.right( (tmp.length())-(tmp.findRev('\t')+1) );

	ex = "cp " + archname + " " + dest; // Ar doesn't extract to a dir, so fake it
	system( ex );

	char pwd[4096];  // Potential bug, but unlikely
	getcwd( pwd, 4096 );

	chdir( dest );

	ex = "ar x ";
	ex = ex + archname + " " + tmp2;

	system( ex );

	cout << archname;
	QString curarch = archname.right( archname.length()-(archname.findRev( '/' )+1) );
	cout << curarch;
	int i = unlink( curarch );
	if( i==-1 )
		perror( "kzip" );
	chdir( pwd );
	return (dest+tmp2);
}

void ArArch::deleteFile( int pos )
{
	QString ex, name, tmp;
	tmp = listing->at( pos );
	name = tmp.right( (tmp.length())-(tmp.findRev('\t')+1) );
	ex = "ar d " + archname + " " + name ;
	system( ex );
	listing->clear();
	openArch( archname );
}

