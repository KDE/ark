/* (c)1997 Robert Palmbos
   See main.cc for license details */
#include <kurl.h>
// Unsorted in qdir.h is used, but in some of the headers
// below it's defined, too. So I brought kurl.h to the top.
#include <iostream.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "tar.h"
#include <qstrlist.h>

TarArch::TarArch( QString te )
	: Arch()
{
	cout << "Entered TarArch" << endl;
	listing = new QStrList;
	compressed = TRUE;
	onlyupdate = FALSE;
	storefullpath = FALSE;
	tmpfile.sprintf( "/tmp/kzip.%d/tmpfile.tar", getpid() );
	tar_exe = te;
	cout << "Left TarArch" << endl;
}

TarArch::~TarArch()
{
	cout << "Entered ~TarArch" << endl;
	updateArch();
	delete listing;
	cout << "Left ~TarArch" << endl;
}

int TarArch::updateArch()
{
	cout << "Entered updateArch" << endl;
	int retcode = 0;
	if( !compressed )
	{
		FILE* fd, *fd2;
		char buffer[4096];
		int size = 0;
		compressed = TRUE;
		archProcess.clearArguments();
//		archProcess.setExecutable( "gzip" );
		archProcess << getCompressor() << "-c" << tmpfile;
		if(archProcess.startPipe(KProcess::Stdout, &fd) == FALSE)
		{
			cerr << "Subprocess wouldn't start!" << endl;
			return -1;
		}

		fd2 = fopen(archname, "w");
		
		while((size = fread(buffer, 1, 4096, fd)))
			fwrite(buffer, size, 1, fd2);
		fclose(fd2);

		while(archProcess.isRunning())
			;

		if( !retcode )
		{
			listing->clear();
			openArch( archname );
		}
	}
	cout << "Left updateArch" << endl;
	return retcode;
}

QString TarArch::getCompressor() 
{
	QString extension = archname.right( archname.length()-archname.findRev('.') );
	cout << extension;
	if( extension == ".tgz" || extension == ".gz" ) 
		return QString( ".gzip" );
	if( extension == ".bz" )
		return QString( "bzip" );
	if( extension == ".Z" || extension == ".taz" )
		return QString( "compress" );
	if( extension == ".bz2" )
		return QString( "bzip2" );
	return 0;
}

QString TarArch::getUnCompressor() 
{
	QString extension = archname.right( archname.length()-archname.findRev('.') );
	cout << extension;
	if( extension == ".tgz" || extension == ".gz" ) 
		return QString( "gunzip" );
	if( extension == ".bz" )
		return QString( "bunzip" );
	if( extension == ".Z" || extension == ".taz" )
		return QString( "uncompress" );
	if( extension == ".bz2" )
		return QString( "bunzip2" );
	return 0;
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
	cout << "Entered openArch" << endl;
	char line[4096];
	char columns[5][80];
	char filename[4096];
	QString buffer;
	FILE *fd;

	archProcess.clearArguments();
//	archProcess.setExecutable( tar_exe );

	archname = name;

	archProcess << tar_exe << "--use-compress-program="+getUnCompressor() 
	            <<	"-tvf" << archname;
	
 	if(archProcess.startPipe(KProcess::Stdout, &fd) == FALSE)
 	{
 		cerr << "Subprocess wouldn't start!" << endl;
 		return;
 	}

	fgets( line, 4096, fd );

	while( !feof(fd) )
	{
		sscanf(line, " %[-drwxst] %[0-9.a-zA-Z/] %[0-9] %17[a-zA-Z0-9:- ] %[^\n]",
			columns[0], columns[1], columns[2], columns[3],
			 filename
			);
		sprintf(line, "%s\t%s\t%s\t%s\t%s",
			columns[0], columns[1], columns[2], columns[3],
			filename);
		listing->append( line );
		fgets( line, 4096, fd );
	}
//	fclose( fd );
//	There should be a file descriptor close call, but this one makes a 
//	BAD FILEDESCRIPTOR error message
//	Another note: gzip reported 'Broken pipe' because of that fclose()
	while(archProcess.isRunning())
		;


	cout << "Left openArch" << endl;
}

void TarArch::createArch( QString file )
{
	cout << "Entered createArch" << endl;
	/*QString ex;
	ex = "tar cf ";
	ex += file;
	system( ex );*/
	archname = file;
	cout << "Left createArch" << endl;
}

const QStrList * TarArch::getListing()
{
	return listing;
}

void TarArch::createTmp()
{
	cout << "Entered createTmp" << endl;
	if( compressed )
	{
		FILE* fd, *fd2;
		char buffer[4096];
		int size = 0;
		compressed = FALSE;
		archProcess.clearArguments();
//		archProcess.setExecutable( "gunzip" );
		archProcess << "gunzip" << "-c" << archname;
		if(archProcess.startPipe(KProcess::Stdout, &fd) == FALSE)
		{
			cerr << "Subprocess wouldn't start!" << endl;
			return;
		}

		fd2 = fopen(tmpfile, "w");
		
		while((size = fread(buffer, 1, 4096, fd)))
		{
			fwrite(buffer, size, 1, fd2);
		}
		fclose(fd2);
		fclose(fd);
		while(archProcess.isRunning())
			;
	}
	cout << "Left createTmp" << endl;
}

int TarArch::addFile( QStrList* urls )
{
	cout << "Entered addFile" << endl;

	int retcode;
	QString file, url, tmp;
	
	createTmp();

	url = urls->first();
	cout << "Url's name is: " << url << endl;
	KURL::decodeURL(url); // Because of special characters
	cout << "Url's name now is: " << url << endl;
	file = url.right( url.length()-5 );
	if( file[file.length()-1]=='/' )
		file[file.length()-1]='\0';

// 	if(archProcess.isRunning())
// 		cerr << "Why oh, why you don't know how to say good bye?" << endl;
	archProcess.clearArguments();	
//	archProcess.setExecutable( tar_exe );

	archProcess << tar_exe;

//  	QStrList segflist1 = archProcess.getArguments();
//  	for (const char* f = segflist1.first(); f; f = segflist1.next())
//  		cout << "Argument:" << "'" << f << "'" << endl;

	if( onlyupdate )
		archProcess << "uvf";
	else
		archProcess << "rvf";
	archProcess << tmpfile;
	QString base;

	if( !storefullpath )
	{
		int pos;
		pos = file.findRev( '/', -1, FALSE );
		base = file.left( ++pos );
		cout << "base is" << base << endl;
//		pos++;
		tmp = file.right( file.length()-pos );
		file = tmp;
		chdir( base );
	}
	while(1)
	{
		int pos;
		archProcess << file;
		url = urls->next();
//		cout << url << " is the name of the url " << endl;
		if( url.isNull() )
			break;
		KURL::decodeURL(url); 
		// It's used because of the special characters, can't be
		// before url.isNull() because this function makes empty string from
		// a NULL pointer
//		cout << url << " is the name of the url " << endl;
		file = url.right( url.length()-5 );
//		cout << file << " is the name of the file " << endl;
		if( file[file.length()-1]=='/' )
			file[file.length()-1]='\0';
		pos = file.findRev( '/', -1, FALSE );
		pos++;
		tmp = file.right( file.length()-pos );
		file = tmp;
	}	
	FILE *fd;
	char inp[4096];
// Debuggin part
 	QStrList segflist	= archProcess.getArguments();
 	for (const char* f = segflist.first(); f; f = segflist.next())
 		cout << "Argument:" << "'" << f << "'" << endl;
 	if(archProcess.startPipe(KProcess::Stdout, &fd) == FALSE)
 	{
 		cerr << "Subprocess wouldn't start!" << endl;
 		cerr << "In addFile" << endl;
 		return -1;
 	}
	newProgressDialog( 1, urls->count() );
	for( long int i=1; !feof( fd ); i++ )
	{
		kapp->processEvents();
		if( Arch::isCanceled() )
		{
			archProcess.kill();
			break;
		}
		fgets( inp, 4096, fd );
		cerr << inp;
		setProgress( i );
 	}
	fclose( fd );
	while(archProcess.isRunning())
		;

	if( storefullpath )
		file.remove( 0, 1 );  // Get rid of leading /
	retcode = updateArch();
	return retcode;
	cout << "Left addFile" << endl;
}

void TarArch::extractTo( QString dir )
{
	cout << "Entered extractTo" << endl;
	FILE *fd;
	char line[4096];

	archProcess.clearArguments();
//	archProcess.setExecutable( tar_exe );
	archProcess << tar_exe;



	archProcess << archname << "-C" << dir;	
 	if(archProcess.startPipe(KProcess::Stdout, &fd) == false)
 	{
 		cerr << "Subprocess wouldn't start!" << endl;
 		return;
 	}
	newProgressDialog( 1, listing->count() );
	for( long int i=1; !feof( fd ); i++ )
	{
		kapp->processEvents();
		if( Arch::isCanceled() )
		{
			archProcess.kill();
			break;
		}
		fgets( line, 4096, fd );
		setProgress( i );
 	}
	fclose( fd );
	while(archProcess.isRunning())
		;


	cout << "Left extractTo" << endl;
}

QString TarArch::unarchFile( int index, QString dest )
{
	cout << "Entered unarchFile" << endl;
	int pos;
	QString tmp, name;
	QString fullname;
	
	updateArch();
	
	tmp = listing->at( index );
	pos = tmp.findRev( '\t', -1, FALSE );
	pos++;
	name = tmp.right( tmp.length()-pos );

	archProcess.clearArguments();
//	archProcess.setExecutable( tar_exe );
	archProcess << tar_exe;

	if( perms )
		archProcess << "--use-compress-program="+getUnCompressor() << "-xvpf";
	else
		archProcess << "--use-compress-program="+getUnCompressor() << "-xvf";

	archProcess << archname << "-C" << dest << name;
	archProcess.start(KProcess::Block);
	fullname = dest + name;
	cout << "Left unarchFile" << endl;
	return fullname;
}

void TarArch::deleteFile( int indx )
{
	cout << "Entered deleteFile" << endl;
	QString name, tmp;
	
	createTmp();
	
	tmp = listing->at(indx);
	name = tmp.right( tmp.length() - (tmp.findRev('\t')+1) );
	archProcess.clearArguments();
//	archProcess.setExecutable( tar_exe );
	archProcess << tar_exe << "--delete" << "-f" << tmpfile << name;
	archProcess.start(KProcess::Block);
	
	updateArch();
	cout << "Left deleteFile" << endl;
}

