/* (c)1997 Robert Palmbos
   See main.cc for license details */
#include <iostream.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <kurl.h>
#include "zip.h"

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


void ZipArch::openArch( QString file, FileListView *flw )
{
	cout << "Entered openArch2" << endl;
	char line[4096];
	char columns[8][80];
	char filename[4096];
	QString buffer;
	FILE *fd;

	archProcess.clearArguments();

	archname = file;

	archProcess << "unzip" << "-v" << archname;
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

	//Clear the archive currently displayed
	flw->clear();
	flw->addColumn( i18n("Name") );
	flw->addColumn( i18n("Length") );
	flw->addColumn( i18n("Method") );
	flw->addColumn( i18n("Size") );
	flw->addColumn( i18n("Ratio") );
	flw->addColumn( i18n("Date") );
	flw->addColumn( i18n("Time") );
	flw->addColumn( i18n("CRC-32") );


	while( !feof(fd) && !strstr( line, "----" ) )
		fgets( line, 4096, fd );
	fgets( line, 4096, fd );

	while( !feof(fd) && !strstr( line, "----" ) )
	{
		sscanf(line, " %[0-9] %[a-zA-Z:] %[0-9] %[0-9%] %[-0-9] %[0-9:] "
			"%[0-9a-z]%3[ ]%[^\n]",
			columns[0], columns[1], columns[2], columns[3],
			columns[4], columns[5], columns[6], columns[7],
			filename
			);

		FileLVI *flvi = new FileLVI(flw);
		flvi->setText(0, filename);
		for(int i=0; i<7; i++)
		{
			flvi->setText(i+1, columns[i]);
		}
		flw->insertItem(flvi);

		sprintf(line, "%s\t%s\t%s\t%s\t%s\t%s\t"
			"%s\t%s",
			columns[0],columns[1],columns[2],columns[3],
			columns[4],columns[5],columns[6],filename);
		listing->append( line );
		cerr << line << "\n";	          	
		fgets( line, 4096, fd );
	}
//	fclose( fd );
//	There should be a file descriptor close call, but this one makes a
//	BAD FILEDESCRIPTOR error message

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
//  	cout << "entered in addFile" << endl;

	archProcess.clearArguments();
//	archProcess.setExecutable( "zip" );
	archProcess << "zip" << "-r";
	QString base;
	QString url;
	QString file;
	
	if( onlyupdate )
		archProcess << "-u";
	archProcess << archname;
	
	url = urls->first();
	do
	{
		file = KURL(url).path(-1); // remove trailing slash
		if( !storefullpath )
		{
			int pos;
			pos = file.findRev( '/' );
			base = file.left( ++pos );
			chdir( base );
			base = file.right( file.length()-pos );
			file = base;
		}
		archProcess << file;
		url = urls->next();
	}while( !url.isNull() );
	archProcess.start(KProcess::Block);
	listing->clear();

	//Argh: should not be commented
	//openArch( archname );
	return 0;
//	cout << "left addFile" << endl;
}

void ZipArch::extractTo( QString dest )
{
//	cout << "Got in extractTo" << endl;
	FILE *fd;
	char line[4096];
	
	archProcess.clearArguments();
//	archProcess.setExecutable( "unzip" );
	archProcess << "unzip" << "-o";
	if( tolower )
		archProcess << "-L";
	archProcess << archname << "-d" << dest;
 	if(archProcess.startPipe(KProcess::Stdout, &fd) == false)
 	{
 		cerr << "Subprocess wouldn't start!" << endl;
 		return;
 	}
  	newProgressDialog( 1, listing->count() );
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

QString ZipArch::unarchFile( int pos, QString dest )
{
//  	cout << "entered unarchFile" << endl;
	QString tmp, tmp2;

	archProcess.clearArguments();
// 	archProcess.setExecutable("unzip");
	tmp = listing->at( pos );
	tmp2 = tmp.right( (tmp.length())-(tmp.findRev('\t')+1) );
	archProcess << "unzip" << "-o" << archname << tmp2 << "-d" << dest;
 	archProcess.start(KProcess::Block);
	return (dest+tmp2);
//  	cout << "left unarchFile" << endl;
}

void ZipArch::deleteFile( int pos )
{
//	cout << "Entered deleteFile" << endl;
	QString name, tmp;

	archProcess.clearArguments();
 	archProcess.setExecutable("zip");
	tmp = listing->at( pos );
	name = tmp.right( (tmp.length())-(tmp.findRev('\t')+1) );
 	archProcess << "-d" << archname << name;
 	archProcess.start(KProcess::Block);
	listing->clear();

	// Argh:  should not be commented
	//openArch( archname );
//	cout << "Left deleteFile" << endl;
}

