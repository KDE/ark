/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)

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
#include <qdir.h>
#include <qdragobject.h>
#include <qevent.h>
#include <qregexp.h>
#include <qheader.h>
#include <qwhatsthis.h>

// KDE includes
#include <kapp.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <qfiledialog.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kkeydialog.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kmessagebox.h>
#include <kstatusbar.h>
#include <ktoolbar.h>
#include <kio/netaccess.h>
#include <krun.h>
#include <kservice.h>
#include <kopenwith.h>
#include <kaction.h>
#include <kstdaction.h>
#include <ktempfile.h>
#include <progressbase.h>
#include <kmimemagic.h>
// c includes

#include <sys/stat.h>
#include <errno.h>

// ark includes
#include "arkapp.h"

#include "dirDlg.h"
#include "selectDlg.h"
#include "shellOutputDlg.h"

#include "extractdlg.h"
#include "adddlg.h"
#include "arch.h"
#include "arkwidget.h"
#include "filelistview.h"

// the archive types
#include "tar.h"
#include "zip.h"
#include "lha.h"
#include "compressedfile.h"
#include "zoo.h"
#include "rar.h"
#include "ar.h"

#include "viewer.h"

extern int errno;

bool Utilities::haveDirPermissions(const QString &strFile)
{
  struct stat statbuffer;
  QString dir = strFile.left(strFile.findRev('/'));
  stat(dir.local8Bit(), &statbuffer);
  unsigned int nFlag = 0;
  if (geteuid() == statbuffer.st_uid)
    {
      nFlag = S_IWUSR; // it's mine
    }
  else if (getegid() == statbuffer.st_gid)
    {
      nFlag = S_IWGRP; // it's my group's
    }
  else
    {
      nFlag = S_IWOTH;  // it's someone else's
    }
  if (! ((statbuffer.st_mode & nFlag) == nFlag))
    {
      KMessageBox::error(0, i18n("You don't have permission to write to the directory %1").arg(dir.local8Bit()));
      return false;
    }
  return true;
}

ArkWidget::ArkWidget( QWidget *, const char *name ) : 
    KTMainWindow(name), archiveContent(0),
    m_nSizeOfFiles(0), m_nSizeOfSelectedFiles(0),
    m_nNumFiles(0), m_nNumSelectedFiles(0), m_bIsArchiveOpen(false),
    m_bIsSimpleCompressedFile(false), m_bDropSourceIsSelf(false),
    m_bViewInProgress(false), m_bOpenWithInProgress(false),
    m_bEditInProgress(false),
    m_bMakeCFIntoArchiveInProgress(false), m_pTempAddList(NULL),
    m_bDropFilesInProgress(false), m_bDragInProgress(false), mpTempFile(NULL),
    mpDownloadedList(NULL), mpAddList(NULL)
{
    kdDebug(1601) << "+ArkWidget::ArkWidget" << endl;
  
    m_viewer = new Viewer(this);
    m_settings = new ArkSettings;
    // Creates a temp directory for this ark instance
    unsigned int pid = getpid();
    QString tmpdir;
    tmpdir.sprintf( "/tmp/ark.%d/", pid );
    QString ex( "mkdir " + tmpdir + " &>/dev/null" );
    system( QFile::encodeName(ex) );

    m_settings->setTmpDir( tmpdir );
    
    ArkApplication::getInstance()->addWindow();

    // Build the ark UI
    kdDebug(1601) << "Build the GUI" << endl;

    setupStatusBar();
    setupActions();
    createFileListView();
    // enable DnD
    setAcceptDrops(true);
    arch = 0;
    initialEnables();

    kdDebug(1601) << "-ArkWidget::ArkWidget" << endl;
    resize(640,300);
}

ArkWidget::~ArkWidget()
{
  ArkApplication::getInstance()->removeWindow();
  delete archiveContent;
  //  delete recentPopup;
  //  delete accelerators;
  //  delete m_filePopup;
  //  delete m_archivePopup;
  delete arch;

  // delete the temporary directory and its contents
  QString tmpdir = m_settings->getTmpDir();
  QString ex( "rm -rf "+tmpdir );
  system( QFile::encodeName(ex) );

  delete m_settings;
}

void ArkWidget::setupActions()
{
  // setup File menu
  newWindowAction = new KAction(i18n("New &Window"), 0, this,
				SLOT(file_newWindow()),
				actionCollection(), "new_window");

  newArchAction = KStdAction::openNew(this, SLOT(file_new()),
				      actionCollection());

  openAction = KStdAction::open(this, SLOT(file_open()), actionCollection());

  reloadAction = new KAction(i18n("&Reload"), "reload", 0, this,
				SLOT(file_reload()),
				actionCollection(), "reload_arch");

  saveAsAction = KStdAction::saveAs(this, SLOT(file_save_as()),
				    actionCollection());

  closeAction = new KAction(i18n("&Close Archive"), 0, this,
			    SLOT(file_close()),
			    actionCollection(), "close_arch");

  recent = KStdAction::openRecent(this,
				  SLOT(file_open(const KURL&)),
				  actionCollection());
  KConfig *kc = m_settings->getKConfig();
  recent->loadEntries(kc);

  (void)KStdAction::keyBindings();

  shellOutputAction  = new KAction(i18n("&View shell output"), 0, this,
				   SLOT(edit_view_last_shell_output()),
				   actionCollection(), "shell_output");

  KStdAction::quit(this, SLOT(window_close()), actionCollection());

  addFileAction = new KAction(i18n("&Add File..."), "ark_addfile", 0, this,
				SLOT(action_add()),
				actionCollection(), "addfile");

  addDirAction = new KAction(i18n("Add Di&r..."), "ark_adddir", 0, this,
				SLOT(action_add_dir()),
				actionCollection(), "adddir");

  extractAction = new KAction(i18n("&Extract..."), "ark_extract", 0, this,
				SLOT(action_extract()),
				actionCollection(), "extract");

  deleteAction = new KAction(i18n("&Delete"), "ark_delete", 0, this,
			     SLOT(action_delete()),
			     actionCollection(), "delete");

  selectAllAction = new KAction(i18n("Select &All"), "ark_selectall", 0, this,
			     SLOT(edit_selectAll()),
			     actionCollection(), "select_all");

  viewAction = new KAction(i18n("&View"), "ark_view", 0, this,
			   SLOT(action_view()),
			   actionCollection(), "view");

  popupViewAction  = new KAction(i18n("&View"), "ark_view", 0, this,
			   SLOT(action_view()),
			   actionCollection(), "popup_menu_view");

  openWithAction = new KAction(i18n("&Open with..."), 0, this,
			   SLOT(slotOpenWith()),
			   actionCollection(), "open_with");

  popupOpenWithAction  = new KAction(i18n("&Open with..."), 0, this,
			   SLOT(slotOpenWith()),
			   actionCollection(), "popup_menu_open_with");


  editAction = new KAction(i18n("Edit &with..."), 0, this,
			   SLOT(action_edit()),
			   actionCollection(), "edit");

  popupEditAction = new KAction(i18n("&Edit with..."), 0, this,
				SLOT(action_edit()),
				actionCollection(), "popup_edit");

  settingsAction =  new KAction(i18n("&Settings"), "ark_options", 0, this,
			   SLOT(options_dirs()),
			   actionCollection(), "settings");
 
  selectAction =  new KAction(i18n("&Select..."), 0, this,
			     SLOT(edit_select()),
			     actionCollection(), "select");
  deselectAllAction =  new KAction(i18n("&Deselect All"), 0, this,
			     SLOT(edit_deselectAll()),
			     actionCollection(), "deselect_all");

  invertSelectionAction  =  new KAction(i18n("&Invert Selection"), 0, this,
					SLOT(edit_invertSel()),
					actionCollection(),
					"invert_selection");

  (void)new KAction(i18n("&Directories..."), 0, this,
		    SLOT(options_dirs()),
		    actionCollection(),
		    "directories");

  KStdAction::showToolbar(this, SLOT(toggleToolBar()), actionCollection());
  KStdAction::showStatusbar(this, SLOT(toggleStatusBar()), actionCollection());
  KStdAction::saveOptions(this, SLOT(options_saveNow()), actionCollection());
  KStdAction::keyBindings(this, SLOT(options_keys()), actionCollection());


  createGUI();
}

void ArkWidget::setHeader()
{
  // called after the QTimer goes off after toggling menu bar to hide it.
  if (m_bIsArchiveOpen)
    setCaption(m_strArchName);
  else
    setCaption(QString::null);
}

void ArkWidget::toggleToolBar()
{
 if(toolBar()->isVisible())
   toolBar()->hide();
 else
   toolBar()->show();

}

void ArkWidget::toggleStatusBar()
{
  if (statusBar()->isVisible())
    statusBar()->hide();
  else
    statusBar()->show();
}

void ArkWidget::setupStatusBar()
{
  kdDebug(1601) << "+ArkWidget::setupStatusBar" << endl;

  KStatusBar *sb = statusBar();

  QWhatsThis::add(sb, i18n("The statusbar shows you how many files you have and how many you have selected. It also shows you total sizes for these groups of files."));

  m_pStatusLabelSelect = new QLabel(sb);
  m_pStatusLabelSelect->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  m_pStatusLabelSelect->setFixedHeight(30);
  m_pStatusLabelSelect->setMargin(0);
  m_pStatusLabelSelect->setAlignment(AlignCenter);
  m_pStatusLabelSelect->setText(i18n("0 Files Selected"));

  m_pStatusLabelTotal = new QLabel(sb);
  m_pStatusLabelTotal->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  m_pStatusLabelTotal->setFixedHeight(30);
  m_pStatusLabelTotal->setMargin(0);
  m_pStatusLabelTotal->setAlignment(AlignCenter);
  m_pStatusLabelTotal->setText(i18n("Total: 0 Files"));


  sb->setMinimumHeight(40);

  sb->addWidget(m_pStatusLabelSelect, 3000);
  sb->addWidget(m_pStatusLabelTotal, 3000);

  kdDebug(1601) << "-ArkWidget::setupStatusBar" << endl;
}

void ArkWidget::initialEnables()
{
  // start out with some menu items disabled
  closeAction->setEnabled(false);
  saveAsAction->setEnabled(false);
  reloadAction->setEnabled(false);

  selectAction->setEnabled(false);
  selectAllAction->setEnabled(false);
  deselectAllAction->setEnabled(false);
  invertSelectionAction->setEnabled(false);

  viewAction->setEnabled(false);

  // always true - you have to have right-clicked on a file to see the
  // menu with these actions
  popupViewAction->setEnabled(true);
  popupOpenWithAction->setEnabled(true);
  popupEditAction->setEnabled(true);
  recent->setEnabled(true);

  deleteAction->setEnabled(false);
  extractAction->setEnabled(false);
  addFileAction->setEnabled(false);
  addDirAction->setEnabled(false);

}


////////////////////////////////////////////////////////////////////
///////////////////////// updateStatusTotals ///////////////////////
////////////////////////////////////////////////////////////////////

void ArkWidget::updateStatusTotals()
{
  m_nNumFiles = 0;
  m_nSizeOfFiles = 0;
  if (archiveContent)
    {
      FileLVI *pItem = (FileLVI *)archiveContent->firstChild();
      while (pItem)
	{
	  ++m_nNumFiles;
	  
	  if (m_currentSizeColumn != -1)
	    m_nSizeOfFiles += atoi(pItem->text(m_currentSizeColumn));
	  pItem = (FileLVI *)pItem->nextSibling();
	}
    }
  //  kdDebug(1601) << "We have " << m_nNumFiles << " elements\n" << endl;

  QString strInfo;

  if (m_nNumFiles == 0)
    strInfo = i18n("Total: 0 Files");
  else if (m_nNumFiles == 1)
    strInfo = i18n("Total: 1 File %1 KB")
      .arg(KGlobal::locale()->formatNumber(m_nSizeOfFiles, 0));
  else
    strInfo = i18n("Total: %1 Files %1 KB")
      .arg(KGlobal::locale()->formatNumber(m_nNumFiles, 0))
      .arg(KGlobal::locale()->formatNumber(m_nSizeOfFiles, 0));
  
  m_pStatusLabelTotal->setText(strInfo);
}

//////////////////////////////////////////////////////////////////////
////////////////////// file_save_as //////////////////////////////////
//////////////////////////////////////////////////////////////////////

void ArkWidget::file_save_as()
{
  // we have to make sure the user doesn't think this is
  // an opportunity to convert .tgz to .zip...

  QString strFile;
  QString extension, filter;
  enum ArchType archtype = getArchType(m_strArchName, extension);
  
  filter = "*" + extension;
  KURL u;
  do
    {
      u = getCreateFilename(i18n("Save Archive As"), filter, extension);
      if (u.isEmpty())
	return;      
      QString ext;
      strFile = u.path();
      ArchType newArchType = getArchType(strFile, ext);
      if (newArchType == archtype)
	break;
      // these types don't mind having no extension. Zip will add one, ulp!
      if (newArchType == UNKNOWN_FORMAT && !strFile.contains('.')
	  && (archtype == RAR_FORMAT || archtype == LHA_FORMAT ||
	      archtype == AA_FORMAT))
	break;
      KMessageBox::error(this, i18n("Please save your archive in the same format as the original.\nHint: Use the same extension."));
    }
  while (true);

  KURL src = m_strArchName;
  mSaveAsURL = u;
  KIO::Job * job = KIO::copy(src, u);
  connect( job, SIGNAL( result( KIO::Job * ) ), this,
	   SLOT( slotSaveAsDone( KIO::Job * ) ) );
}

void ArkWidget::slotSaveAsDone(KIO::Job * job)
{
  if (job->error())
    job->showErrorDialog();
  else
    file_open(mSaveAsURL.path());
}


//////////////////////////////////////////////////////////////////////
///////////////////////// file_open //////////////////////////////////
//////////////////////////////////////////////////////////////////////


void ArkWidget::file_open(const QString & strFile)
{
  kdDebug(1601) << "+ArkWidget::file_open(const QString & strFile)" << endl;
  //strFile has no protocol by now - it's a path.
  struct stat statbuffer;
  
  if (stat(strFile.local8Bit(), &statbuffer) == -1)
    {
      if (errno == ENOENT || errno == ENOTDIR || errno ==  EFAULT)
	{
	  KMessageBox::error(this, i18n("The archive %1 does not exist.").arg(strFile.local8Bit()));
	}
      else if (errno == EACCES)
	{
	  KMessageBox::error(this, i18n("Can't access the archive %1").arg(strFile.local8Bit()));
	  recent->removeURL(strFile);
	}
      else
	KMessageBox::error(this, i18n("Unknown error."));
      recent->removeURL(strFile);
      return;
    }
  else
    {
      // this will be the appropriate flag depending on whose file it is
      unsigned int nFlag = 0;
      if (geteuid() == statbuffer.st_uid)
	{
	  nFlag = S_IRUSR; // it's mine
	}
      else if (getegid() == statbuffer.st_gid)
	{
	  nFlag = S_IRGRP; // it's my group's
	}
      else
	{
	  nFlag = S_IROTH;  // it's someone else's
	}
    
      if (! ((statbuffer.st_mode & nFlag) == nFlag))
	{
	  KMessageBox::error(this, i18n("You don't have permission to access that archive") );
	  recent->removeURL(strFile);
	  return;
	}
    }

  // see if the user is just opening the same file that's already
  // open (erm...)

  if (strFile == m_strArchName && m_bIsArchiveOpen)
    {
      return;
    }

    // see if the ark is already open in another window
  if (ArkApplication::getInstance()->isArkOpenAlready(strFile))
    {
      // close this window
      window_close();

	// raise the window containing the already open archive
      ArkApplication::getInstance()->raiseArk(strFile);

      // notify the user what's going on
      KMessageBox::information(0, i18n("The archive %1 is already open and has been raised.\nNote: if the filename does not match, it only means that one of the two is a symbolic link.").arg((const char *)strFile));
      return;
    }

  // no errors if we made it this far.

  if (isArchiveOpen())
    file_close();  // close old zip

    // Set the current archive filename to the filename
  m_strArchName = strFile;

  // add it to the application-wide list of open archives
  ArkApplication::getInstance()->addOpenArk(strFile, this);

  // display the archive contents
  showZip(strFile);
  kdDebug(1601) << "-ArkWidget::file_open(const QString & strFile)" << endl;
}

//////////////////////////////////////////////////////////////////////
////////////////////////////// download //////////////////////////////
//////////////////////////////////////////////////////////////////////


void ArkWidget::download(const KURL &url, QString &strFile)
{
  // downloads url into strFile, making sure strFile has the same extension
  // as url.
  if (!url.isLocalFile())
    {
      QString extension;
      getArchType(url.path(), extension);
      mpTempFile = new KTempFile("/tmp/ark", extension);
      strFile = mpTempFile->name();
      kdDebug(1601) << "Downloading " << url.path().local8Bit() << " as " <<
	strFile.local8Bit() << endl;
    }
  KIO::NetAccess::download(url, strFile);
}



//////////////////////////////////////////////////////////////////////
//
// ArkWidget slots
//
//////////////////////////////////////////////////////////////////////


void ArkWidget::saveProperties()
{
  kdDebug(1601) << "+saveProperties (exit)" << endl;

  KConfig *kc = m_settings->getKConfig();
  recent->saveEntries(kc);

#if 0
  if ( m_settings->isSaveOnExitChecked() )
    accelerators->writeSettings( m_settings->getKConfig() );
#endif

  m_settings->writeConfiguration();

  kdDebug(1601) << "-saveProperties (exit)" << endl;
}

int ArkWidget::getCol(const QString & _columnHeader)
{
  // return the column corresponding to the header, or -1 for failure
  int column;
  for (column = 0; column < archiveContent->header()->count();
       ++column)
    {
      if (archiveContent->columnText(column) == _columnHeader)
	{
	  return column;
	}
    }
  
  kdError(1601) << "Can't find header " << _columnHeader << endl;
  return -1;
}
QString ArkWidget::getColData(const QString &_filename,
			      int _col)
{
  FileListView *flw = fileList();
  FileLVI *flvi = (FileLVI*)flw->firstChild();
  while (flvi)
    {
      QString curFilename = flvi->text(0);
      if (curFilename == _filename)
	return (flvi->text(_col));
      flvi = (FileLVI*)flvi->itemBelow();
    }
  kdError(1601) << "Couldn't find " << _filename << " in ArkWidget::getColData"
		<< endl;

  return QString(QString::null);
}


// File menu /////////////////////////////////////////////////////////

KURL ArkWidget::getCreateFilename(const QString & _caption,
				  const QString & _filter,
				  const QString & _extension)
{
  int choice=0;
  struct stat statbuffer;
  bool skip = false;
  QString strFile;
  KURL url;

  while (true)
    // keep asking for filenames as long as the user doesn't want to 
    // overwrite existing ones; break if they agree to overwrite
    // or if the file doesn't already exist. Return if they cancel.
    // Also check for proper extensions.
    {
      if (!skip)
	{
	  url = KFileDialog::getSaveURL(QString::null,
					     _filter,
					     0, _caption);
	  strFile = url.path();
	}
      skip = false;
      if (strFile.isEmpty())
	return QString::null;
      
      kdDebug(1601) << "Trying to create an archive named " <<
	strFile.local8Bit() << endl;
      if (stat(strFile.local8Bit(), &statbuffer) != -1)  // already exists!
	{
	  choice =
	    KMessageBox::warningYesNoCancel(0, i18n("Archive already exists. Do you wish to overwrite it?"), i18n("Archive already exists"));
	  if (choice == KMessageBox::Yes)
	    {
	      unlink(strFile);
	      break;
	    }
	  else if (choice == KMessageBox::Cancel)
	    {
	      return QString::null;
	    }
	  else
	    {
	      continue;
	    }
	}
      // if we got here, the file does not already exist.
      if (!Utilities::haveDirPermissions(strFile))
	return QString::null;
      
      QString temp;
      if (m_bMakeCFIntoArchiveInProgress &&
	  getArchType(strFile, temp) == COMPRESSED_FORMAT)
	{
	  KMessageBox::information(0, i18n("Sorry, you need to create an archive, not a new\ncompressed file. Please try again."));
	  continue;	       
	}
      
      // if we made it here, it's a go.
      if (m_strArchName.contains('.') && !strFile.contains('.'))
	{
	  // if the filename has no dot in it, ask to append extension
	  QString extension = _extension;
	  if (extension.isNull())
	    extension = ".zip";
	  
	  int nRet = KMessageBox::warningYesNo(0, i18n("Your file is missing an extension to indicate the archive type.\nIs it OK to create a file of the default type (%1)?").arg(extension), i18n("Error"));
	  if (nRet == KMessageBox::Yes)
	    {
	      strFile += extension;
	      url = strFile;
	      skip = true; // skip the getSaveUrl part
	      continue; // gotta check if it exists again
	    }
	  else // no? well choose a different filename then.
	    {
	      continue;
	    }
	}
      else
	break; 
    } // end of while loop
  return url;
}

void ArkWidget::file_new()
{
  QString strFile;
  KURL url = getCreateFilename(i18n("Create a New Archive"),
			       m_settings->getFilter());
  strFile = url.path();
  m_settings->clearShellOutput();
  if (!strFile.isEmpty())
    createArchive( strFile );
}

void ArkWidget::slotCreate(Arch * _newarch, bool _success,
			   const QString & _filename, int)
{
  if ( _success )
    {
      file_close();
      m_strArchName = _filename;
      setCaption( _filename );
      createFileListView();
      setCaption(_filename);
      m_bIsArchiveOpen = true;
      arch = _newarch;
      QString extension;
      m_bIsSimpleCompressedFile =
	(getArchType(m_strArchName, extension) == COMPRESSED_FORMAT);
      fixEnables();
      if (m_bMakeCFIntoArchiveInProgress)
	{
	  QStringList listForCompressedFile;
	  listForCompressedFile.append(m_compressedFile);
	  addFile(&listForCompressedFile);
	}
      QApplication::restoreOverrideCursor();
    }
  else
    {
      QApplication::restoreOverrideCursor();
      KMessageBox::error(this, i18n("Sorry, ark cannot create an archive of that type.\n\n  [Hint:  The filename should have an extension such as `.zip' to\n  indicate the type of the archive. Please see the help pages for\nmore information on supported archive formats.]"));
    }
}

void ArkWidget::file_newWindow()
{
  kdDebug(1601) << "-ArkWidget::file_newWindow" << endl;
  
  ArkWidget *kw = new ArkWidget;
  kw->show();
  kdDebug(1601) << "-ArkWidget::file_newWindow" << endl;

}

void ArkWidget::file_open()
{
  kdDebug(1601) << "+ArkWidget::file_open" << endl;

  KURL url;
  QString strFile;
  url = KFileDialog::getOpenURL(m_settings->getOpenDir(),
				m_settings->getFilter(), this);
  qApp->processEvents();

  if (!url.isEmpty())
    {
      download(url, strFile);
      m_settings->clearShellOutput();
      recent->addURL(url);
      file_open(strFile);
    }
  kdDebug(1601) << "-ArkWidget::file_open" << endl;
}

void ArkWidget::file_open(const KURL& url)
{
  kdDebug(1601) << "+ArkWidget::file_open(const KURL& url)" << endl;
  QString strFile;
  if (!url.isEmpty())
  {
    if (isArchiveOpen())
      file_close();  // close old arch. If we don't, our temp file is wrong!
    download(url, strFile);
    m_settings->clearShellOutput();
    kdDebug(1601) << "Recent open: " << strFile.local8Bit() << endl;
    file_open(strFile);
  }
  kdDebug(1601) << "-ArkWidget::file_open(const KURL& url)" << endl;
}

void ArkWidget::showZip( QString _filename )
{
  kdDebug(1601) << "+ArkWidget::showZip" << endl;
  createFileListView();
  openArchive( _filename );
  kdDebug(1601) << "-ArkWidget::showZip" << endl;
}

void ArkWidget::slotOpen(Arch *_newarch, bool _success,
			 const QString & _filename, int )
{
  kdDebug(1601) << "+ArkWidget::slotOpen" << endl;
  
  archiveContent->setUpdatesEnabled(true);
  archiveContent->triggerUpdate();
  
  if ( _success )
    {
	QFileInfo fi( _filename );
	QString path = fi.dirPath( true );
	m_settings->setLastOpenDir( path );

	if (_filename.left(9) == QString("/tmp/ark.") ||
	    !fi.isWritable())
	  {
	    _newarch->setReadOnly(true);
            QApplication::restoreOverrideCursor(); // no wait cursor during a msg box (David)
	    KMessageBox::information(this, i18n("This archive is read-only. If you want to save it under\na new name, go to the File menu and select Save As."));
            QApplication::setOverrideCursor( waitCursor );
	  }
	setCaption( _filename );
	//	createActionMenu( _flag );
	arch = _newarch;
	updateStatusTotals();
	m_bIsArchiveOpen = true;
	QString extension;
	m_bIsSimpleCompressedFile =
	  (getArchType(m_strArchName, extension) == COMPRESSED_FORMAT);
    }
  fixEnables();
  QApplication::restoreOverrideCursor();
  kdDebug(1601) << "-ArkWidget::slotOpen" << endl;
}

void ArkWidget::slotDeleteDone(bool _bSuccess)
{
  kdDebug(1601) << "+ArkWidget::slotDeleteDone------------------------------" << endl;
  archiveContent->setUpdatesEnabled(true);
  archiveContent->triggerUpdate();
  if (_bSuccess)
    {
      updateStatusTotals();
      updateStatusSelection();	
    }
  // disable the select all and extract options if there are no files left
  fixEnables();
  QApplication::restoreOverrideCursor();
  kdDebug(1601) << "-ArkWidget::slotDeleteDone" << endl;
}

void ArkWidget::slotExtractDone()
{
  kdDebug(1601) << "+ArkWidget::slotExtractDone" << endl;
  QApplication::restoreOverrideCursor();
  // I don't need this any more??
  //  if ( !KOpenWithHandler::exists() )
  //    (void) new KFileOpenWithHandler();
  if (m_bViewInProgress)
    {
      m_bViewInProgress = false;
      if (m_bEditInProgress)
	{
	  kdDebug(1601) << "Edit in progress..." << endl;
	  KURL::List list;
	  // edit will be in progress until the KProcess terminates.
	  KOpenWithDlg l( list, i18n("Edit With:"), QString::null,
			  (QWidget*)0L);
	  if ( l.exec() )
	    {
	      KProcess *kp = new KProcess;
	      m_strFileToView =
		m_strFileToView.right(m_strFileToView.length() - 5);
	      *kp << l.text() << m_strFileToView;
	      connect(kp, SIGNAL(processExited(KProcess *)),
		      this, SLOT(slotEditFinished(KProcess *)));
	      if (kp->start(KProcess::NotifyOnExit,
			    KProcess::AllOutput) == false)
		{
		  KMessageBox::error(0, i18n("Trouble editing the file..."));
		}
	    }
	}
      else
	{
	  m_pKRunPtr = new KRun (m_strFileToView);
	}
    }
  else if (m_bOpenWithInProgress)
    {
      m_bOpenWithInProgress = false;
      KURL::List list;
      KURL url = m_strFileToView;
      list.append(url);
      KOpenWithDlg l( list, i18n("Open With:"), QString::null, (QWidget*)0L);
      if ( l.exec() )
	{
	  KService::Ptr service = l.service();
	  if ( !!service )
	    {
	      KRun::run( *service, list );
	    }
	  else
	    {
	      QString exec = l.text();
	      exec += " %f";
	      KRun::run( exec, list );
	    }
	}
    }
  else if (m_bDragInProgress)
    {
      m_bDragInProgress = false;
      QStrList list;
      for (QStringList::Iterator it = mDragFiles.begin();
	   it != mDragFiles.end(); ++it)
	{      
	  QString URL;
	  URL.sprintf("/tmp/ark.%d/", getpid());
	  URL += *it;
	  URL = QUriDrag::localFileToUri(URL);
	  list.append(URL);
	}
      QUriDrag *d = new QUriDrag(list, archiveContent->viewport());
      //      d->setPixmap(QPixmap(QString("document.xpm")),
      //		   QPoint(0,0));
      m_bDropSourceIsSelf = true;
      d->dragCopy();
      m_bDropSourceIsSelf = false;
    }
  delete m_extractList;
  m_extractList = NULL;
  archiveContent->setUpdatesEnabled(true);
  fixEnables();  
  kdDebug(1601) << "-ArkWidget::slotExtractDone" << endl;
}

void ArkWidget::slotEditFinished(KProcess *kp)
{
  kdDebug(1601) << "+ArkWidget::slotEditFinished" << endl;
  delete kp;
  QStringList list;
  // now put the file back into the archive.
  list.append(m_strFileToView);
  addFile(&list);
  kdDebug(1601) << "-ArkWidget::slotEditFinished" << endl;
}

void ArkWidget::slotAddDone(bool _bSuccess)
{
  kdDebug(1601) << "+ArkWidget::slotAddDone" << endl;
  archiveContent->setUpdatesEnabled(true);
  archiveContent->triggerUpdate();
  delete mpAddList;
  mpAddList = NULL;

  if (_bSuccess)
    {
      file_reload();
      if (m_bDropFilesInProgress)
	{
	  m_bDropFilesInProgress = false;
	  delete m_pTempAddList;
	  m_pTempAddList = NULL;
	}
      if (m_bEditInProgress)
	{
	  m_bEditInProgress = false;
	}
      if (m_bMakeCFIntoArchiveInProgress)
	{
	  m_bMakeCFIntoArchiveInProgress = false;
	  QApplication::restoreOverrideCursor();
	  if (m_pTempAddList == NULL)
	    {
	      // now get the files to be added
	      action_add();
	    }
	  else
	    {
	      // files were dropped in; add those files now.
	      m_bDropFilesInProgress = true;
	      addFile(m_pTempAddList);
	    }
	  return;
	}
    }
  if (mpDownloadedList)
    {
      for (QStringList::Iterator it = mpDownloadedList->begin();
	   it != mpDownloadedList->end(); ++it)
	{
	  QString str = *it;

	  // is this necessary? Maybe risky. The tmp/ark.### directory
	  // will be removed anyhow...
	  str = str.right(str.length() - 5);
	  unlink(str.local8Bit());
	}
      delete mpDownloadedList;
      mpDownloadedList = NULL;
    }
  fixEnables();
  QApplication::restoreOverrideCursor();
  kdDebug(1601) << "-ArkWidget::slotAddDone" << endl;
}

//////////////////////////////////////////////////////////////////////
/////////////////////////// disableAll ///////////////////////////////
//////////////////////////////////////////////////////////////////////

void ArkWidget::disableAll() // private
{
  kdDebug(1601) << "+ArkWidget::disableAll" << endl;

  openAction->setEnabled(false);
  newArchAction->setEnabled(false);
  closeAction->setEnabled(false);
  saveAsAction->setEnabled(false);
  reloadAction->setEnabled(false);

  selectAction->setEnabled(false);
  selectAllAction->setEnabled(false);
  deselectAllAction->setEnabled(false);
  invertSelectionAction->setEnabled(false);

  viewAction->setEnabled(false);
  deleteAction->setEnabled(false);
  extractAction->setEnabled(false);
  addFileAction->setEnabled(false);
  addDirAction->setEnabled(false);
  openWithAction->setEnabled(false);
  editAction->setEnabled(false);

  popupViewAction->setEnabled(false);
  popupOpenWithAction->setEnabled(false);
  popupEditAction->setEnabled(false);
  
  kdDebug(1601) << "-ArkWidget::disableAll" << endl;
}

//////////////////////////////////////////////////////////////////////
/////////////////////////// fixEnables ///////////////////////////////
//////////////////////////////////////////////////////////////////////

void ArkWidget::fixEnables() // private
{
  bool bHaveFiles = (m_nNumFiles > 0);
  bool bReadOnly = false;
  bool bAddDirSupported = true;
  QString extension;
  enum ArchType archtype = getArchType(m_strArchName, extension);
  if (archtype == ZOO_FORMAT || archtype == AA_FORMAT)
    bAddDirSupported = false;

  if (arch)
    bReadOnly = arch->isReadOnly();

  // always true - you have to have right-clicked on a file to see the
  // menu with these actions
  popupViewAction->setEnabled(true);
  popupOpenWithAction->setEnabled(true);
  popupEditAction->setEnabled(true);


  openAction->setEnabled(true);
  newArchAction->setEnabled(true);
  closeAction->setEnabled(bHaveFiles);
  saveAsAction->setEnabled(bHaveFiles);
  reloadAction->setEnabled(bHaveFiles);

  selectAction->setEnabled(bHaveFiles);
  selectAllAction->setEnabled(bHaveFiles);
  deselectAllAction->setEnabled(bHaveFiles);
  invertSelectionAction->setEnabled(bHaveFiles);

  deleteAction->setEnabled(bHaveFiles  && m_nNumSelectedFiles > 0
			   && arch && !bReadOnly);
  addFileAction->setEnabled(m_bIsArchiveOpen &&
			    !bReadOnly);
  addDirAction->setEnabled(m_bIsArchiveOpen &&
			   !bReadOnly && bAddDirSupported);
  extractAction->setEnabled(bHaveFiles);
  viewAction->setEnabled(bHaveFiles && m_nNumSelectedFiles == 1);
  openWithAction->setEnabled(bHaveFiles && m_nNumSelectedFiles == 1);
  editAction->setEnabled(bHaveFiles && m_nNumSelectedFiles == 1);
}


void ArkWidget::file_reload()
{
  if (isArchiveOpen())
    {
      QString filename = arch->fileName();
      file_close();
      file_open(filename);
    }
}

void ArkWidget::reload()
{
  if (isArchiveOpen())
    {
      file_reload();
    }
}
	
void ArkWidget::file_close()
{
  kdDebug(1601) << "+ArkWidget::file_close" << endl;
  if (isArchiveOpen())
    {
      delete arch;
      arch = 0;
      setCaption(QString::null);
      m_bIsArchiveOpen = false;
      
      if (archiveContent)
	{
	  archiveContent->clear();
	}
      setView(0);
      ArkApplication::getInstance()->removeOpenArk(m_strArchName);
      if (mpTempFile)
	{      
	  kdDebug(1601) << "Removing temp file " <<
	    mpTempFile->name().local8Bit() << endl;
	  mpTempFile->unlink();
	  delete mpTempFile;
	  mpTempFile = NULL;
	}
      updateStatusTotals();
      updateStatusSelection();
      fixEnables();
    }
  kdDebug(1601) << "-ArkWidget::file_close" << endl;
}

void ArkWidget::window_close()
{
    kdDebug(1601) << "+ArkWidget::window_close" << endl;

    file_close();
    if (ArkApplication::getInstance()->windowCount() < 2  )
      {
	saveProperties();
	delete this;  // hack
	kapp->quit();
      }
    else
      {
	saveProperties();
	delete this;
      }
    kdDebug(1601) << "-ArkWidget::window_close" << endl;
}


void ArkWidget::closeEvent( QCloseEvent *e )
{
    KTMainWindow::closeEvent(e);
    window_close();
}


void ArkWidget::file_quit()
{
  window_close();
}

// Edit menu /////////////////////////////////////////////////////////

void ArkWidget::edit_select()
{
  SelectDlg *sd = new SelectDlg( m_settings, this );
  if ( sd->exec() )
    {
      QString exp = sd->getRegExp();
      m_settings->setSelectRegExp( exp );

      QRegExp reg_exp( exp, true, true );
      if (!reg_exp.isValid())
	kdError(1601) <<
	  "ArkWidget::edit_select: regular expression is not valid." << endl;
      else
	{
	  // first deselect everything
	  archiveContent->clearSelection();
	  FileLVI * flvi = (FileLVI*)archiveContent->firstChild();
	  

	  // don't call the slot for each selection!
	  disconnect( archiveContent, SIGNAL( selectionChanged()),
		      this, SLOT( slotSelectionChanged() ) );
    
	  while (flvi)
	    {
	      if ( reg_exp.match(flvi->text(0))==0 )
		{
		  archiveContent->setSelected(flvi, true);
		}
	      flvi = (FileLVI*)flvi->itemBelow();
	    }
	  // restore the behavior
	  connect( archiveContent, SIGNAL( selectionChanged()),
		   this, SLOT( slotSelectionChanged() ) );
	  updateStatusSelection();
	}
    }
}

void ArkWidget::edit_selectAll()
{
  FileLVI * flvi = (FileLVI*)archiveContent->firstChild();

  // don't call the slot for each selection!
  disconnect( archiveContent, SIGNAL( selectionChanged()),
	   this, SLOT( slotSelectionChanged() ) );
  while (flvi)
    {
      archiveContent->setSelected(flvi, true);
      flvi = (FileLVI*)flvi->itemBelow();
    }

  // restore the behavior
  connect( archiveContent, SIGNAL( selectionChanged()),
	   this, SLOT( slotSelectionChanged() ) );
  updateStatusSelection();	
}

void ArkWidget::edit_deselectAll()
{
  archiveContent->clearSelection();
  updateStatusSelection();	
}

void ArkWidget::edit_invertSel()
{
  FileLVI * flvi = (FileLVI*)archiveContent->firstChild();
  // don't call the slot for each selection!
  disconnect( archiveContent, SIGNAL( selectionChanged()),
	   this, SLOT( slotSelectionChanged() ) );

  while (flvi)
    {
      archiveContent->setSelected(flvi, !flvi->isSelected());
      flvi = (FileLVI*)flvi->itemBelow();
    }
  // restore the behavior
  connect( archiveContent, SIGNAL( selectionChanged()),
	   this, SLOT( slotSelectionChanged() ) );
  updateStatusSelection();	
}

void ArkWidget::edit_view_last_shell_output()
{
  ShellOutputDlg* sod = new ShellOutputDlg( m_settings, this );
  sod->exec();
}

KURL ArkWidget::askToCreateRealArchive()
{
  // ask user whether to create a real archive from a compressed file
  // returns filename if so
  KURL url;
  int choice =
    KMessageBox::warningYesNo(0, i18n("You are currently working with a simple compressed file.\nWould you like to make it into an archive so that it can contain multiple files?\nIf so, you must choose a name for your new archive."), i18n("Warning"));
  if (choice == KMessageBox::Yes)
    {
      m_bMakeCFIntoArchiveInProgress = true;
      url = getCreateFilename(i18n("Create a New Archive"),
			      m_settings->getFilter());
    }
  return url;
}

void ArkWidget::createRealArchive(const QString &strFilename)
{
  FileListView *flw = fileList();  
  FileLVI *flvi = (FileLVI*)flw->firstChild();
  m_compressedFile = flvi->getFileName().local8Bit();
  QString tmpdir = m_settings->getTmpDir();
  m_compressedFile = "file:" + tmpdir + "/" + m_compressedFile;
  kdDebug(1601) << "The compressed file is " << (const char *)m_compressedFile << endl;
  createArchive(strFilename);
  // the file will be moved into the new archive in slotCreate.
}

// Action menu /////////////////////////////////////////////////////////

void ArkWidget::action_add()
{
  QString extension;
  ArchType archtype = getArchType(m_strArchName, extension);
  if (m_bIsSimpleCompressedFile && (m_nNumFiles == 1))
    {
      QString strFilename;
      KURL url = askToCreateRealArchive();
      strFilename = url.path();
      if (!strFilename.isEmpty())
	{
	  createRealArchive(strFilename);
	}
      return;
    }
  kdDebug(1601) << "Add dir: " << (const char *)m_settings->getAddDir() << endl;
  AddDlg *dlg = new AddDlg(archtype, m_settings->getAddDir(),
			   m_settings, this, "adddlg");
  if (dlg->exec())
    {
      // we're making our own so we can delete it later.
      QStringList *temp = dlg->getFiles();
      mpAddList = new QStringList(*temp);

      if (mpAddList->count() > 0)
	{
	  if (m_bIsSimpleCompressedFile && mpAddList->count() > 1)
	    {
	      QString strFilename;
	      KURL url = askToCreateRealArchive();
	      strFilename = url.path();
	      if (!strFilename.isEmpty())
		{
		  createRealArchive(strFilename);
		}
	      return;
	    }
	  addFile(mpAddList);
	}
    }
}

void ArkWidget::addFile(QStringList *list)
{
  // takes a list of KURLs.
  archiveContent->setUpdatesEnabled(false);
  disableAll();
  QApplication::setOverrideCursor( waitCursor );
  if (m_bEditInProgress)
    {
      // there's only one file, and it's in the temp directory.
      // If the filename has more than three /'s then we should
      // change to the first level directory so that the paths
      // come out right.
      QStringList::Iterator it = list->begin();
      QString filename = *it;
      QString path;
      if (filename.contains('/') > 3)
	{
	  kdDebug(1601) << "Filename is originally: " << filename << endl;
	  int i = filename.find('/', 5);
	  path = filename.left(1+i);
	  kdDebug(1601) << "Changing to dir: " << path << endl;
	  chdir( QFile::encodeName(path) );
	  filename = filename.right(filename.length()-i-1);
	  // HACK!! We need a relative path. If I have "file:", it
	  // will look like an absolute path. So five spaces here to get
	  // chopped off later....
	  filename = "     " + filename;
	  *it = filename;
	}
    }
  else
    {
      bool bNotLocal = true;
      // if they are URLs, we have to download them, replace the URLs
      // with filenames, and remember to delete the temporaries later.
      for (QStringList::Iterator it = list->begin(); it != list->end(); ++it)
	{
	  QString str = *it;
	  kdDebug(1601) << "Want to add " << str.local8Bit() << endl;
	  if (str.left(5) == QString("file:"))
	    {
	      bNotLocal = false;
	      break;  // no need to continue
	    }
	  KURL url = str;
	  QString tempfile = m_settings->getTmpDir();
	  tempfile += str.right(str.length() - str.findRev("/") - 1);
	  KIO::NetAccess::download(url, tempfile);
	  tempfile = "file:" + tempfile;
	  // replace the URL with the name of the temporary
	  *it = tempfile;
	}
      if (bNotLocal)
	mpDownloadedList = new QStringList(*list);
    }
  arch->addFile(list);
}

void ArkWidget::action_add_dir()
{
  QString dirName
    = KFileDialog::getExistingDirectory(m_settings->getAddDir(), 0,
					i18n("Select a Directory to Add"));
  if (!dirName.isEmpty())
    {
      // fix protocol
      dirName = "file:" + dirName;
      archiveContent->setUpdatesEnabled(false);
      QApplication::setOverrideCursor( waitCursor );
      disableAll();
      arch->addDir(dirName);
    }
}

void ArkWidget::action_delete()
{
  // remove selected files and create a list to send to the archive
  // Warn the user if he/she/it tries to delete a directory entry in
  // a tar file - it actually deletes the contents of the directory
  // as well.

  kdDebug(1601) << "+ArkWidget::action_delete" << endl;
  
  if (archiveContent->isSelectionEmpty())
    return; // shouldn't happen - delete should have been disabled!

  QString extension;
  bool bIsTar = getArchType(m_strArchName, extension) == TAR_FORMAT;
  bool bDeletingDir = false;
  QStringList list;
  FileLVI* flvi = (FileLVI*)archiveContent->firstChild();
  FileLVI* old_flvi;
  QStringList dirs;

  if (bIsTar)
    {
      // check if they're deleting a directory
      while (flvi)
	{
	  if ( archiveContent->isSelected(flvi) )
	    {
	      old_flvi = flvi;
	      flvi = (FileLVI*)flvi->itemBelow();
	      QString strFile = old_flvi->getFileName().copy();
	      list.append(strFile);
	      QString strTemp = old_flvi->text(1);
	      if (strTemp.left(1) == "d")
		{
		  bDeletingDir = true;
		  dirs.append(strFile);
		}
	    }		
	  else flvi = (FileLVI*)flvi->itemBelow();
	}
      if (bDeletingDir)
	{
	  int nRet = KMessageBox::warningContinueCancel(this, i18n("If you delete a directory in a Tar archive, all the files in that\ndirectory will also be deleted. Are you sure you wish to proceed?"), i18n("Warning"), i18n("Continue"));
	  if (nRet == KMessageBox::Cancel)
	    return;
	}
    }

  bool bDoDelete = true;
  if (!bDeletingDir)
    {
      // ask for confirmation
      bDoDelete = KMessageBox::questionYesNo(this, i18n("Do you really want to delete the selected items?")) == KMessageBox::Yes;
    }
  if (!bDoDelete)
    return;

  // reset to the beginning to do the second sweep
  flvi = (FileLVI*)archiveContent->firstChild();
  while (flvi)
    {
      // if it's selected or, if it's a tar and we're deleting a directory
      // and the file is in that directory, delete the listview item.
      old_flvi = flvi;
      flvi = (FileLVI*)flvi->itemBelow();
      bool bDel = false;

      QString strFile = old_flvi->getFileName().copy();
      if (bIsTar && bDeletingDir)
	{
	  for (QStringList::Iterator it = dirs.begin(); it != dirs.end(); ++it)
	    {
	      QRegExp re = "^" + *it;
	      if (re.match(strFile) != -1)
		{
		  bDel = true;
		  break;
		}
	    }
	}
      if (bDel || archiveContent->isSelected(old_flvi))
	{
	  if (!bIsTar)
	    list.append(strFile);
	  delete old_flvi;
	}		
    }
 
  archiveContent->setUpdatesEnabled(false);
  QApplication::setOverrideCursor( waitCursor );
  disableAll();
  arch->remove(&list);
  kdDebug(1601) << "-ArkWidget::action_delete" << endl;
}

void ArkWidget::slotOpenWith()
{
  FileLVI *pItem = archiveContent->currentItem();
  if (pItem  != NULL )
    {
      QString name = pItem->text(0); // get name
      QString fullname;
      fullname = "file:";
      fullname += m_settings->getTmpDir();
      fullname += name;

      m_extractList = new QStringList;
      m_extractList->append(name);
      archiveContent->setUpdatesEnabled(false);
      m_bOpenWithInProgress = true;
      m_strFileToView = fullname;
      QApplication::setOverrideCursor( waitCursor );
      disableAll();
      arch->unarchFile(m_extractList, m_settings->getTmpDir());
    }
}

bool ArkWidget::getOverwrite(ArchType _archtype)
{
  // returns true if Overwrite is set on this particular archive type
  switch (_archtype)
    {
    case ZIP_FORMAT:
      return m_settings->getZipExtractOverwrite();
    case TAR_FORMAT:
      return m_settings->getTarOverwriteFiles();
    case RAR_FORMAT:
      return m_settings->getRarOverwriteFiles();
    case ZOO_FORMAT:
      return m_settings->getZooOverwriteFiles();
    case LHA_FORMAT:
    case AA_FORMAT:
    case COMPRESSED_FORMAT:
      return true;
    case UNKNOWN_FORMAT:
      ASSERT(0); // never happens;
    }
  return false;
}

bool ArkWidget::reportExtractFailures(const QString & _dest,
				      QStringList *_list)
{
  // reports extract failures when Overwrite = False and the file
  // exists already in the destination directory.
  // If list is null, it means we are extracting all files.
  // Otherwise the list contains the files we are to extract.

  QString strFilename, tmp;
  struct stat statbuffer;
  bool bRedoExtract = false;

  QApplication::restoreOverrideCursor();

  ASSERT(_list != NULL);
  QString strDestDir = _dest;

  // make sure the destination directory has a / at the end.
  if (strDestDir.right(1) != '/')
    strDestDir += '/';

  if (_list->isEmpty())
  {
    // make the list
    FileListView *flw = fileList();
    FileLVI *flvi = (FileLVI*)flw->firstChild();
    while (flvi)
      {
	tmp = flvi->getFileName().local8Bit();
	_list->append(tmp);
	flvi = (FileLVI*)flvi->itemBelow();
      }
  }
  QStringList existingFiles;
  // now the list contains all the names we must verify.
  for (QStringList::Iterator it = _list->begin(); it != _list->end(); ++it)
    {
      strFilename = *it;
      QString strFullName = strDestDir + strFilename;
      if (stat(QFile::encodeName(strFullName), &statbuffer) != -1)
	existingFiles.append(strFilename);
    }
  
  int numFilesToReport = existingFiles.count();
  
  kdDebug(1601) << "There are " << numFilesToReport << " files to report existing already." << endl;

  // now report on the contents
  if (numFilesToReport == 1)  
    {
      kdDebug(1601) << "One to report" << endl;
      strFilename = *(existingFiles.at(0));
      QString message = strFilename + i18n("%1 will not be extracted because it will overwrite an existing file.\nGo back to Extract Dialog?").arg(strFilename);
      bRedoExtract =
	KMessageBox::questionYesNo(this, message) == KMessageBox::Yes;
    }
  else if (numFilesToReport != 0)
    {
      ExtractFailureDlg *fDlg = new ExtractFailureDlg(&existingFiles,
						      this);
      bRedoExtract = !fDlg->exec();
    }
  return bRedoExtract;
}


void ArkWidget::action_extract()
{
  QString extension;
  ArchType archtype = getArchType(m_strArchName, extension);
  ExtractDlg *dlg = new ExtractDlg(archtype, m_settings);

  // if they choose pattern, we have to tell arkwidget to select
  // those files... once we're in the dialog code it's too late.
  connect(dlg, SIGNAL(pattern(const QString &)), 
	  this, SLOT(selectByPattern(const QString &)));
  bool bRedoExtract = false;

  if (m_nNumSelectedFiles == 0)
    dlg->disableSelectedFilesOption();
  if (archiveContent->currentItem() == NULL)
    dlg->disableCurrentFileOption();

  // list of files to be extracted
  m_extractList = new QStringList;
  if (dlg->exec())
    {
      int extractOp = dlg->extractOp();
      kdDebug(1601) << "Extract op: " << extractOp << endl;
      archiveContent->setUpdatesEnabled(false);
      QApplication::setOverrideCursor( waitCursor );
      disableAll();

      // if overwrite is false, then we need to check for failure of
      // extractions.
      bool bOvwrt = getOverwrite(archtype); 

      switch(extractOp)
	{
	case ExtractDlg::All:
	  if (!bOvwrt)  // send empty list to indicate we're extracting all
	    bRedoExtract = reportExtractFailures(m_settings->getExtractDir(),
						 m_extractList);
	  if (!bRedoExtract) // if the user's OK with those failures, go ahead
	    arch->unarchFile(0);
	  break;
	case ExtractDlg::Pattern:
	case ExtractDlg::Selected:
	case ExtractDlg::Current:
	  if (extractOp != ExtractDlg::Current )
	    {
	      // make a list to send to unarchFile
	      FileListView *flw = fileList();
	      FileLVI *flvi = (FileLVI*)flw->firstChild();
	      while (flvi)
		{
		  if ( flw->isSelected(flvi) )
		    {
		      kdDebug(1601) << "unarching " << flvi->getFileName() << endl;
		      QCString tmp = QFile::encodeName(flvi->getFileName());
		      m_extractList->append(tmp);
		    }
		  flvi = (FileLVI*)flvi->itemBelow();
		}
	    }
	  else
	    {
	      FileLVI *pItem = archiveContent->currentItem();
	      if (pItem == 0)
		{
		  kdDebug(1601) << "Can't seem to figure out which is current!" << endl;
		  return;
		}
	      QString tmp = pItem->text(0);  // get the name
	      m_extractList->append( QFile::encodeName(tmp) );
	    }
	  if (!bOvwrt)
	    bRedoExtract =
	      reportExtractFailures(m_settings->getExtractDir(),
				    m_extractList);
	  if (!bRedoExtract)
	    arch->unarchFile(m_extractList); // extract selected files
	  break;
	default:
	  ASSERT(0);
	  // never happens
	  break;
	}
      
      delete dlg;
    }

  // user might want to change some options or the selection...
  if (bRedoExtract)
    action_extract();
}

void ArkWidget::action_edit()
{
  // begin an edit. This is like a view, but once the process exits,
  // the file is put back into the archive. If the user tries to quit or
  // close the archive, there will be a warning that any changes to the
  // files open under "Edit" will be lost unless the archive remains open.
  // [hmm, does that really make sense? I'll leave it for now.]

  m_bEditInProgress = true;
  action_view();
  // The rest of the action happens when the process exits.

}

void ArkWidget::action_view()
{
  FileLVI *pItem = archiveContent->currentItem();
  if (pItem  != NULL )
    {
      showFile(pItem);
    }
}

void ArkWidget::showFile( FileLVI *_pItem )
{
  QString name = _pItem->text(0); // get name

  QString fullname;
  fullname = "file:";
  fullname += m_settings->getTmpDir();
  fullname += name;

  m_extractList = new QStringList;
  m_extractList->append(name);

  archiveContent->setUpdatesEnabled(false);
  m_bViewInProgress = true;
  m_strFileToView = fullname;
  QApplication::setOverrideCursor( waitCursor );
  disableAll();
  arch->unarchFile(m_extractList, m_settings->getTmpDir() );
}

// Options menu //////////////////////////////////////////////////////

void ArkWidget::options_dirs()
{
  DirDlg *dd = new DirDlg( m_settings, this );
  dd->exec();
  delete dd;
}

void ArkWidget::options_keys()
{
    KKeyDialog::configureKeys(actionCollection(), xmlFile());
}

void ArkWidget::options_saveNow()
{
  m_settings->writeConfigurationNow();
}

// Popup /////////////////////////////////////////////////////////////


void ArkWidget::doPopup(QListViewItem *pItem, const QPoint &pPoint,
                        int nCol) // slot
  // do the right-click popup menus
{
  kdDebug(1601) << "+ArkWidget::doPopup" << endl;
  if (nCol == 0)
  {
    archiveContent->setCurrentItem(pItem);
    archiveContent->setSelected(pItem, true);
    ((QPopupMenu *)factory()->container("file_popup", this))->popup(pPoint);
  }
  else // clicked anywhere else but the name column
  {
    //m_archivePopup->popup(pPoint);

    ((QPopupMenu *)factory()->container("archive_popup", this))->popup(pPoint);

  }
  kdDebug(1601) << "-ArkWidget::doPopup" << endl;
}

// Service functions /////////////////////////////////////////////////

void ArkWidget::slotSelectionChanged()
{
  updateStatusSelection();
}


////////////////////////////////////////////////////////////////////
//////////////////// updateStatusSelection /////////////////////////
////////////////////////////////////////////////////////////////////

void ArkWidget::updateStatusSelection()
{
  m_nNumSelectedFiles = 0;
  m_nSizeOfSelectedFiles = 0;

  if (archiveContent)
    {
      FileLVI * flvi = (FileLVI*)archiveContent->firstChild();
      while (flvi)
	{
	  if (flvi->isSelected())
	    {
	      ++m_nNumSelectedFiles;
	      if (m_currentSizeColumn != -1)
		m_nSizeOfSelectedFiles +=
		  atoi(flvi->text(m_currentSizeColumn));
	    }
	  flvi = (FileLVI*)flvi->itemBelow();
	}
    }
  QString strInfo;
  if (m_nNumSelectedFiles == 0)
    {
      strInfo = i18n("0 Files Selected");
    }
  if (m_nNumSelectedFiles != 1)
    {
      strInfo = i18n("%1 Files Selected %1 KB").arg(KGlobal::locale()->formatNumber(m_nNumSelectedFiles, 0)).arg(KGlobal::locale()->formatNumber(m_nSizeOfSelectedFiles, 0));
    }
  else
    {
    strInfo = i18n("1 File Selected %1 KB").arg(KGlobal::locale()->formatNumber(m_nSizeOfSelectedFiles, 0));
    }
  m_pStatusLabelSelect->setText(strInfo);
  fixEnables();
}


void ArkWidget::selectByPattern(const QString & _pattern) // slot
{
// select all the files that match the pattern

  FileLVI * flvi = (FileLVI*)archiveContent->firstChild();
  QRegExp *glob = new QRegExp(_pattern, true, true); // file globber

  archiveContent->clearSelection();
  while (flvi)
    {
      if (glob->match(flvi->text(0), 0, 0) != -1)
	archiveContent->setSelected(flvi, true);
      flvi = (FileLVI*)flvi->itemBelow();
    }

  delete glob;
}

// Drag & Drop ////////////////////////////////////////////////////////

#if 0 // not sure I need this
void ArkWidget::dragEnterEvent(QDragEnterEvent* event)
{
  event->accept(QUrlDrag::canDecode(event));
}

#endif

void ArkWidget::dragMoveEvent(QDragMoveEvent *e)
{
  QStringList list;

  if (QUrlDrag::canDecode(e) &&
      !m_bDropSourceIsSelf)
    {
    e->accept();
  }
}


void ArkWidget::dropEvent(QDropEvent* e)
{
  kdDebug(1601) << "+ArkWidget::dropEvent" << endl;

  // I think I've got all the deletia covered... see slotAddDone.
  mpAddList = new QStringList; 

  if (QUrlDrag::decodeToUnicodeUris(e, *mpAddList))
  {
    dropAction(mpAddList);
  }

  kdDebug(1601) << "-dropEvent" << endl;
}

//////////////////////////////////////////////////////////////////////
///////////////////////// dropAction /////////////////////////////////
//////////////////////////////////////////////////////////////////////

void ArkWidget::dropAction(QStringList *list)
{
// Called by dropEvent

// The possibilities treated are as follows:
// drop a regular file into a window with
//   * an open archive - add it.
//   * no open archive - ask user to open an archive for adding file or cancel
// drop an archive into a window with
//   * an open archive - ask user to add to open archive or to open it freshly
//   * no open archive - open it
// drop many files (can be a mix of archives and regular) into a window with
//   * an open archive - add them.
//   * no open archive - ask user to open an archive for adding files or cancel

// and don't forget about gzip files.

  QString str;
  QStringList urls; // to be sent to addFile

  str = list->first();

  QString extension;
  if (1 == list->count() &&  (UNKNOWN_FORMAT != getArchType(str, extension)))
  {
    // if there's one thing being dropped and it's an archive
    if (isArchiveOpen())
    {
      // ask them if they want to add the dragged archive to the current
      // one or open it as the new current archive
      int nRet = KMessageBox::warningYesNoCancel(this,
				      i18n("Do you wish to add this to the current archive or open it as a new archive?"),
				      QString::null,
                                      i18n("Add"), i18n("Open"));
      if (KMessageBox::Yes == nRet) // add it
      {
	addFile(list);
	return;
      }
      else if (KMessageBox::Cancel == nRet) // cancel
      {
	delete list;
	return;
      }
    }

    // if I made it here, there's either no archive currently open
    // or they selected "Open".
    delete list;
    KURL url = str;
    file_open(url);
  }
  else
  {
    if (isArchiveOpen())
    {
      if (m_bIsSimpleCompressedFile && (m_nNumFiles == 1))
	{
	  QString strFilename;
	  KURL url = askToCreateRealArchive();
	  strFilename = url.path();
	  if (!strFilename.isEmpty())
	    {
	      m_pTempAddList = new QStringList(*list);
	      createRealArchive(strFilename);
	    }
	  delete list;
	  return;
	}
      // add the files to the open archive
      addFile(list);
    }
    else
    {
      // no archive is open, so we ask if the user wants to open one
      // for this/these file/files.

      QString str;
      str = (list->count() > 1)
	? i18n("There is no archive currently open. Do you wish to create one now for these files?")
        : i18n("There is no archive currently open. Do you wish to create one now for this file?");
      int nRet = KMessageBox::warningYesNo(this, str);
      if (nRet == KMessageBox::Yes) // yes
      {
	file_new();
	if (isArchiveOpen()) // they still could have canceled!
	{
	  addFile(list);
	}
      }
      else // basically a cancel on the drop.
	delete list;
    }
  }
}

//////////////////////////////////////////////////////////////////////
///////////////////////// showFavorite ///////////////////////////////
//////////////////////////////////////////////////////////////////////


void ArkWidget::showFavorite()
{
  const QFileInfoList *flist;
  QDir *fav;

  file_close();
  createFileListView();

  archiveContent->addColumn( i18n(" File ") );
  archiveContent->addColumn( i18n(" Size ") );
  archiveContent->setColumnAlignment(1, AlignRight);
  archiveContent->setMultiSelection( false );

  fav = new QDir( m_settings->getFavoriteDir() );
  if ( !fav->exists() )
    {
      KMessageBox::error( this, i18n("Archive directory does not exist."));
      return;
    }
  flist = fav->entryInfoList();
  QFileInfoListIterator flisti( *flist );
  ++flisti; // Skip . directory

  if ( (flisti.current())->fileName() == ".." )
    {
      FileLVI *flvi = new FileLVI(archiveContent);
      flvi->setText(0, "..");
      archiveContent->insertItem(flvi);
      ++flisti;
    }

  QString size;
  bool isDirectory;
  for( uint i=0; i < flist->count()-2; i++ )
    {
      QString name( (flisti.current())->fileName() );
      isDirectory = (flisti.current())->isDir();
      QString extension;
      if ( (getArchType(name, extension)!=-1) || (isDirectory) )
	{
	  FileLVI *flvi = new FileLVI(archiveContent);
	  flvi->setText(0, name);
	  if (!isDirectory)
	    {
	      size = KGlobal::locale()->formatNumber(flisti.current()->size(), 0);
	      flvi->setText(1, size);
	      archiveContent->insertItem(flvi);
	    }
	}
      ++flisti;
    }
  archiveContent->setColumnWidth(0, archiveContent->columnWidth(0) + 10 );

  setCaption( m_settings->getFavoriteDir() );

  delete fav;

  //	writeStatusMsg( i18n( "Archive Directory") );
}

/**
 * Writes a message in the status bar.
 * This message is visible during 5 seconds.
 */
#if 0
void ArkWidget::writeStatusMsg(const QString text)
{
	statusBarTimer->stop();
	statusBar()->changeItem(text, 0);
	statusBarTimer->start(5000,true);
}
#endif

#if 0
void ArkWidget::clearStatusBar()
{
	statusBar()->changeItem(QString::null,0);
}
#endif


void ArkWidget::arkWarning(const QString& msg)
{
  KMessageBox::information(this, msg);
}

#if 0
void ArkWidget::slotStatusBarTimeout()
{
	clearStatusBar();
}
#endif


void ArkWidget::createFileListView()
{
  delete archiveContent;
  archiveContent = new FileListView(this);
  archiveContent->setMultiSelection(true);
  setView(archiveContent);
  updateRects();
  archiveContent->show();
  connect( archiveContent, SIGNAL( selectionChanged()),
	   this, SLOT( slotSelectionChanged() ) );
  connect(archiveContent,
	  SIGNAL(rightButtonPressed(QListViewItem *,
				    const QPoint &, int)),
	  this, SLOT(doPopup(QListViewItem *,
			     const QPoint &, int)));
}

//////////////////////////////////////////////////////////////////////
//////////////////////// badBzipName /////////////////////////////////
//////////////////////////////////////////////////////////////////////

bool ArkWidget::badBzipName(const QString & _filename)
{
  if (_filename.right(3) == ".BZ" || _filename.right(4) == ".TBZ")
    KMessageBox::error(this, i18n("Sorry, bzip doesn't like filename extensions that use capital letters.") );
  else if (_filename.right(4) == ".tbz")
    KMessageBox::error(this, i18n("Sorry, bzip doesn't like filename extensions that aren't exactly \".bz\"."));
  else if (_filename.right(4) == ".BZ2" ||  _filename.right(5) == ".TBZ2")
    KMessageBox::error(this, i18n("Sorry, bzip2 doesn't like filename extensions that use capital letters."));
  else if (_filename.right(5) == ".tbz2")
    KMessageBox::error(this, i18n("Sorry, bzip2 doesn't like filename extensions that aren't exactly \".bz\".") );
  else
    return false;
  return true;
}

//////////////////////////////////////////////////////////////////////
////////////////////// createArchive /////////////////////////////////
//////////////////////////////////////////////////////////////////////


void ArkWidget::createArchive( const QString & _filename )
{

  // make sure we can write there

  Arch * newArch = 0;
  QString extension;
  switch( getArchType( _filename, extension) )
    {
    case TAR_FORMAT:
      newArch = new TarArch( m_settings, m_viewer, _filename );
      break;
    case ZIP_FORMAT:
      newArch = new ZipArch( m_settings, m_viewer, _filename );
      break;
    case LHA_FORMAT:
      newArch = new LhaArch( m_settings, m_viewer, _filename );
      break;
    case COMPRESSED_FORMAT:
      newArch = new CompressedFile( m_settings, m_viewer, _filename );
      break;
    case ZOO_FORMAT:
      newArch = new ZooArch( m_settings, m_viewer, _filename );
      break;
    case RAR_FORMAT:
      newArch = new RarArch( m_settings, m_viewer, _filename );
      break;
    case AA_FORMAT:
      newArch = new ArArch( m_settings, m_viewer, _filename );
      break;
    default:
      if (!badBzipName(_filename))
	KMessageBox::error(this, i18n("Unknown archive format or corrupted archive") );
      return;
    }

  if (!newArch->utilityIsAvailable())
    {
      KMessageBox::error(this, i18n("Sorry, the utility %1 is not in your PATH.\nPlease install it or contact your system administrator.").arg(newArch->getUtility()));
      return;
    }

  connect( newArch, SIGNAL(sigCreate(Arch *, bool, const QString &, int)),
	   this, SLOT(slotCreate(Arch *, bool, const QString &, int)) );
  connect( newArch, SIGNAL(sigDelete(bool)), this, SLOT(slotDeleteDone(bool)));
  connect( newArch, SIGNAL(sigAdd(bool)),
	   this, SLOT(slotAddDone(bool)));
  connect( newArch, SIGNAL(sigExtract(bool)),
	   this, SLOT(slotExtractDone()));

  archiveContent->setUpdatesEnabled(false);
  QApplication::setOverrideCursor( waitCursor );
  newArch->create();
  recent->addURL(_filename);
}

//////////////////////////////////////////////////////////////////////
//////////////////////// openArchive /////////////////////////////////
//////////////////////////////////////////////////////////////////////

void ArkWidget::openArchive(const QString & _filename )
{
  // figure out what kind of archive it is
  Arch *newArch = 0;
  
  QString extension;
  switch( getArchType( _filename, extension ) )
    {
    case TAR_FORMAT:
      newArch = new TarArch(m_settings, m_viewer, _filename );
      break;
    case ZIP_FORMAT:
      newArch = new ZipArch(m_settings, m_viewer, _filename );
      break;
    case LHA_FORMAT:
      newArch = new LhaArch( m_settings, m_viewer, _filename );
      break;
    case COMPRESSED_FORMAT:
      newArch = new CompressedFile( m_settings, m_viewer, _filename );
      break;
    case ZOO_FORMAT:
      newArch = new ZooArch( m_settings, m_viewer, _filename );
      break;
    case RAR_FORMAT:
      newArch = new RarArch( m_settings, m_viewer, _filename );
      break;
    case AA_FORMAT:
      newArch = new ArArch( m_settings, m_viewer, _filename );
      break;
    default:
      if (!badBzipName(_filename))
	{
	  // it's still a bad name, so let's try to figure out what it is.
	  // Maybe it just needs the proper extension!
	   KMimeMagic *mimePtr = KMimeMagic::self();
	   KMimeMagicResult * mimeResultPtr = mimePtr->findFileType(_filename);
	   QString mimetype = mimeResultPtr->mimeType();
	   if (mimetype == "application/x-gzip")
	     {
	       KMessageBox::error(this, i18n("Gzip archives need to have an extension `gz'."));
	       return;
	     }
	   else if (mimetype == "application/x-zoo")
	     {
	       KMessageBox::error(this, i18n("Zoo archives need to have an extension `zoo'."));
	       return;
	     }
	   else
	     {
	       KMessageBox::error( this, i18n("Unknown archive format or corrupted archive") );
	       // and just leave the old archive displayed
	       return;
	     }
	}
    }
  
  if (!newArch->utilityIsAvailable())
    {
      KMessageBox::error(this, i18n("Sorry, the utility %1 is not in your PATH.\nPlease install it or contact your system administrator.").arg(newArch->getUtility()));
      return;
    }

  connect( newArch, SIGNAL(sigOpen(Arch *, bool, const QString &, int)),
	   this, SLOT(slotOpen(Arch *, bool, const QString &,int)) );
  connect( newArch, SIGNAL(sigDelete(bool)),
	   this, SLOT(slotDeleteDone(bool)));
  connect( newArch, SIGNAL(sigAdd(bool)),
	   this, SLOT(slotAddDone(bool)));
  connect( newArch, SIGNAL(sigExtract(bool)),
	   this, SLOT(slotExtractDone()));

  archiveContent->setUpdatesEnabled(false);
  disableAll();
  QApplication::setOverrideCursor( waitCursor );
  newArch->open();
}

//////////////////////////////////////////////////////////////////////
///////////////////////// listingAdd /////////////////////////////////
//////////////////////////////////////////////////////////////////////

void ArkWidget::listingAdd(QStringList *_entries)
{
  // add the column data in _entries to the list view.

  FileLVI *flvi = new FileLVI( fileList() );

  int i = 0;
  for ( QStringList::Iterator it = _entries->begin();
	it != _entries->end(); ++it ) 
    {
      flvi->setText(i, *it);
      ++i;
    }
  archiveContent->insertItem(flvi);
}

//////////////////////////////////////////////////////////////////////
///////////////////////// setHeaders /////////////////////////////////
//////////////////////////////////////////////////////////////////////


void ArkWidget::setHeaders(QStringList *_headers,
			   int * _rightAlignCols, int _numColsToAlignRight)
{
  int i = 0;
  m_currentSizeColumn = -1;
  for ( QStringList::Iterator it = _headers->begin();
	it != _headers->end(); ++it, ++i ) 
    {
      QString str = *it;
       archiveContent->addColumn(str);
       if (str.contains(i18n("Size")))
	 m_currentSizeColumn = i;
    }

  for (int i = 0; i < _numColsToAlignRight; ++i)
    {
      archiveContent->setColumnAlignment( _rightAlignCols[i],
					  QListView::AlignRight );
    }
}

#include "arkwidget.moc"
