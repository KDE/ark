/*
 * Copyright (C)
 * 
 * 2002: Helio Chissini de Castro <helio@conectiva.com.br>
 * 2001: Corel Corporation (author: Michael Jarrett <michaelj@corel.com>)
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

// C includes
#include <unistd.h>
#include <stdlib.h>

// QT includes
#include <qdir.h>

// KDE includes
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kprocess.h>

// Ark includes
#include "filelistview.h"
#include "shellOutputDlg.h"
#include "arksettings.h"
#include "arkwidgetbase.h"


// Protected - so the average Joe can't instantize
ArkWidgetBase::ArkWidgetBase(QWidget *widget)
	: m_widget(widget), arch(0), m_settings(0), archiveContent(0),
	m_archType(UNKNOWN_FORMAT), m_nSizeOfFiles(0),
	m_nSizeOfSelectedFiles(0), m_nNumFiles(0), m_nNumSelectedFiles(0),
	m_bIsArchiveOpen(false), m_bIsSimpleCompressedFile(false),
	m_bDropSourceIsSelf(false), m_extractList(0)
{
	m_settings = new ArkSettings;
	
	// Creates a temp directory for this ark instance
    //getpid() doesn't help here, since we can have many arkkparts
    //embedded in one Konqueror, all with the same pid
	//unsigned int pid = getpid();
	QString tmpdir, directory;

    int count=0;
    QDir dir;
    srand( getpid() );
    for( ; count <= 255; count++ )
    {
        //no trailing slash here, otherwise the dir is created by locateLocal if
        //it doesn't exist
	    directory.sprintf( "ark.%d", rand() );
	    tmpdir = locateLocal( "tmp", directory );
        kdDebug( 1601 )<< "ArkWidgetBase::ArkWidgetBase tmpdir: " << tmpdir << " exists( " << dir.exists( tmpdir ) << " )"<< endl;
        if( !dir.exists( tmpdir ) )
            break;
    }
    if( count < 255 && dir.mkdir( tmpdir ) )
        m_settings->setTmpDir( tmpdir );
    else
        kdWarning( 1601 ) << "Could not create a temporary directory." << endl;
}

/**
* Destroys anything dynamic inside us.
*/
ArkWidgetBase::~ArkWidgetBase()
{
	kdDebug(1601) << "ArkWidget::~ArkWidgetBase" << endl;
	// avoid race condition, so that archiveContent isn't used while being deleted
	delete archiveContent;
	archiveContent = 0;
	delete arch;
	delete m_settings;
}

void
ArkWidgetBase::cleanArkTmpDir( bool part )
{
    QString tmpdir = m_settings->getTmpDir();
    KProcess proc;
    proc << "rm" << "-rf" << tmpdir;
    proc.start(KProcess::Block);
    return;
}

/**
 * Returns the file item, or 0 if not found.
 * @param _filename The filename in question to reference in the archive
 * @return The requested file's FileLVI
 */
const FileLVI *
ArkWidgetBase::getFileLVI(const QString &_filename) const
{
	FileListView *flw = fileList();
	FileLVI *flvi = (FileLVI*)flw->firstChild();
	while (flvi)
	{
		QString curFilename = flvi->fileName();
		if (curFilename == _filename)
		{
			return flvi;
		}
		flvi = (FileLVI*)flvi->itemBelow();
	}
	
	return 0;
}


/**
* Adds a file and stats to the file listing
* @param _entries A stringlist of the entries for each column of the list.
*/
void 
ArkWidgetBase::listingAdd(QStringList *_entries)
{
	FileLVI *flvi = new FileLVI( fileList() );
	int i = 0;
	for ( QStringList::Iterator it = _entries->begin(); it != _entries->end(); ++it )
	{
		flvi->setText(i, *it);
		++i;
	}
}


/**
* Sets up the column headers for the file list. Clears previous entries
* before adding new ones.
* @param _headers A list of headers to add.
* @param _rightAlignCols An array of ints representing columns to right-align.
* @param _numColsToAlignRight Size of _rightAlignCols
*/
void 
ArkWidgetBase::setHeaders(QStringList *_headers, int * _rightAlignCols, int _numColsToAlignRight)
{
	int i = 0;
	
	clearHeaders();
	
	for ( QStringList::Iterator it = _headers->begin(); it != _headers->end(); ++it, ++i )
	{
		QString str = *it;
		archiveContent->addColumn(str);
	}

	for (int i = 0; i < _numColsToAlignRight; ++i)
	{
		archiveContent->setColumnAlignment( _rightAlignCols[i], QListView::AlignRight );
	}
}

/**
* Clears all headers from the file list
*/
void 
ArkWidgetBase::clearHeaders()
{
	while(archiveContent->columns() > 0)
	{
		archiveContent->removeColumn(0);
	}
}

/**
* Brings up a dialog showing the results returned by the last cmdline tool.
* @param parent Parent widget of the dialog.
*/
void 
ArkWidgetBase::viewShellOutput()
{
	ShellOutputDlg* sod = new ShellOutputDlg(m_settings, m_widget);
	sod->exec();
	delete sod;
}

/**
* Miscellaneous tasks involved in closing an archive.
 */
void
ArkWidgetBase::closeArch()
{
    if( isArchiveOpen() )
    {
        delete arch;
        arch = 0;
        m_bIsArchiveOpen = false;	
    }

    if ( 0 != archiveContent )
    {
        archiveContent->clear();
        clearHeaders();
    }
}


