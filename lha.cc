/* (c)1997 Robert Palmbos
   See main.cc for license details */
#include <kurl.h>
#include <iostream.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "lha.h"
// Just to find out the segfault problem
#include <qstrlist.h>
//#include <string.h>
#include <sys/errno.h>

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
	cout << "Entered openArch" << endl;
	char line[4096];
	char columns[8][80];
	char filename[4096];
	QString buffer;
	FILE *fd;

	archProcess.clearArguments();
	archProcess.setExecutable( "lha" );

	archname = file;

	archProcess << "-v" << archname;
 	if(archProcess.startPipe(KProcess::Stdout, &fd) == FALSE)
 	{
 		cerr << "Subprocess wouldn't start!" << endl;
 		return;
 	}

	fgets( line, 4096, fd );
	if( feof(fd) )
	{
		fclose( fd );
		return;
	}

	while( !feof(fd) && !strstr( line, "----------" ) )
		fgets( line, 4096, fd );
	fgets( line, 4096, fd );

	while( !feof(fd) && !strstr( line, "----------" ) )
	{
//		cerr << "Actual line's: " << line << endl;
// In the scanf's format string, the first and the fifth conversion string
// contains a 'd' and an *, respectively so that scanf can parse directories,
// too.
		if( QString::QString(line).contains("[generic]") ) {
			sscanf( line, " %[]\[generic] %[0-9] %[0-9] %[0-9.%*] %10[-a-z0-9 ] "
				"%12[A-Za-z0-9: ]%1[ ]%[^\n]", 
				columns[0], columns[2], columns[3], columns[4], columns[5], 
				columns[6], columns[7], filename );
			strcpy( columns[1], " " );
		} else {
			sscanf(line, " %[-drwxst] %[0-9/] %[0-9] %[0-9] %[0-9.%*] %10[-a-z0-9 ] "
			"%12[A-Za-z0-9: ]%1[ ]%[^\n]",
				columns[0], columns[1], columns[2], columns[3],
				columns[4], columns[5], columns[6], columns[7],
				filename
				);
		}
		cerr << "The actual file's : '" << filename << "'" << endl;
// Hereby I skip the line if the first field contains 'd', it means directory.
//		if(!QString::QString(columns[0]).contains('d'))
//		{
			sprintf(line, "%s\t%s\t%s\t%s\t%s\t%s\t"
				"%s\t%s",
				columns[0],columns[1],columns[2],columns[3],
				columns[4],columns[5],columns[6],filename);
			listing->append( line );
//		}
		fgets( line, 4096, fd );
	}
//	fclose( fd );
//	cerr << strerror(errno);
//	There should be a file descriptor close call, but this one makes a 
//	BAD FILEDESCRIPTOR error message
//	BIG question: if the child (the subprocess) closed the socket on its side,
//	the other side including the FILE * structure opened by fdopen() would
//	close, too.
	

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
	archProcess.clearArguments();
	archProcess.setExecutable( "lha" );
	archProcess << "a";
	QString base;
	QString url;
	QString file;
	
	if( onlyupdate )
		archProcess << "-u";
	archProcess << archname;
	
	url = urls->first();
	do
	{
		KURL::decodeURL(url); // Because of special characters
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
		archProcess << file;
		url = urls->next();
	}while( !url.isNull() );
	archProcess.start(KProcess::Block);
	int pos = archname.findRev( ".lha" );
	if( pos != -1 )
		archname.replace( pos, 4, ".lzh" );  // My lha makes it end with lzh :(
	listing->clear();
	openArch( archname );
	return 0;
//	cout << "left addFile" << endl;
}

void LhaArch::extractTo( QString dest )
{
	FILE *fd;
	char line[4096];
	archProcess.clearArguments();
	archProcess.setExecutable( "lha" );
	archProcess << "xfw=" << dest << archname;

  	newProgressDialog( 1, listing->count() );
 	if(archProcess.startPipe(KProcess::Stdout, &fd) == false)
 	{
 		cerr << "Subprocess wouldn't start!" << endl;
 		return;
 	}
	for( long int i=0; !feof(fd); i++)
	{
		kapp->processEvents();
		fgets( line, 4096, fd );  
		if( Arch::isCanceled() )
		{
			archProcess.kill();
			break;
		}
		setProgress( i );
	}
}

QString LhaArch::unarchFile( int pos, QString dest )
{
//  	cout << "entered unarchFile" << endl;
	QString tmp, tmp2;

// Segfault testing
//	QStrList segflist	= archProcess.getArguments();
//	for (const char* f = segflist.first(); f; f = segflist.next())
//		cout << "Argument:" << f << endl;

	archProcess.clearArguments();
 	archProcess.setExecutable("lha");
	tmp = listing->at( pos );
	tmp2 = tmp.right( (tmp.length())-(tmp.findRev('\t')+1) );
	archProcess << "xfw=" + dest << archname << tmp2;
 	archProcess.start(KProcess::Block);
	return (dest+tmp2);
//  	cout << "left unarchFile" << endl;
}

void LhaArch::deleteFile( int pos )
{
	cout << "Entered deleteFile" << endl;
	QString name, tmp;

	archProcess.clearArguments();
 	archProcess.setExecutable("lha");
	tmp = listing->at( pos );
	name = tmp.right( (tmp.length())-(tmp.findRev('\t')+1) );
 	archProcess << "df" << archname << name;
 	archProcess.start(KProcess::Block);
	listing->clear();
	openArch( archname );
	cout << "Left deleteFile" << endl;
}

