/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/


#include <kurl.h>
// Unsorted in qdir.h is used, but in some of the headers
// below it's defined, too. So I brought kurl.h to the top.
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

// Qt includes
#include <qmessagebox.h>

// KDE includes
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

// ark includes
#include "viewer.h"
#include "extractdlg.h"
#include "tar.h"
#include "tar.moc"

TarArch::TarArch( ArkSettings *_settings, Viewer *_gui, FileListView *_flw )
	: Arch(_settings, _gui, "")
{
	kdebug(0, 1601, "+TarArch::TarArch");
	
	stdout_buf = NULL;
	cout << "Entered TarArch" << endl;
	m_flw = _flw;
	
	listing = new QStringList;
	m_settings->setaddPath( false );
	compressed = true;
	
	m_settings->getTarCommand();
	
	kdebug(0, 1601, "-TarArch::TarArch");
}

TarArch::~TarArch()
{
	cout << "Entered ~TarArch" << endl;
	updateArch();
//	delete listing;
	cout << "Left ~TarArch" << endl;
}

int TarArch::getEditFlag()
{
	return Arch::Extract;
}

int TarArch::updateArch()
{
	if( !compressed )
	{
		compressed = TRUE;
		disconnect( &kproc, 0, 0, 0 );
		kproc.clearArguments();
		kproc << getCompressor() << "-c" << tmpfile.local8Bit() << " > " 
			<< m_filename.local8Bit();
		connect( &kproc, SIGNAL(processExited(KProcess *)),
		                       this, SLOT(openFinished(KProcess *)) );
		if( kproc.start( KProcess::NotifyOnExit ) == FALSE )
		{
			KMessageBox::error(0, "Can't fork a compressor.");
			return -1;
		}
	}
	return 0;
}

QString TarArch::getCompressor() 
{
	QString extension = m_filename.right( m_filename.length()-m_filename.findRev('.') );
	cout << extension;
	if( extension == ".tgz" || extension == ".gz" ) 
		return QString( "gzip" );
	if( extension == ".bz" )
		return QString( "bzip" );
	if( extension == ".Z" || extension == ".taz" )
		return QString( "compress" );
	if( extension == ".bz2" )
		return QString( "bzip2" );
	if( extension == ".lzo" || extension == ".tzo" )
		return QString( "lzop" );
	return QString::null;
}

QString TarArch::getUnCompressor() 
{
	QString extension = m_filename.right( m_filename.length()-m_filename.findRev('.') );
	cout << extension;
	if( extension == ".tgz" || extension == ".gz" ) 
		return QString( "gunzip" );
	if( extension == ".bz" )
		return QString( "bunzip" );
	if( extension == ".Z" || extension == ".taz" )
		return QString( "uncompress" );
	if( extension == ".bz2" )
		return QString( "bunzip2" );
	if( extension == ".lzo" || extension == ".tzo" )
		return QString( "lzop" );
	return QString::null;
}

unsigned char TarArch::setOptions( bool p, bool l, bool o )
{
	perms = p;
	tolower = l;
	overwrite = o;
	return 5;
}

void TarArch::openArch( const QString & name )
{
	cout << "Entered openArch" << endl;
	QString buffer;
	destination_flw = m_flw;

	kproc.clearArguments();

	m_filename = name;
	QString tar_exe = m_settings->getTarCommand();
	
	kproc << tar_exe << "--use-compress-program="+getUnCompressor()
	      <<	"-tvf" << m_filename.local8Bit();
	
//	m_flw->addColumn( i18n("Name") );
//	m_flw->addColumn( i18n("Permissions") );
//	m_flw->addColumn( i18n("Owner/Group") );
//	m_flw->addColumn( i18n("Size") );
//	m_flw->addColumn( i18n("TimeStamp") );

	disconnect( &kproc, 0, 0, 0 );
	connect( &kproc, SIGNAL(processExited(KProcess *)), 
	                       this, SLOT(openFinished(KProcess *)));
	connect( &kproc, SIGNAL(receivedStdout(KProcess *, char *,int)),
	                       this, SLOT(inputPending(KProcess *, char *, int)));
	                       
	m_flw->addColumn( i18n("Filename") );
	m_flw->addColumn( i18n("Permissions") );
	m_flw->addColumn( i18n("Owner/Group") );
	m_flw->addColumn( i18n("Size") );
	m_flw->addColumn( i18n("TimeStamp") );

	if( kproc.start( KProcess::NotifyOnExit, KProcess::Stdout ) == FALSE )
	{
		KMessageBox::error(0, "Can't fork a decompressor.");
		return;
	}
//	fclose( fd );
//	There should be a file descriptor close call, but this one makes a
//	BAD FILEDESCRIPTOR error message
//	Another note: gzip reported 'Broken pipe' because of that fclose()
//	while(archProcess.isRunning())
//		;

	//	m_arkwidget->open_ok( m_filename );
	
	cout << "Left openArch" << endl;
}

void TarArch::createArch( const QString & file )
{
	cout << "Entered createArch" << endl;
	m_filename = file;
	cout << "Left createArch" << endl;
}

const QStringList * TarArch::getListing()
{
	return listing;
}

/* untested */
void TarArch::createTmp()
{
	cout << "Entered createTmp" << endl;
	if( compressed )
	{
//		FILE* fd, *fd2;
//		char buffer[4096];
//		int size = 0;
		compressed = FALSE;

		kproc.clearArguments();
		kproc << "gunzip" << "-c" << m_filename.local8Bit() << " > "
			 << tmpfile.local8Bit();
		
		disconnect( &kproc, 0, 0, 0 );
		connect(&kproc, SIGNAL(processExited(KProcess *)),
		                      this, SLOT(createTmpFinished(KProcess *)));
		
		if( kproc.start( KProcess::NotifyOnExit ) == FALSE )
		{
			KMessageBox::error(0, "Can't fork a decompressor");
		}
	}
}

/* untested, someone please tell me how DND works in KDE2.0 */
int TarArch::addFile( QStringList* urls )
{
	cout << "Entered addFile" << endl;

	int retcode;
	QString file, url, tmp;
	QString tar_exe = m_settings->getTarCommand();
		
	createTmp();

	url = urls->first();
	file = KURL(url).path(-1); // remove trailing slash

	kproc.clearArguments();
	kproc << tar_exe.local8Bit();
	
	if( m_settings->getonlyUpdate() )
		kproc << "uvf";
	else
		kproc << "rvf";
	kproc << tmpfile.local8Bit();
	
	QString base;

	if( !m_settings->getaddPath() )
	{
		int pos;
		pos = file.findRev( '/', -1, FALSE );
		base = file.left( ++pos );
		cout << "base is" << base << endl;
//		pos++;
		tmp = file.right( file.length()-pos );
		file = tmp;
		chdir( base.local8Bit() );
	}
	QStringList::Iterator it=urls->begin();
	while(1)
	{
		int pos;
		kproc << file.local8Bit();
		it++;
		url = *it;
//		cout << url << " is the name of the url " << endl;
		if( url.isNull() )
			break;
		file = KURL(url).path(-1); // remove trailing slash
		pos = file.findRev( '/', -1, FALSE );
		pos++;
		tmp = file.right( file.length()-pos );
		file = tmp;
	}	

	if( kproc.start( KProcess::NotifyOnExit, KProcess::Stdout ) == FALSE )
	{
		KMessageBox::error(0, "Can't start a kprocess.");
		return -1;
	}
	
	if( m_settings->getaddPath() )
		file.remove( 0, 1 );  // Get rid of leading /

	retcode = updateArch();
	return retcode;
	cout << "Left addFile" << endl;
}

#if 0
void TarArch::extraction()
{
	QString dir, ex;

	ExtractDlg ld( ExtractDlg::All );
	int mask = setOptions( FALSE, FALSE, FALSE );
	ld.setMask( mask );
	if( ld.exec() )
	{
		dir = ld.getDest();
		if( dir.isEmpty() )
			return;
		QDir dest( dir );
		if( !dest.exists() ) {
			if( mkdir( dir.local8Bit(), S_IWRITE | S_IREAD | S_IEXEC ) ) {
				//arkWarning( i18n("Unable to create destination directory") );
				return;
			}
		}
		setOptions( ld.doPreservePerms(), ld.doLowerCase(), ld.doOverwrite() );
		switch( ld.extractOp() ) {
			case ExtractDlg::All: {
				extractTo( dir );
				break;
			}
		}
	}
}	

void TarArch::extractTo( const QString & dir )
{
	cout << "Entered extractTo" << endl;

	QString tar_exe = m_settings->getTarCommand();
		
	kproc.clearArguments();
	kproc << tar_exe.local8Bit();
	kproc << "--use-compress-program="+getUnCompressor() 
	      <<	"-xvf" << m_filename.local8Bit() << "-C" << dir;	

	disconnect( &kproc, 0, 0, 0 );
	connect( &kproc, SIGNAL(processExited(KProcess *)), 
	                       this, SLOT(extractFinished(KProcess *)));
	connect( &kproc, SIGNAL(receivedStdout(KProcess *, char *,int)),
	                       this, SLOT(updateExtractProgress(KProcess *, char *, int)));

 	if(kproc.start( KProcess::NotifyOnExit, KProcess::Stdout ) == false)
 	{
 		KMessageBox::error(0,"Subprocess wouldn't start!");
 		return;
 	}

	cout << "Left extractTo" << endl;
}
#endif

/* untested */
QString TarArch::unarchFile( QStringList * _fileList)
{
  QString dest = m_settings->getExtractDir();
	cout << "Entered unarchFile" << endl;
	int pos;
	QString tmp, name;
	QString fullname;
	QString tar_exe = m_settings->getTarCommand();	
	
	updateArch();
	
	tmp = (*listing)[index];
	pos = tmp.findRev( '\t', -1, FALSE );
	pos++;
	name = tmp.right( tmp.length()-pos );

//	archProcess.clearArguments();
//	archProcess.setExecutable( tar_exe );
//	archProcess << tar_exe;

	kproc.clearArguments();
	kproc << tar_exe.local8Bit();
	
	if( perms )
		kproc << "--use-compress-program="+getUnCompressor() << "-xvpf";
	else
		kproc << "--use-compress-program="+getUnCompressor() << "-xvf";

	kproc << m_filename.local8Bit() << "-C" << dest.local8Bit()
		 << name.local8Bit();
	if( kproc.start(KProcess::Block) == false )
	{
		KMessageBox::error(0,"Can't start a kprocess.");
	}
	fullname = dest + name;
	cout << "Left unarchFile" << endl;
	return fullname;
}

void TarArch::deleteSelectedFiles()
{
	QMessageBox::warning(0, i18n("ark"), i18n("Sorry, not implemented yet !"), i18n("OK"));
}

void TarArch::deleteFiles( const QString& patterns )
{
	kdebug(0, 1601, "+Tar::deleteFiles");
	
	QString name, tmp;
	QString tar_exe = m_settings->getTarCommand();	
	
	createTmp();
	
	kproc.clearArguments();
	kproc << tar_exe.local8Bit() << "--delete" << "-f" << tmpfile.local8Bit() << patterns.local8Bit();
	kproc.start(KProcess::Block);
	
	updateArch();

	kdebug(0, 1601, "-Tar::deleteFiles");
}

void TarArch::updateExtractProgress( KProcess *, char *buffer, int bufflen )
{
	// some kind of progress bar, or something?

	// I know its ugly, but I wan't the names for future reference
 	buffer = buffer, bufflen = bufflen;
}

void TarArch::inputPending( KProcess *, char *buffer, int bufflen )
{
   char    columns[6][255];
   char    line[512];
   char    wline[512];
   char   *pos;
   char   *start;
   char   *mybuf;
   char   *tok;
   int     i;

   cout << ".";
   bufflen = bufflen;
    
   if( stdout_buf == NULL )
   	stdout_buf = strdup( "" );

   mybuf = (char *) malloc( strlen(stdout_buf) + bufflen+1 );
   strcpy( mybuf, stdout_buf );
   strncpy( &mybuf[strlen(stdout_buf)], buffer, bufflen );
   mybuf[strlen(stdout_buf)+bufflen] = 0;

   start = pos = mybuf;
   while( (pos = strchr( start, '\n' )) != NULL )
   {
   /* the sscanf doesn't work with symbolic links, I'm not sure why */
//       sscanf( start, " %[-drwxst] %[0-9.a-zA-Z/_] %[0-9] %17[a-zA-Z0-9:- ] %[^\n]",
//              columns[0], columns[1], columns[2], columns[3],
//               filename
//             );

		FileLVI *flvi = new FileLVI(destination_flw);
		strncpy( wline, start, pos-start );
		wline[pos-start] = 0;
		tok = strtok( wline, " " );
		strcpy( columns[0], tok );
		for( i=1; i<6; i++ )
		{
			tok = strtok( NULL, " " );
			strcpy( columns[i], tok );
		}
		strcat( columns[3], columns[4] );
		flvi->setText(0, QString::fromLocal8Bit(columns[5]));
		for(int i=0; i<4; i++)
		{
			flvi->setText(i+1, QString::fromLocal8Bit(columns[i]));
		}
		destination_flw->insertItem(flvi);

		sprintf(line, "%s\t%s\t%s\t%s\t%s",
		        columns[0], columns[1], columns[2], columns[3],
		        columns[5]);
		listing->append( QString::fromLocal8Bit(line) );
		start = (pos+1);
	}
	free( stdout_buf );
	stdout_buf = strdup( start );
	free( mybuf );

}

void TarArch::extractFinished( KProcess * )
{
	// turn off busy light (when someone makes one)
	cout << "Extract Finished." << endl;
}

void TarArch::openFinished( KProcess * )
{
    // do nothing
    // turn off busy light (when someone makes one)
    cout << "Open finshed" << endl;
}

void TarArch::createTmpFinished( KProcess * )
{
    // do nothing
    // turn off busy light (when someone makes one)
    cout << "Create Tmp finished" << endl;
}


// copy the working archive to the real archive
void TarArch::updateFinished( KProcess * )
{
#if 0
    FILE   *fd, fd2;
    char    buffer[4096];
    int     size = 0;

    // turn off busy light (when someone makes one)


    fd = fopen( tmpfile.local8Bit(), "r" );
    fd2 = fopen( m_filename, "w" );
		
    while( (size = fread( buffer, 1, 4096, fd )) )
        fwrite(buffer, size, 1, fd2);
    fclose(fd2);
#endif
    
// is this necessary?
//    if( !retcode )
//    {
//        listing->clear();
//        openArch( m_filename );
//    }

    cout << "Update finished" << endl;
}
