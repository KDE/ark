/* (c)1997 Robert Palmbos
See main.cc for license details */
#include <iostream.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "ar.h"
#include "text.h"
#include "errors.h"

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
	
	archProcess.clearArguments();
	archProcess.setExecutable( "ar" );
	archname = file;
	archProcess << "vt" << file;
	if( archProcess.startPipe(KProcess::Stdout, &fd) == FALSE )
	{
		cerr << "Subproccess won't start, armageddon is iminent!";
		return;
	}
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
		while( (tmp = strstr( line, "\a" ))!=0 )
			tmp[0] = ' ';
		listing->append( line );
	}
	// pclose( fd );  I'm not sure what should be here
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
	QString base;
	QString url;
	QString file;

	archProcess.clearArguments();
	archProcess.setExecutable( "ar" );
	archProcess << "q";	
	if( onlyupdate )
		archProcess << "u";
	archProcess << archname;
	
	url = urls->first();
	do
	{
		file = url.right( url.length()-5);
		if( file[file.length()-1]=='/' ) {
			return UNSUPDIR;
		}
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
		archProcess << file;
		url = urls->next();
	}while( !url.isNull() );
	cout << "starting add command";
	archProcess.start( KProcess::Block );
	listing->clear();
	openArch( archname );
	return 0;
}

void ArArch::extractTo( QString dest )
{
	FILE *fd;
	char line[4096];

	archProcess.clearArguments();
	archProcess.setExecutable( "cp" );
	archProcess << archname << dest; // Ar doesn't extract to a dir, so fake it
	archProcess.start( KProcess::Block );	

	char pwd[4096];
	getcwd( pwd, 4096 );
	chdir( dest );
	archProcess.clearArguments();
	archProcess.setExecutable( "ar" );
	archProcess << "vx" << archname;

	if( archProcess.startPipe( KProcess::Stdout, &fd ) == FALSE )
	{
		cerr << "Subprocess won't start, perhaps your computer has a virus?";
		return;
	}
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

	archProcess.clearArguments();
	archProcess.setExecutable( "cp" );
	archProcess << archname << dest; // Ar doesn't extract to a dir, so fake it
	archProcess.start( KProcess::Block );		

	char pwd[4096]; 
	getcwd( pwd, 4096 );

	chdir( dest );

	archProcess.clearArguments();
	archProcess.setExecutable( "ar" );
	archProcess << "x" << archname << tmp2;

	archProcess.start( KProcess::Block );

	QString curarch = archname.right( archname.length()-(archname.findRev( '/' )+1) );
	int i = unlink( curarch );
	if( i==-1 )
		perror( "kzip" );
	chdir( pwd );
	return (dest+tmp2);
}

void ArArch::deleteFile( int pos )
{
	QString name, tmp;
	tmp = listing->at( pos );
	name = tmp.right( (tmp.length())-(tmp.findRev('\t')+1) );
	archProcess.clearArguments();
	archProcess.setExecutable( "ar" );
	archProcess << "d" << archname << name ;
	archProcess.start( KProcess::Block );
	listing->clear();
	openArch( archname );
}

