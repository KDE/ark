 /*
  Copyright (C)

  2001: Corel Corporation (author: Michael Jarrett <michaelj@corel.com>)

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

// C includes
#include <unistd.h>

// QT includes
#include <qstringlist.h>
#include <qstring.h>
#include <qheader.h>

// KDE includes
#include <klocale.h>
#include <kdebug.h>
#include <kstddirs.h>

// Ark includes
#include "filelistview.h"
#include "arch.h"
#include "shellOutputDlg.h"
#include "arksettings.h"
#include "arkwidgetbase.h"


// Protected - so the average Joe can't instantize
ArkWidgetBase::ArkWidgetBase(QWidget *widget)
	: m_widget(widget), arch(0), m_settings(0), archiveContent(0),
	m_archType(UNKNOWN_FORMAT), m_nSizeOfFiles(0),
	m_nSizeOfSelectedFiles(0), m_nNumFiles(0), m_nNumSelectedFiles(0),
	m_bIsArchiveOpen(false), m_bIsSimpleCompressedFile(false),
	m_bDragInProgress(false), m_bDropSourceIsSelf(false),
	m_extractList(0)
{
    m_settings = new ArkSettings;

    // Creates a temp directory for this ark instance
    unsigned int pid = getpid();
    QString tmpdir,directory;
    directory.sprintf( "ark.%d/", pid );
    tmpdir = locateLocal( "tmp", directory );

    m_settings->setTmpDir( tmpdir );
}

/**
* Destroys anything dynamic inside us.
*/
ArkWidgetBase::~ArkWidgetBase()
{
  kdDebug(1601) << "ArkWidget::~ArkWidgetBase" << endl;

  if(archiveContent) delete archiveContent;
  if(arch) delete arch;
  if(m_settings) delete m_settings;
}

/**
* @param _columnHeader The name of the column (== the display text)
* @return The column matching columnHeader, or -1 if not found
*/
int ArkWidgetBase::getCol(const QString & _columnHeader)
{
  // return the column corresponding to the header, or -1 for failure
  int column;
  for (column = 0; column < archiveContent->header()->count(); ++column)
    {
      if (archiveContent->columnText(column) == _columnHeader)
	{
	  return column;
	}
    }

  kdError(1601) << "Can't find header " << _columnHeader << endl;
  return -1;
}

/**
* Returns the data in a file column
* @param _filename The filename in question to reference in the archive
* @param _col The 0-indexed column whose contents is desired
* @return QString of column data
*/
QString ArkWidgetBase::getColData(const QString &_filename, int _col)
{
  FileListView *flw = fileList();
  FileLVI *flvi = (FileLVI*)flw->firstChild();
  while (flvi)
    {
      QString curFilename = flvi->getFileName();
      if (curFilename == _filename)
	return (flvi->text(_col));
      flvi = (FileLVI*)flvi->itemBelow();
    }
  kdError(1601) << "Couldn't find " << _filename << " in ArkWidget::getColData"
		<< endl;

  return QString(QString::null);
}

/**
* @return 0-indexed column number that contains uncompressed file sizes
*/
int ArkWidgetBase::getSizeColumn()
{
  for (int i = 0; i < archiveContent->header()->count(); ++i)
    if (archiveContent->columnText(i) == SIZE_STRING)
      return i;
  return -1;
}


/**
* Adds a file and stats to the file listing
* @param _entries A stringlist of the entries for each column of the list.
*/
void ArkWidgetBase::listingAdd(QStringList *_entries)
{
  FileLVI *flvi = new FileLVI( fileList() );
  int i = 0;
  for ( QStringList::Iterator it = _entries->begin();
	it != _entries->end(); ++it )
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
void ArkWidgetBase::setHeaders(QStringList *_headers,
			       int * _rightAlignCols, int _numColsToAlignRight)
{
  int i = 0;
  m_currentSizeColumn = -1;

  clearHeaders();

  for ( QStringList::Iterator it = _headers->begin();
	it != _headers->end(); ++it, ++i )
    {
      QString str = *it;
       archiveContent->addColumn(str);
       if (SIZE_STRING == str)
	 m_currentSizeColumn = i;
    }

  for (int i = 0; i < _numColsToAlignRight; ++i)
    {
      archiveContent->setColumnAlignment( _rightAlignCols[i],
					  QListView::AlignRight );
    }
}

/**
* Clears all headers from the file list
*/
void ArkWidgetBase::clearHeaders()
{
	while(archiveContent->columns() > 0)
		archiveContent->removeColumn(0);
}

/**
* Brings up a dialog showing the results returned by the last cmdline tool.
* @param parent Parent widget of the dialog.
*/
void ArkWidgetBase::viewShellOutput()
{
  ShellOutputDlg* sod = new ShellOutputDlg(m_settings, m_widget);
  sod->exec();
  delete sod;
}

/**
* Special form of extract that uses the temp directory and forces
* directory junk options to be ignored.
* @param fileList Files to extract
*/
void ArkWidgetBase::prepareViewFiles(QStringList *fileList)
{
      arch->unarchFile(fileList, m_settings->getTmpDir(), true);
}


/**
* Miscellaneous tasks involved in closing an archive.
*/
void ArkWidgetBase::closeArch()
{
	if(isArchiveOpen())
	{
		delete arch;
		arch = 0;
		m_bIsArchiveOpen = false;
	}
	
	if (0 != archiveContent)
	{
		archiveContent->clear();
		clearHeaders();
	}
}

