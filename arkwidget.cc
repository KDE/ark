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
#include <qmessagebox.h>
#include <qregexp.h>

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

// c includes

#include <sys/stat.h>
#include <errno.h>

// ark includes
#include "arkapp.h"

#include "dirDlg.h"
#include "generalOptDlg.h"
#include "selectDlg.h"
#include "shellOutputDlg.h"

#include "extractdlg.h"
#include "adddlg.h"
#include "arch.h"
#include "arkwidget.h"

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

ArkWidget::ArkWidget( QWidget *, const char *name ) : 
    KTMainWindow(name), m_nSizeOfFiles(0), m_nSizeOfSelectedFiles(0),
    m_nNumFiles(0), m_nNumSelectedFiles(0), m_bIsArchiveOpen(false),
    m_bIsSimpleCompressedFile(false), m_bDropSourceIsSelf(false),
    m_bViewInProgress(false), m_bOpenWithInProgress(false),
    m_bMakeCFIntoArchiveInProgress(false), m_pTempAddList(NULL),
    m_bDropFilesInProgress(false)
{
    kDebugInfo( 1601, "+ArkWidget::ArkWidget");
  
    m_viewer = new Viewer(this);
    m_settings = new ArkSettings;
    // Creates a temp directory for this ark instance
    unsigned int pid = getpid();
    QString tmpdir;
    tmpdir.sprintf( "/tmp/ark.%d/", pid );
    QString ex( "mkdir " + tmpdir + " &>/dev/null" );
    system( ex.local8Bit() );

    m_settings->setTmpDir( tmpdir );
    
    ArkApplication::getInstance()->addWindow();

    // Build the ark UI
    kDebugInfo( 1601, "Build the GUI");

    setupStatusBar();
    setupActions();
    createFileListView();
    // enable DnD
    setAcceptDrops(true);
    arch = 0;
    initialEnables();

    kDebugInfo( 1601, "-ArkWidget::ArkWidget");
    resize(640,300);
}

ArkWidget::~ArkWidget()
{
  KConfig *kc = m_settings->getKConfig();
  kc->setGroup( "ark" );
  recent->saveEntries(kc);
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
  system( ex.local8Bit() );

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

  closeAction = new KAction(i18n("&Close Archive"), 0, this,
			    SLOT(file_close()),
			    actionCollection(), "close_arch");

  recent = KStdAction::openRecent(this,
				  SLOT(file_openRecent(const KURL&)),
				  actionCollection());
  KConfig *kc = m_settings->getKConfig();
  recent->loadEntries(kc);    // this doesn't seem to work!

  (void)KStdAction::keyBindings();

  shellOutputAction  = new KAction(i18n("&View shell output..."), 0, this,
				   SLOT(edit_view_last_shell_output()),
				   actionCollection(), "shell_output");

  KStdAction::quit(this, SLOT(window_close()), actionCollection());

  addFileAction = new KAction(i18n("&Add File"), "ark_addfile", 0, this,
				SLOT(action_add()),
				actionCollection(), "addfile");

  addDirAction = new KAction(i18n("Add Di&r"), "ark_adddir", 0, this,
				SLOT(action_add_dir()),
				actionCollection(), "adddir");

  extractAction = new KAction(i18n("&Extract"), "ark_extract", 0, this,
				SLOT(action_extract()),
				actionCollection(), "extract");

  deleteAction = new KAction(i18n("&Delete"), "ark_delete", 0, this,
			     SLOT(action_delete()),
			     actionCollection(), "delete");

  selectAllAction = new KAction(i18n("Select All"), "ark_selectall", 0, this,
			     SLOT(edit_selectAll()),
			     actionCollection(), "select_all");

  viewAction = new KAction(i18n("&View"), "ark_view", 0, this,
			   SLOT(action_view()),
			   actionCollection(), "view");

  popupViewAction  = new KAction(i18n("&View"), "ark_view", 0, this,
			   SLOT(action_view()),
			   actionCollection(), "popup_menu_view");

  openWithAction = new KAction(i18n("&Open with"), 0, this,
			   SLOT(slotOpenWith()),
			   actionCollection(), "open_with");

  popupOpenWithAction  = new KAction(i18n("&Open with"), 0, this,
			   SLOT(slotOpenWith()),
			   actionCollection(), "popup_menu_open_with");

  settingsAction =  new KAction(i18n("&Settings"), "ark_options", 0, this,
			   SLOT(options_dirs()),
			   actionCollection(), "settings");
 
  selectAction =  new KAction(i18n("Select..."), 0, this,
			     SLOT(edit_select()),
			     actionCollection(), "select");
  deselectAllAction =  new KAction(i18n("Deselect All"), 0, this,
			     SLOT(edit_deselectAll()),
			     actionCollection(), "deselect_all");

  invertSelectionAction  =  new KAction(i18n("Invert Selection"), 0, this,
					SLOT(edit_invertSel()),
					actionCollection(),
					"invert_selection");

  (void)new KAction(i18n("&General"), 0, this,
		    SLOT(options_general()),
		    actionCollection(),
		    "general");

  (void)new KAction(i18n("&Directories..."), 0, this,
		    SLOT(options_dirs()),
		    actionCollection(),
		    "directories");

  (void)new KAction(i18n("&Keys..."), 0, this,
		    SLOT(options_keys()),
		    actionCollection(),
		    "keys");

  KStdAction::showToolbar(this, SLOT(toggleToolBar()), actionCollection());
  KStdAction::showStatusbar(this, SLOT(toggleStatusBar()), actionCollection());

  createGUI();
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
  kDebugInfo( 1601, "+ArkWidget::setupStatusBar");

  KStatusBar *sb = statusBar();

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

  kDebugInfo( 1601, "-ArkWidget::setupStatusBar");
}

void ArkWidget::initialEnables()
{
  // start out with some menu items disabled
  closeAction->setEnabled(false);
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
  kDebugInfo( 1601, "+ArkWidget::updateStatusTotals");
  m_nNumFiles = 0;
  m_nSizeOfFiles = 0;
  if (archiveContent)
    {
      FileLVI *pItem = (FileLVI *)archiveContent->firstChild();
      while (pItem)
	{
	  ++m_nNumFiles;
	  
	  kDebugInfo( 1601, "Adding %s\n",
		      (const char *)pItem->text(m_currentSizeColumn));

	  kDebugInfo( 1601, "Adding %d\n",
		      atoi(pItem->text(m_currentSizeColumn)));
	  if (m_currentSizeColumn != -1)
	    m_nSizeOfFiles += atoi(pItem->text(m_currentSizeColumn));
	  pItem = (FileLVI *)pItem->nextSibling();
	}
    }
  kDebugInfo( 1601, "We have %d elements\n", m_nNumFiles);

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
  kDebugInfo( 1601, "-ArkWidget::updateStatusTotals");
}

//////////////////////////////////////////////////////////////////////
///////////////////////// file_open //////////////////////////////////
//////////////////////////////////////////////////////////////////////


void ArkWidget::file_open(const QString & strFile)
{
  struct stat statbuffer;
  
  if (stat(strFile, &statbuffer) == -1)
    {
      if (errno == ENOENT || errno == ENOTDIR || errno ==  EFAULT)
	{
	  QString x = "file:";
	  QString errormesg = i18n("The archive ") + strFile +
	    i18n(" does not exist");
	  KMessageBox::error(this, errormesg);
	}
      else if (errno == EACCES)
	{
	  QString errormesg = i18n("Can't access the archive ") + strFile;
	  KMessageBox::error(this, errormesg);
	}
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
}


//////////////////////////////////////////////////////////////////////
//
// ArkWidget slots
//
//////////////////////////////////////////////////////////////////////


void ArkWidget::saveProperties()
{
  kDebugInfo( 1601, "+saveProperties (exit)");

  KConfig *kc = m_settings->getKConfig();
  kc->setGroup( "ark" );
  recent->saveEntries(kc);

#if 0
  if ( m_settings->isSaveOnExitChecked() )
    accelerators->writeSettings( m_settings->getKConfig() );
#endif

  m_settings->writeConfiguration();

  kDebugInfo( 1601, "-saveProperties (exit)");
}

QString ArkWidget::getNewFileName()
{
  KURL url = KFileDialog::getSaveURL(QString::null,
				     m_settings->getFilter());
  QString strFile;

  if (!url.isEmpty())
    {
      strFile = url.path();  // needs work for network stuff XXX
    }
  return strFile;
} 


// File menu /////////////////////////////////////////////////////////

QString ArkWidget::getCreateFilename()
{
  int choice=0;
  struct stat statbuffer;
  QString strFile = getNewFileName();
  
  if (!strFile.isEmpty())
    {
      while (true)
	// keep asking for filenames as long as the user doesn't want to 
	// overwrite existing ones; break if they agree to overwrite
	// or if the file doesn't already exist. Return if they cancel.
	// Also check for proper extensions.
	{
	  if (stat(strFile, &statbuffer) != -1)  // already exists!
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
		  return "";
		}
	      else
		{
		  strFile = getNewFileName();
		  if (!strFile.isEmpty())
		    continue;
		  else
		    return "";
		}
	    }
	  // if we got here, the file does not already exist.
	  if (! strFile.contains('.'))
	    {
	      // if the filename has no dot in it, ask should we append ".zip"?
	      int nRet = KMessageBox::warningYesNo(0, i18n("Your file is missing an extension to indicate the archive type.\nShall create a file of the default type (ZIP)?"), i18n("Error"));
	      if (nRet == KMessageBox::Yes)
		{
		  strFile += ".zip";
		  continue; // gotta check if it exists again
		}
	      else // no? well choose a different filename then.
		{
		  strFile = getNewFileName();
		  if (!strFile.isEmpty())
		    continue;
		  else
		    return "";
		}
	    }
	  else
	    break; 
	} // end of while loop
      
    }
  return strFile;
}

void ArkWidget::file_new()
{
  QString strFile = getCreateFilename();
  if (!strFile.isEmpty())
    createArchive( strFile );
}

void ArkWidget::slotCreate(Arch * _newarch, bool _success,
			   const QString & _filename, int _flag)
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
      m_bIsSimpleCompressedFile =
	(getArchType(m_strArchName) == COMPRESSED_FORMAT);
      fixEnables();
      if (m_bMakeCFIntoArchiveInProgress)
	{
	  QStringList listForCompressedFile;
	  listForCompressedFile.append(m_compressedFile);
	  addFile(&listForCompressedFile);
	}
    }
  else
    {
      QMessageBox::warning(this, i18n("Error"), i18n("\nSorry - ark cannot create an archive of that type.\n\n  [Hint:  The filename should have an extension such as `.zip' to\n  indicate the type of the archive. Please see the help pages for\n  more information on supported archive formats.]"));
    }
  QApplication::restoreOverrideCursor();
}

void ArkWidget::file_newWindow()
{
  kDebugInfo( 1601, "-ArkWidget::file_newWindow");
  
  ArkWidget *kw = new ArkWidget;
  kw->show();
  kDebugInfo( 1601, "-ArkWidget::file_newWindow");

}

void ArkWidget::file_open()
{
  kDebugInfo( 1601, "+ArkWidget::file_open");

  KURL url;
  QString strFile;
  url = KFileDialog::getOpenURL(m_settings->getOpenDir(),
				m_settings->getFilter(), this);

  qApp->processEvents();

  // do I have to remove this later if it's a temporary from net?
  // Needs work. XXX
  if (!url.isEmpty())
    {
      KIO::NetAccess::download(url, strFile); 
      m_settings->clearShellOutput();
      recent->addURL(url);
      file_open(strFile);  // note: assumes it is local for now
    }
  kDebugInfo( 1601, "-ArkWidget::file_open");
}

void ArkWidget::file_openRecent(const KURL& url)
{
  QString strFile;
  if (!url.isEmpty())
  {
    KIO::NetAccess::download(url, strFile); 
    m_settings->clearShellOutput();
    recent->addURL(url);
    file_open(strFile);
    return;
  }
}

void ArkWidget::showZip( QString _filename )
{
  kDebugInfo( 1601, "+ArkWidget::showZip");
  createFileListView();
  openArchive( _filename );
  kDebugInfo( 1601, "-ArkWidget::showZip");
}

void ArkWidget::slotOpen(Arch *_newarch, bool _success,
			 const QString & _filename, int _flag )
{
  kDebugInfo( 1601, "+ArkWidget::slotOpen");
  
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
	    KMessageBox::information(this, i18n("This archive is read-only."));
	  }
	setCaption( _filename );
	//	createActionMenu( _flag );
	arch = _newarch;
	updateStatusTotals();
	m_bIsArchiveOpen = true;
	m_bIsSimpleCompressedFile =
	  (getArchType(m_strArchName) == COMPRESSED_FORMAT);
    }
  fixEnables();
  QApplication::restoreOverrideCursor();
  kDebugInfo( 1601, "-ArkWidget::slotOpen");
}

void ArkWidget::slotDeleteDone(bool _bSuccess)
{
  kDebugInfo(1601, "+ArkWidget::slotDeleteDone------------------------------");
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
  kDebugInfo(1601, "-ArkWidget::slotDeleteDone");
}

void ArkWidget::slotExtractDone()
{
  kDebugInfo(1601, "+ArkWidget::slotExtractDone");
  QApplication::restoreOverrideCursor();
  if (m_bViewInProgress)
    {
      m_bViewInProgress = false;
      new KRun (m_strFileToView);
    }
  else if (m_bOpenWithInProgress)
    {
      m_bOpenWithInProgress = false;
      KURL::List list;
      KURL url = m_strFileToView;
      list.append(url);
      KOpenWithDlg l( list, i18n("Open With:"), "", (QWidget*)0L);
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
  archiveContent->setUpdatesEnabled(true);
  fixEnables();  
  kDebugInfo(1601, "-ArkWidget::slotExtractDone");
}

void ArkWidget::slotAddDone(bool _bSuccess)
{
  kDebugInfo(1601, "+ArkWidget::slotAddDone");
  archiveContent->setUpdatesEnabled(true);
  archiveContent->triggerUpdate();
  if (_bSuccess)
    {
      file_reload();
      if (m_bDropFilesInProgress)
	{
	  m_bDropFilesInProgress = false;
	  delete m_pTempAddList;
	  m_pTempAddList = NULL;
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
  fixEnables();
  QApplication::restoreOverrideCursor();
  kDebugInfo(1601, "-ArkWidget::slotAddDone");
}

//////////////////////////////////////////////////////////////////////
/////////////////////////// disableAll ///////////////////////////////
//////////////////////////////////////////////////////////////////////

void ArkWidget::disableAll() // private
{
  kDebugInfo( 1601, "+ArkWidget::disableAll");

  openAction->setEnabled(false);
  newArchAction->setEnabled(false);
  closeAction->setEnabled(false);
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

  kDebugInfo( 1601, "-ArkWidget::disableAll");
}

//////////////////////////////////////////////////////////////////////
/////////////////////////// fixEnables ///////////////////////////////
//////////////////////////////////////////////////////////////////////

void ArkWidget::fixEnables() // private
{
  kDebugInfo( 1601, "+ArkWidget::fixEnables");

  bool bHaveFiles = (m_nNumFiles > 0);
  bool bReadOnly = false;
  bool bAddDirSupported = true;
  enum ArchType archtype = getArchType(m_strArchName);
  if (archtype == ZOO_FORMAT || archtype == AA_FORMAT)
    bAddDirSupported = false;

  if (arch)
    bReadOnly = arch->isReadOnly();

  openAction->setEnabled(true);
  newArchAction->setEnabled(true);
  closeAction->setEnabled(bHaveFiles);
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

  kDebugInfo( 1601, "-ArkWidget::fixEnables");
}


void ArkWidget::file_reload()
{
  if (isArchiveOpen())
    {
      QString filename = arch->fileName();
      file_close();
      file_open( filename );
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
  kDebugInfo( 1601, "+ArkWidget::file_close");
  if (isArchiveOpen())
    {
      delete arch;
      arch = 0;
      setCaption("");
      m_bIsArchiveOpen = false;
      
      if (archiveContent)
	{
	  archiveContent->clear();
	}
      delete archiveContent;
      archiveContent = 0;
      setView(0);
      ArkApplication::getInstance()->removeOpenArk(m_strArchName);
      updateStatusTotals();
      updateStatusSelection();
      fixEnables();
    }
  kDebugInfo( 1601, "-ArkWidget::file_close");
}

void ArkWidget::window_close()
{
    kDebugInfo( 1601, "+ArkWidget::window_close");

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
    kDebugInfo( 1601, "-ArkWidget::window_close");
}


void ArkWidget::closeEvent( QCloseEvent * )
{
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
      kDebugError(reg_exp.isValid(), 0, 1601, "ArkWidget::edit_select: regular expression is not valid.");
		
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

QString ArkWidget::askToCreateRealArchive()
{
  // ask user whether to create a real archive from a compressed file
  // returns filename if so
  QString strFilename;
  int choice =
    KMessageBox::warningYesNo(0, i18n("You are currently working with a simple compressed file.\nWould you like to make it into an archive so that it can contain multiple files?\nIf so, you must choose a name for your new archive."), i18n("Warning"));
  if (choice == KMessageBox::Yes)
    {
      m_bMakeCFIntoArchiveInProgress = true;
      strFilename = getCreateFilename();
    }
  return strFilename;
}

void ArkWidget::createRealArchive(const QString &strFilename)
{
  FileListView *flw = fileList();  
  FileLVI *flvi = (FileLVI*)flw->firstChild();
  m_compressedFile = flvi->getFileName().local8Bit();
  QString tmpdir = m_settings->getTmpDir();
  m_compressedFile = "file:" + tmpdir + "/" + m_compressedFile;
  kDebugInfo(1601, "The compressed file is %s",
	     (const char *)m_compressedFile);
  createArchive(strFilename);
  // the file will be moved into the new archive in slotCreate.
}

// Action menu /////////////////////////////////////////////////////////

void ArkWidget::action_add()
{
  ArchType archtype = getArchType(m_strArchName);
  if (m_bIsSimpleCompressedFile && (m_nNumFiles == 1))
    {
      QString strFilename = askToCreateRealArchive();
      if (!strFilename.isEmpty())
	{
	  createRealArchive(strFilename);
	}
      return;
    }
  kDebugInfo( 1601, "Add dir: %s", (const char *)m_settings->getAddDir());
  AddDlg *dlg = new AddDlg(archtype, m_settings->getAddDir(),
			   m_settings, this, "adddlg");
  if (dlg->exec())
    {
      QStringList *list = dlg->getFiles();
      if (list->count() > 0)
	{
	  if (m_bIsSimpleCompressedFile && list->count() > 1)
	    {
	      QString strFilename = askToCreateRealArchive();
	      if (!strFilename.isEmpty())
		{
		  createRealArchive(strFilename);
		}
	      return;
	    }
	  addFile(list);
	}
    }
}

void ArkWidget::addFile(QStringList *list)
{
  archiveContent->setUpdatesEnabled(false);
  disableAll();
  QApplication::setOverrideCursor( waitCursor );
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

  kDebugInfo( 1601, "+ArkWidget::action_delete");
  
  if (archiveContent->isSelectionEmpty())
    return; // shouldn't happen - delete should have been disabled!

  bool bIsTar = getArchType(m_strArchName) == TAR_FORMAT;
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
  kDebugInfo( 1601, "-ArkWidget::action_delete");
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

      QStringList list;
      list.append(name);
      archiveContent->setUpdatesEnabled(false);
      m_bOpenWithInProgress = true;
      m_strFileToView = fullname;
      QApplication::setOverrideCursor( waitCursor );
      disableAll();
      arch->unarchFile( &list, m_settings->getTmpDir() );
    }
}

void ArkWidget::action_extract()
{
  ExtractDlg *dlg = new ExtractDlg(getArchType(m_strArchName),
				   m_settings);

  // if they choose pattern, we have to tell arkwidget to select
  // those files... once we're in the dialog code it's too late.
  connect(dlg, SIGNAL(pattern(const QString &)), 
	  this, SLOT(selectByPattern(const QString &)));

  if (dlg->exec())
    {
      int extractOp = dlg->extractOp();
      kDebugInfo( 1601, "Extract op: %d", extractOp);
      archiveContent->setUpdatesEnabled(false);
      QApplication::setOverrideCursor( waitCursor );
      disableAll();

      switch(extractOp)
	{
	case ExtractDlg::All:
	  arch->unarchFile(0);
	  break;
	case ExtractDlg::Pattern:
	case ExtractDlg::Selected:
	  {
	    // make a list to send to unarchFile
	    QStringList *list = new QStringList;
	    FileListView *flw = fileList();
	    FileLVI *flvi = (FileLVI*)flw->firstChild();
	    QString tmp;
	    while (flvi)
	      {
		if ( flw->isSelected(flvi) )
		  {
		    kDebugInfo( 1601, "unarching %s",
				flvi->getFileName().ascii() );
		    tmp = flvi->getFileName().local8Bit();
		    list->append(tmp.local8Bit());
		  }
		flvi = (FileLVI*)flvi->itemBelow();
	      }
	    arch->unarchFile(list); // extract selected files
	    delete list;
	    break;
	  }
	case ExtractDlg::Current:
	  {
	    FileLVI *pItem = archiveContent->currentItem();
	    if (pItem == 0)
	      kDebugInfo( 1601, "Can't seem to figure out which is current!");
	    else
	      {
		QString tmp = pItem->text(0);  // get the name
		QStringList *list = new QStringList;
		list->append(tmp.local8Bit());
		arch->unarchFile(list) ;
		delete list;
	      }
	    break;
	  }
	default:
	  ASSERT(0);
	  // never happens
	  break;
	}
      
      delete dlg;
    }
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

  QStringList list;
  list.append(name);

  archiveContent->setUpdatesEnabled(false);
  m_bViewInProgress = true;
  m_strFileToView = fullname;
  QApplication::setOverrideCursor( waitCursor );
  disableAll();
  arch->unarchFile( &list, m_settings->getTmpDir() );
}

// Options menu //////////////////////////////////////////////////////

void ArkWidget::options_general()
{
  GeneralDlg *gd = new GeneralDlg( m_settings, this );
  gd->exec();
  delete gd;
}

void ArkWidget::options_dirs()
{
  DirDlg *dd = new DirDlg( m_settings, this );
  dd->exec();
  delete dd;
}

void ArkWidget::options_keys()
{
  //  KKeyDialog::configureKeys(accelerators, this);
}

#if 0
void ArkWidget::options_saveNow()
{
  m_settings->writeConfigurationNow();
}

void ArkWidget::options_saveOnExit()
{
  optionsMenu->setItemChecked(eMSaveOnExit,
			      !m_settings->isSaveOnExitChecked());
  m_settings->setSaveOnExitChecked( !m_settings->isSaveOnExitChecked() );
}

#endif

// Popup /////////////////////////////////////////////////////////////


void ArkWidget::doPopup(QListViewItem *pItem, const QPoint &pPoint,
                        int nCol) // slot
  // do the right-click popup menus
{
  kDebugInfo(1601, "+ArkWidget::doPopup");
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
  kDebugInfo(1601, "-ArkWidget::doPopup");
}

// Service functions /////////////////////////////////////////////////

void ArkWidget::slotSelectionChanged()
{
  kDebugInfo( 1601, "+ArkWidget::slotSelectionChanged");
  updateStatusSelection();
  kDebugInfo( 1601, "-ArkWidget::slotSelectionChanged");
}


////////////////////////////////////////////////////////////////////
//////////////////// updateStatusSelection /////////////////////////
////////////////////////////////////////////////////////////////////

void ArkWidget::updateStatusSelection()
{
  kDebugInfo( 1601, "+ArkWidget::updateStatusSelection");

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
      strInfo = i18n("%1 Files Selected %1 KB")
	.arg(KGlobal::locale()->formatNumber(m_nNumSelectedFiles, 0))
	.arg(KGlobal::locale()->formatNumber(m_nSizeOfSelectedFiles, 0));
    }
  else
    {
    strInfo = i18n("1 File Selected %1 KB")
      .arg(KGlobal::locale()->formatNumber(m_nSizeOfSelectedFiles, 0));
    }
  m_pStatusLabelSelect->setText(strInfo);
  fixEnables();
  kDebugInfo( 1601, "-ArkWidget::updateStatusSelection");
}


void ArkWidget::selectByPattern(const QString & _pattern) // slot
{
// select all the files that match the pattern

  FileLVI * flvi = (FileLVI*)archiveContent->firstChild();
  QRegExp *glob = new QRegExp(_pattern, true, true); // file globber

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
  kDebugInfo( 1601, "+ArkWidget::dropEvent");

  QStringList list;

  if (QUrlDrag::decodeToUnicodeUris(e, list))
  {
    dropAction(&list);
  }

  kDebugInfo( 1601, "-dropEvent");
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

  // make sure they are proper URLs
  for(QStringList::Iterator it = list->begin(); it != list->end(); ++it)
    {
      str = *it;
      kDebugInfo( 1601, "%s", (const char *)str);
      if (str.left(5) != QString("file:"))
	{
	  str = "file:" + str;
	}
      urls.append(str);
    }

  if (1 == list->count() &&  (UNKNOWN_FORMAT != getArchType(str)))
  {
    // if there's one thing being dropped and it's an archive
    if (isArchiveOpen())
    {
      // ask them if they want to add the dragged archive to the current
      // one or open it as the new current archive
      int nRet = QMessageBox::warning(this, i18n("Question"),
				      i18n("Do you wish to add this to the current archive or open it as a new archive?"),
				      i18n("Add"), i18n("Open"),
				      i18n("Cancel"), 0, 1);
      if (0 == nRet) // add it
      {
	addFile(&urls);
	return;
      }
      else if (2 == nRet) // cancel
      {
	return;
      }
    }

    // if I made it here, there's either no archive currently open
    // or they selected "Open".
    str = urls.first();
    if (str.left(5) == "file:")
      str = str.right(str.length()-5);  // get rid of "file:" part of url
    file_open(str);
  }
  else
  {
    if (isArchiveOpen())
    {
      if (m_bIsSimpleCompressedFile && (m_nNumFiles == 1))
	{
	  QString strFilename = askToCreateRealArchive();
	  if (!strFilename.isEmpty())
	    {
	      m_pTempAddList = new QStringList(urls);
	      createRealArchive(strFilename);
	    }
	  return;
	}
      // add the files to the open archive
      addFile(&urls);
    }
    else
    {
      // no archive is open, so we ask if the user wants to open one
      // for this/these file/files.

      int nRet = QMessageBox::warning(this, i18n("Error"),
				      (list->count() > 1) ?
				      i18n("There is no archive currently open. Do you wish to open one now for these files?") : i18n("There is no archive currently open. Do you wish to open one now for this file?"),
				      i18n("Yes"), i18n("No"), 0, 0, 1);
      if (0 == nRet) // yes
      {
	file_new();
	if (isArchiveOpen()) // they still could have canceled!
	{
	  addFile(&urls);
	}
      }
      // no else: just forget it.
    }
  }
}

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
      if ( (getArchType(name)!=-1) || (isDirectory) )
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
	statusBar()->changeItem("",0);
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


ArchType ArkWidget::getArchType( QString archname )
{
  if ((archname.right(4) == ".tgz")
      || (archname.right(7) == ".tar.gz")
      || (archname.right(6) == ".tar.Z")
      || (archname.right(7) == ".tar.bz")
      || (archname.right(8) == ".tar.bz2")
      || (archname.right(8) == ".tar.lzo")
      || (archname.right(4) == ".tbz")
      || (archname.right(4) == ".tzo")
      || (archname.right(4) == ".taz")
      || (archname.right(5) == ".tbz2")
      || (archname.right(4) == ".tar"))
  {
    return TAR_FORMAT;
  }
  if ((archname.right(4) == ".lha") || (archname.right(4) == ".lzh"))
  {
    return LHA_FORMAT;
  }
  if (archname.right(4) == ".zip")
  {
    return ZIP_FORMAT;
  }
  if (archname.right(3) == ".gz" || archname.right(4) == ".lzo"
      || archname.right(3) == ".bz" || archname.right(4) == ".bz2"
      || archname.right(2) == ".Z")
    {
      return COMPRESSED_FORMAT;
    }
  if (archname.right(4) == ".zoo")
    return ZOO_FORMAT;
  if (archname.right(4) == ".rar")
    return RAR_FORMAT;
  if (archname.right(2) == ".a")
    return AA_FORMAT;
  return UNKNOWN_FORMAT;
}


void ArkWidget::createArchive( const QString & _filename )
{
  Arch * newArch = 0;
  switch( getArchType( _filename ) )
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
      KMessageBox::error(this,
			 i18n("Unknown archive format or corrupted archive") );
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
}

void ArkWidget::openArchive(const QString & _filename )
{
  Arch *newArch = 0;
  
  switch( getArchType( _filename ) )
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
      KMessageBox::error( this, i18n("Unknown archive format or corrupted archive") );
      // and just leave the old archive displayed
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
