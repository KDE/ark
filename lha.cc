/* (c)1997 Robert Palmbos
   See main.cc for license details */

#include <iostream.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/errno.h>

// KDE includes
#include <kurl.h>

// ark includes
#include "filelistview.h"
#include "lha.h"

LhaArch::LhaArch( ArkSettings *d )
  : Arch()
{
	listing = new QStringList;
	data = d;
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

void LhaArch::openArch( const QString & file, FileListView *flw )
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

	flw->clear();
	flw->addColumn( i18n("Name") );
	flw->addColumn( i18n("Permissions") );
	flw->addColumn( i18n("Owner/Group") );
	flw->addColumn( i18n("Packed") );
	flw->addColumn( i18n("Size") );
	flw->addColumn( i18n("Ratio") );
	flw->addColumn( i18n("CRC") );
	flw->addColumn( i18n("TimeStamp") );

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
		FileLVI *flvi = new FileLVI(flw);
		flvi->setText(0, QString::fromLocal8Bit(filename));
		for(int i=0; i<7; i++)
		{
                	flvi->setText(i+1, QString::fromLocal8Bit(columns[i]));
		}
		flw->insertItem(flvi);

			sprintf(line, "%s\t%s\t%s\t%s\t%s\t%s\t"
				"%s\t%s",
				columns[0],columns[1],columns[2],columns[3],
				columns[4],columns[5],columns[6],filename);
			listing->append( QString::fromLocal8Bit(line) );
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

void LhaArch::createArch( const QString & file )
{
	archname = file;
}

const QStringList *LhaArch::getListing()
{
	return listing;
}


int LhaArch::addFile( QStringList *urls )
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
		file = KURL(url).path(-1); // remove trailing slash
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

	// Argh : should not be commented
//	openArch( archname );
	return 0;
//	cout << "left addFile" << endl;
}

void LhaArch::extractTo( const QString & dest )
{
	FILE *fd;
	char line[4096];
	archProcess.clearArguments();
	archProcess.setExecutable( "lha" );
	archProcess << "xfw="+dest << archname;

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

QString LhaArch::unarchFile(const QString & _filename )
{
  return "";
}

QString LhaArch::unarchFile( )
{
//  	cout << "entered unarchFile" << endl;
	QString tmp, tmp2;
  QString dest = m_settings->getExtractDir();

// Segfault testing
//	QStrList segflist	= archProcess.getArguments();
//	for (const char* f = segflist.first(); f; f = segflist.next())
//		cout << "Argument:" << f << endl;

	archProcess.clearArguments();
 	archProcess.setExecutable("lha");
	tmp = (*listing).[pos];
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
	tmp = (*listing)[pos];
	name = tmp.right( (tmp.length())-(tmp.findRev('\t')+1) );
 	archProcess << "df" << archname << name;
 	archProcess.start(KProcess::Block);
	listing->clear();

	//Argh : should not be commented
	//openArch( archname );
	cout << "Left deleteFile" << endl;
}

