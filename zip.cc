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


// Qt includes
#include <qstringlist.h>

// KDE includes
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetypes.h>
#include <kpixmapcache.h>
#include <kurl.h>

// ark includes
#include "arkwidget.h"
#include "zip.h"
#include "zipAddDlg.h"
#include "zipExtractDlg.h"
#include "zip.moc"

ZipArch::ZipArch( ArkData *_d, ArkWidget *_mainWindow, FileListView *_fileListView )  
	: QObject(), Arch()
{
	m_data = _d;
	m_arkwidget = _mainWindow;
	m_flw = _fileListView;
}

ZipArch::~ZipArch()
{
}

int ZipArch::getEditFlag()
{
	return Arch::Extract | Arch::Delete | Arch::Add;
}

void ZipArch::slotProcessusKilled()
{
	m_kp->kill();
}

void ZipArch::slotStoreDataStdout(KProcess* _p, char* _data, int _length)
{
	char c = _data[_length];
	_data[_length] = '\0';

	m_data->appendShellOutputData( _data );
	_data[_length] = c;
}

void ZipArch::slotStoreDataStderr(KProcess* _p, char* _data, int _length)
{
	char c = _data[_length];
	_data[_length] = '\0';
	
	m_shellErrorData.append( _data );
	_data[_length] = c;
}

bool ZipArch::stderrIsError()
{
	const QString err("error");
	return m_shellErrorData.find(err) != -1;
}

void ZipArch::slotOpenDataStdout(KProcess* _p, char* _data, int _length)
{
	char columns[8][80];
	char filename[4096];

//	kdebug(0, 1601, "+slotOpenDataStdout");

//	if( !(++m_steps % 100) )
//		kapp->processEvents();
	
	char c = _data[_length];
	_data[_length] = '\0';
	
	m_data->appendShellOutputData( _data );

	char line[1024] = "";
	char *tmpl = line;
//	tmpl = line;
//	*tmpl = '\0';

	char *tmpb;
	
	for( tmpb = m_buffer; *tmpb != '\0'; tmpl++, tmpb++ )
		*tmpl = *tmpb;

	for( tmpb = _data; *tmpb != '\n'; tmpl++, tmpb++ )
		*tmpl = *tmpb;
		
	tmpb++;	*tmpl = '\0';

	if( *tmpb == '\0' )
		m_buffer[0]='\0';

	if( !strstr( line, "----" ) )
	{
		if( m_header_removed && !m_finished ){

			sscanf(line, " %[0-9] %[a-zA-Z:] %[0-9] %[0-9%] %[-0-9] %[0-9:] "
			"%[0-9a-z]%3[ ]%[^\n]",
			columns[0], columns[1], columns[2], columns[3],
			columns[4], columns[5], columns[6], columns[7],
			filename
			);

			FileLVI *flvi = new FileLVI(m_flw);
			flvi->setText(0, filename);

			for(int i=0; i<7; i++)
			{
				flvi->setText(i+1, columns[i]);
			}
			m_flw->insertItem(flvi);
		}
	}
	else if(!m_header_removed)
		m_header_removed = true;
	else
		m_finished = true;

	bool stop = (*tmpb == '\0');

	while( !stop && !m_finished )
	{
		tmpl = line;
		*tmpl = '\0';

		for(; (*tmpb!='\n') && (*tmpb!='\0'); tmpl++, tmpb++)
			*tmpl = *tmpb;

		if( *tmpb == '\n' ){
			*tmpl = '\n';  	tmpl++;
			*tmpl = '\0';  	tmpb++;

			if( !strstr( line, "----" ) )
			{
				if( m_header_removed ){

				sscanf(line, " %[0-9] %[a-zA-Z:] %[0-9] %[0-9%] %[-0-9] %[0-9:] "
				"%[0-9a-z]%3[ ]%[^\n]",
				columns[0], columns[1], columns[2], columns[3],
				columns[4], columns[5], columns[6], columns[7],
				filename
				);

				FileLVI *flvi = new FileLVI(m_flw);
				flvi->setText(0, filename);

				for(int i=0; i<7; i++)
				{
					flvi->setText(i+1, columns[i]);
				}
				m_flw->insertItem(flvi);
				}
			}
			else if( !m_header_removed )
				m_header_removed = true;
			else{
				m_finished = true;
			}
		}
		else if( *tmpb == '\0' ){
			*tmpl = '\0';
			strcpy( m_buffer, line );
			stop = true;
		}
	}

	_data[_length] = c;
}

void ZipArch::initListView()
{
	kdebug(0, 1601, "+ZipArch::initListView");
	
	m_flw->addColumn( i18n("Name") );
	m_flw->addColumn( i18n("Length") );
	m_flw->setColumnAlignment( 1, QListView::AlignRight );
	m_flw->addColumn( i18n("Method") );
	m_flw->addColumn( i18n("Size") );
	m_flw->setColumnAlignment( 3, QListView::AlignRight );
	m_flw->addColumn( i18n("Ratio") );
	m_flw->setColumnAlignment( 4, QListView::AlignRight );
	m_flw->addColumn( i18n("Date") );
	m_flw->addColumn( i18n("Time") );
	m_flw->addColumn( i18n("CRC-32") );
	m_flw->setColumnAlignment( 7, QListView::AlignRight );

	kdebug(0, 1601, "-ZipArch::initListView");
}

void ZipArch::initOpen()
{
	kdebug(0, 1601, "+ZipArch::initOpen");
	
	m_buffer[0] = '\0';
	m_header_removed = false;
	m_finished = false;
	m_steps = 0;
	
	m_data->clearShellOutput();

	m_kp = new KProcess();
	*m_kp << "unzip" << "-v" << m_filename.ascii();
	
	connect( m_kp, SIGNAL(receivedStdout(KProcess*, char*, int)), SLOT(slotOpenDataStdout(KProcess*, char*, int)));

//	showWait();
	
	kdebug(0, 1601, "-ZipArch::initOpen");
}

void ZipArch::openArch( QString _filename )
{
	kdebug(0, 1601, "+ZipArch::openArch");

	m_filename = _filename;
	
	initListView();
	initOpen();

 	if(m_kp->start(KProcess::Block, KProcess::Stdout) == false)
 	{
 		KMessageBox::error( 0, i18n("Subprocess wouldn't start!") );
 		return;
 	}
	kdebug(0, 1601, "process stopped");

//	m_wd->close();
//	delete m_wd;

	kdebug(0, 1601, "normalExit = %d", m_kp->normalExit() );
	if( m_kp->normalExit() )
		kdebug(0, 1601, "exitStatus = %d", m_kp->exitStatus() );

	if( m_kp->normalExit() && !m_kp->exitStatus() )
		m_arkwidget->open_ok( m_filename );
	else{
		m_flw->clear();
		m_arkwidget->open_fail();
	}

	delete m_kp;
	
	kdebug(0, 1601, "-ZipArch::openArch");
}


void ZipArch::createArch( QString _filename )
{
	m_filename = _filename;
}


void ZipArch::add( QString _location, int _mode, QString _compression, bool _recurse, bool _junk, bool _msdos, bool _convertLF )
{
	kdebug(0, 1601, "+ZipArch::add");
	
	m_kp = new KProcess();
			
	*m_kp << "zip";
	
	if( _recurse ) *m_kp << "-r";
		
	*m_kp << _compression.ascii();
	
	if( _junk ) *m_kp << "-j";
		
	if( _msdos ) *m_kp << "-k";
			
	if( _convertLF ) *m_kp << "-l";
	
	switch( _mode ){
		case Update : *m_kp << "-u"; break;
		case Freshen : *m_kp << "-f"; break;
		case Move : *m_kp << "-m"; break;
	}
	
	*m_kp << m_filename << _location.ascii();
	
 	if(m_kp->start(KProcess::Block, KProcess::Stdout) == false)
 	{
 		KMessageBox::error( 0, i18n("Subprocess wouldn't start!") );
 	}

	kdebug(0, 1601, "normalExit = %d", m_kp->normalExit() );
	kdebug(0, 1601, "exitStatus = %d", m_kp->exitStatus() );

	if( m_kp->normalExit() && m_kp->exitStatus() ){
 		KMessageBox::sorry( 0, "Add failed" );
 	}

 	delete m_kp;

	kdebug(0, 1601, "-ZipArch::add");
}


int ZipArch::addFile( QStrList *urls )
{
  	kdebug(0, 1601, "+ZipArch::addFile");

	ZipAddDlg *zad = new ZipAddDlg( this, m_data, m_data->getAddDir() );  	
	zad->exec();
  	delete zad;

  	kdebug(0, 1601, "+ZipArch::addFile");
}

QString ZipArch::unarchFile( int pos, QString dest )
{
	kdebug(0, 1601, "+ZipArch::unarchFile");

	QString tmp;
	
	m_kp = new KProcess();
	
	*m_kp << "unzip" << "-o" << m_filename;
	
	FileLVI * flvi = (FileLVI*)m_flw->firstChild();
	while (flvi)
	{
		if( m_flw->isSelected(flvi) ){
			kdebug(0, 1601, "unarch %s", flvi->getFileName().ascii() );
			tmp = flvi->getFileName().ascii();
			*m_kp << tmp.ascii();
		}
		flvi = (FileLVI*)flvi->itemBelow();
	}

	*m_kp << "-d" << dest;
	
 	if(m_kp->start(KProcess::Block, KProcess::Stdout) == false)
 	{
 		KMessageBox::error( 0, i18n("Subprocess wouldn't start!") );
 	}

	kdebug(0, 1601, "normalExit = %d", m_kp->normalExit() );
	kdebug(0, 1601, "exitStatus = %d", m_kp->exitStatus() );

	if( m_kp->normalExit() && m_kp->exitStatus() ){
 		KMessageBox::sorry( 0, "Unarch failed" );
 	}

 	delete m_kp;
 		
	kdebug(0, 1601, "-ZipArch::unarchFile");
	
	return (dest+tmp);	
}

void ZipArch::removeSelectedItems()
{
	FileLVI* flvi = (FileLVI*)m_flw->firstChild();
        FileLVI* old_flvi;
	while (flvi)
	{
		if( m_flw->isSelected(flvi) ){
			old_flvi = flvi;
			flvi = (FileLVI*)flvi->itemBelow();
			delete old_flvi;
		}		
		else flvi = (FileLVI*)flvi->itemBelow();
	}
}

void ZipArch::deleteSelectedFiles()
{
	kdebug(0, 1601, "+ZipArch::deleteSelectedFiles");

	m_data->clearShellOutput(); m_shellErrorData = "";
	
	m_kp = new KProcess();
		
 	*m_kp << "zip" << "-d" << m_filename.ascii();
 	
	FileLVI* flvi = (FileLVI*)m_flw->firstChild();
	while (flvi)
	{
		if( m_flw->isSelected(flvi) ){
			kdebug(0, 1601, "delete %s", flvi->getFileName().ascii() );
			*m_kp << flvi->getFileName().ascii();
		}
		flvi = (FileLVI*)flvi->itemBelow();
	}
 	
	connect( m_kp, SIGNAL(receivedStdout(KProcess*, char*, int)), SLOT(slotStoreDataStdout(KProcess*, char*, int)));
	connect( m_kp, SIGNAL(receivedStderr(KProcess*, char*, int)), SLOT(slotStoreDataStderr(KProcess*, char*, int)));
 	
 	if(m_kp->start(KProcess::Block, KProcess::AllOutput) == false)
 	{
 		KMessageBox::error( 0, i18n("Subprocess wouldn't start!") );
 		return;
 	}
	
	kdebug(0, 1601, "normalExit = %d", m_kp->normalExit() );
	kdebug(0, 1601, "exitStatus = %d", m_kp->exitStatus() );
	
	if( m_kp->normalExit() && (m_kp->exitStatus()==0) )
	{
		if(stderrIsError())
		{
	 		KMessageBox::error( 0, i18n("You probably don't have sufficient permissions\n"
	 					"Please check the file owner and the integrity\n"
	 					"of the archive.") );
		}
		else removeSelectedItems();
	}
	else
 		KMessageBox::sorry( 0, i18n("Deletion failed") );

 	delete m_kp;
 		
	kdebug(0, 1601, "-ZipArch::deleteSelectedFiles");
}

void ZipArch::slotExtractExited(KProcess *)
{
	kdebug(0, 1601, "+slotExtractExited");

	kdebug(0, 1601, "normalExit = %d", m_kp->normalExit() );
	if( m_kp->normalExit() )
		kdebug(0, 1601, "exitStatus = %d", m_kp->exitStatus() );

	m_wd->close();

		
	if( m_kp->normalExit() && (m_kp->exitStatus()==0) )
	{
		if(stderrIsError())
		{
	 		KMessageBox::error( 0, i18n("You probably don't have sufficient permissions\n"
	 					"Please check the file owner and the integrity\n"
	 					"of the archive.") );
		}
	}
	else
 		KMessageBox::sorry( 0, i18n("Extraction failed") );
		
	delete m_kp;
		
	kdebug(0, 1601, "-slotExtractExited");
}

void ZipArch::initExtract( bool _overwrite, bool _junkPaths, bool _lowerCase)
{
	m_data->clearShellOutput();
	m_shellErrorData = "";

	m_kp = new KProcess();
		
	*m_kp << "unzip";
		
	if( _overwrite )
		*m_kp << "-o";
	else
		*m_kp << "-n";
	
	if( _junkPaths )
		*m_kp << "-j";
		
	if( _lowerCase )
		*m_kp << "-L";
		
	*m_kp << m_filename;

	connect( m_kp, SIGNAL(processExited(KProcess *)), SLOT(slotExtractExited(KProcess *)));
	connect( m_kp, SIGNAL(receivedStdout(KProcess*, char*, int)), SLOT(slotStoreDataStdout(KProcess*, char*, int)));
	connect( m_kp, SIGNAL(receivedStderr(KProcess*, char*, int)), SLOT(slotStoreDataStderr(KProcess*, char*, int)));
}

void ZipArch::extraction()
{

 	ZipExtractDlg *zed=new ZipExtractDlg( m_data, !m_flw->isSelectionEmpty(), m_data->getExtractDir() );
 	if( zed->exec() ){
 		
		QString dir;

		dir = zed->getDestination();
		QDir dest( dir );
		
		if( !dest.exists() ) {
			if( mkdir( dir.ascii(), S_IWRITE | S_IREAD | S_IEXEC ) ) {
				KMessageBox::error( 0, i18n("Unable to create destination directory") );
				return;
			}
		}
	
		initExtract( zed->overwrite(), zed->junkPaths(), zed->lowerCase() );		
		
		switch( zed->selection() )
		{
			case ZipExtractDlg::All: break;
			case ZipExtractDlg::Selection: {
				QStringList * list = m_flw->selectedFilenames();
				QStringList::Iterator it = list->begin();
				
				for ( ; it != list->end(); it++ )
				{
					*m_kp << *it;
				}	
				break;		
			}
			case ZipExtractDlg::Pattern: {
				*m_kp << zed->pattern();
				break;		
			}
	                default: kdebug(3, 1601, "ZipArch::extraction(): unhandled value in switch");
		}
		
		*m_kp << "-d" << dir.ascii(); 	

 		if(m_kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
	 	{
 			kdebug(0, 1601, "Subprocess wouldn't start!");
 			return;
	 	}

		m_wd = new WaitDlg();
		connect(m_wd, SIGNAL(dialogClosed()), SLOT(slotProcessusKilled()));
		m_wd->exec();	
	}
	delete zed;
}

void ZipArch::slotIntegrityExited(KProcess *)
{
	kdebug(0, 1601, "+slotIntegrityExited");

	kdebug(0, 1601, "normalExit = %d", m_kp->normalExit() );
	kdebug(0, 1601, "exitStatus = %d", m_kp->exitStatus() );
		
	if( m_kp->normalExit() && (m_kp->exitStatus()==0) )
	{
		if(stderrIsError())
		{
	 		KMessageBox::error( 0, i18n("You probably don't have sufficient permissions\n"
	 					"Please check the file owner and the integrity\n"
	 					"of the archive.") );
		}
	}
	else
 		KMessageBox::sorry( 0, i18n("Test of integrity failed") );
		
	delete m_kp;
		
	kdebug(0, 1601, "-slotIntegrityExited");
}

void ZipArch::testIntegrity()
{
	m_data->clearShellOutput();
	m_shellErrorData = "";

	m_kp = new KProcess();
		
	*m_kp << "unzip -t";
		
	*m_kp << m_filename;

	connect( m_kp, SIGNAL(processExited(KProcess *)), SLOT(slotIntegrityExited(KProcess *)));
	connect( m_kp, SIGNAL(receivedStdout(KProcess*, char*, int)), SLOT(slotStoreDataStdout(KProcess*, char*, int)));
//	connect( m_kp, SIGNAL(receivedStderr(KProcess*, char*, int)), SLOT(slotStoreDataStderr(KProcess*, char*, int)));
 		
 	if(m_kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
	{
 		kdebug(0, 1601, "Subprocess wouldn't start!");
 		return;
	}
}

#if 0
void ZipArch::showWait()
{
	m_wd = new WaitDlg( m_arkwidget, "", false );
	connect( m_wd, SIGNAL(dialogClosed()), SLOT( slotProcessusKilled()) );
	m_wd->show();
	kapp->processEvents();
	m_wd->update();
	kapp->processEvents();
}
#endif
