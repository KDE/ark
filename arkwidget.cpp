/*
 Emacs: -*- mode: c++; c-basic-offset: 4; -*-

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 2002: Helio Chissini de Castro <helio@conectiva.com.br>
 2001-2002: Roberto Teixeira <maragato@kde.org>
 2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
 1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1997-1999: Rob Palmbos palm9744@kettering.edu

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
#include <stdlib.h>

// Qt includes
#include <qdragobject.h>
#include <qwhatsthis.h>

// KDE includes
#include <kdebug.h>
#include <kdebugclasses.h>
#include <klocale.h>
#include <qpopupmenu.h>
#include <kkeydialog.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>
#include <kio/job.h>
#include <kopenwith.h>
#include <kaction.h>
#include <kstdaction.h>
#include <ktempfile.h>
#include <kio/progressbase.h>
#include <kmimemagic.h>
#include <kedittoolbar.h>
#include <kstandarddirs.h>
#include <kprocess.h>
#include <kmainwindow.h>
#include <kstatusbar.h>
#include <kfiledialog.h>

// c includes
#include <errno.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <config.h>

// for statfs:
#ifdef BSD4_4
#include <sys/mount.h>
#elif defined(__linux__)
#include <sys/vfs.h>
#elif defined(__sun)
#include <sys/statvfs.h>
#define STATFS statvfs
#elif defined(_AIX)
#include <sys/statfs.h>
#endif

#ifndef STATFS
#define STATFS statfs
#endif

// ark includes
#include "arkapp.h"
#include "generalOptDlg.h"
#include "selectDlg.h"
#include "extractdlg.h"
#include "arch.h"
#include "arkwidget.h"
#include "filelistview.h"
#include "arksettings.h"

bool 
Utilities::haveDirPermissions( const QString &strFile )
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
		KMessageBox::error(0, i18n("You do not have permission to write to the directory %1").arg(dir.local8Bit()));
		return false;
	}
	return true;
}

bool 
Utilities::diskHasSpace(const QString &dir, long size)
	// check if disk has enough space to accomodate (a) new file(s) of
	// the given size in the partition containing the given directory
{
	fprintf(stderr, "Size: %ld\n", size);
	struct STATFS buf;
	if (STATFS(QFile::encodeName(dir), &buf) == 0)
	{
		double nAvailable = (double)buf.f_bavail * buf.f_bsize;
		if ( nAvailable < (double)size )
		{
			KMessageBox::error(0, i18n("You have run out of disk space."));
			return false;
		}
	}
	else
	{
		// something bad happened
      Q_ASSERT(0);
	}
	return true;
}

long 
Utilities::getSizes(QStringList *list)
{
	long sum = 0;
	QString str;
	
	for ( QStringList::Iterator it = list->begin(); it != list->end(); ++it)
	{
		str = *it;
		QFile f(str.right(str.length()-5));
		sum += f.size();
	}
	return sum;
}

//----------------------------------------------------------------------
//
//  Class ArkWidget starts here
//
//----------------------------------------------------------------------

ArkWidget::ArkWidget( QWidget *, const char *name ) :
    KMainWindow(0, name), ArkWidgetBase(this),
    m_bViewInProgress(false), m_bOpenWithInProgress(false),
    m_bEditInProgress(false),
    m_bMakeCFIntoArchiveInProgress(false), m_extractOnly(false),
    m_extractRemote(false), m_pTempAddList(NULL),
    m_bDropFilesInProgress(false), mpTempFile(NULL),
    mpDownloadedList(NULL), mpAddList(NULL)
{
	
	kdDebug(1601) << "+ArkWidget::ArkWidget" << endl;
	
	setArkInstanceId( ArkApplication::getInstance()->addWindow() );
	
	// Build the ark UI
	kdDebug(1601) << "Build the GUI" << endl;
	
	setupStatusBar();
	createFileListView();
	setupActions();
	
	// enable DnD
	setAcceptDrops(true);
	initialEnables();
	
	kdDebug(1601) << "-ArkWidget::ArkWidget" << endl;
	resize(640,300);
}

ArkWidget::~ArkWidget()
{
	// Call common function from arkwidgetbase
	cleanArkTmpDir();
	
	kdDebug(1601) << "-ArkWidget::~ArkWidget" << endl;
}

void 
ArkWidget::setupActions()
{
	// setup File menu
  	newWindowAction = new KAction(i18n("New &Window"), 0, this,
			SLOT(file_newWindow()), actionCollection(), "new_window");
	
	newArchAction = KStdAction::openNew(this, SLOT(file_new()),	actionCollection());
	openAction = KStdAction::open(this, SLOT(file_open()), actionCollection());
	
	reloadAction = new KAction(i18n("Re&load"), "reload", 0, this, SLOT(file_reload()), actionCollection(), "reload_arch");
	saveAsAction = KStdAction::saveAs(this, SLOT(file_save_as()), actionCollection());
	closeAction = new KAction(i18n("&Close Archive"), 0, this, SLOT(file_close()), actionCollection(), "close_arch");

	recent = KStdAction::openRecent(this, SLOT(file_open(const KURL&)), actionCollection());
	KConfig *kc = m_settings->getKConfig();
	recent->loadEntries(kc);
	
	shellOutputAction  = new KAction(i18n("&View Shell Output"), 0, this,
			SLOT(edit_view_last_shell_output()), actionCollection(), "shell_output");
	
	KStdAction::quit(this, SLOT(window_close()), actionCollection());
	
	addFileAction = new KAction(i18n("Add &File..."), "ark_addfile", 0, this,
			SLOT(action_add()), actionCollection(), "addfile");
	
	addDirAction = new KAction(i18n("Add &Directory..."), "ark_adddir", 0, this,
			SLOT(action_add_dir()), actionCollection(), "adddir");
	
	extractAction = new KAction(i18n("E&xtract..."), "ark_extract", 0, this,
			SLOT(action_extract()),	actionCollection(), "extract");

	deleteAction = new KAction(i18n("De&lete..."), "ark_delete", 0, this,
			SLOT(action_delete()), actionCollection(), "delete");
	
	viewAction = new KAction(i18n("to view something","&View"), "ark_view", 0, this,
			SLOT(action_view()), actionCollection(), "view");
	
	popupViewAction  = new KAction(i18n("to view something","&View"), "ark_view", 0, this,
			SLOT(action_view()), actionCollection(), "popup_menu_view");
	
	openWithAction = new KAction(i18n("&Open With..."), 0, this,
			SLOT(slotOpenWith()), actionCollection(), "open_with");
	
	popupOpenWithAction  = new KAction(i18n("&Open With..."), 0, this,
			SLOT(slotOpenWith()), actionCollection(), "popup_menu_open_with");
	
	editAction = new KAction(i18n("Edit &With..."), 0, this,
			SLOT(action_edit()), actionCollection(), "edit");

	popupEditAction = new KAction(i18n("Edit &With..."), 0, this,
			SLOT(action_edit()), actionCollection(), "popup_edit");
	
	selectAction =  new KAction(i18n("&Select..."), 0, this,
			SLOT(edit_select()),	actionCollection(), "select");
	
	selectAllAction = KStdAction::selectAll(this, 
			SLOT(edit_selectAll()),	actionCollection(), "select_all");
	
	deselectAllAction =  new KAction(i18n("&Deselect All"), 0, this,
			SLOT(edit_deselectAll()), actionCollection(), "deselect_all");
	
	invertSelectionAction = new KAction(i18n("&Invert Selection"), 0, this,
			SLOT(edit_invertSel()), actionCollection(), "invert_selection");

	toolbarAction = KStdAction::showToolbar(this, SLOT(toggleToolBar()), actionCollection());
	statusbarAction = KStdAction::showStatusbar(this, SLOT(toggleStatusBar()), actionCollection());
	KStdAction::saveOptions(this, SLOT(options_saveNow()), actionCollection());
	KStdAction::keyBindings(this, SLOT(options_keys()), actionCollection());
	KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());
	KStdAction::preferences(this, SLOT(options_dirs()), actionCollection());
	
	createGUI();
}


void 
ArkWidget::editToolbars()
{
	KEditToolbar dlg(actionCollection());
	if ( dlg.exec() )
	{
		createGUI( );
	}
}

void
ArkWidget::setHeader()
{
	// called after the QTimer goes off after toggling menu bar to hide it.
	if ( m_bIsArchiveOpen )
	{
		setCaption(m_strArchName);
	}
	else
	{
		setCaption(QString::null);
	}
}

void 
ArkWidget::toggleToolBar()
{
	if( toolbarAction->isChecked() )
	{
		toolBar("mainToolBar")->show();
	}
	else
	{
		toolBar("mainToolBar")->hide();
	}
}

void 
ArkWidget::toggleStatusBar()
{
	if ( statusbarAction->isChecked() )
	{
		statusBar()->show();
	}
	else
	{
		statusBar()->hide();
	}
}

void ArkWidget::setupStatusBar()
{
  kdDebug(1601) << "+ArkWidget::setupStatusBar" << endl;

  KStatusBar *sb = statusBar();

  QWhatsThis::add(sb, i18n("The statusbar shows you how many files you have and how many you have selected. It also shows you total sizes for these groups of files."));

  m_pStatusLabelSelect = new QLabel(sb);
  m_pStatusLabelSelect->setFrameStyle(QFrame::Panel | QFrame::Raised);
  m_pStatusLabelSelect->setAlignment(AlignLeft);
  m_pStatusLabelSelect->setText(i18n("0 Files Selected"));

  m_pStatusLabelTotal = new QLabel(sb);
  m_pStatusLabelTotal->setFrameStyle(QFrame::Panel | QFrame::Raised);
  m_pStatusLabelTotal->setAlignment(AlignRight);
  m_pStatusLabelTotal->setText(i18n("Total: 0 Files"));

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
  //recent->setEnabled(true);

  deleteAction->setEnabled(false);
  extractAction->setEnabled(false);
  addFileAction->setEnabled(false);
  addDirAction->setEnabled(false);
  openWithAction->setEnabled(false);
  editAction->setEnabled(false);
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
            m_nSizeOfFiles += pItem->text(m_currentSizeColumn).toInt();
          pItem = (FileLVI *)pItem->nextSibling();
        }
    }
  //  kdDebug(1601) << "We have " << m_nNumFiles << " elements\n" << endl;

  QString strInfo = i18n("%n File  %1", "%n Files  %1", m_nNumFiles).arg(KIO::convertSize(m_nSizeOfFiles));
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
  // Yes... again.
  enum ArchType archtype = Arch::getArchType(m_strArchName, extension, m_url);

  filter = "*" + extension;
  KURL u;
  do
    {
      u = getCreateFilename(i18n("Save Archive As"), filter, extension);
      if (u.isEmpty())
        return;
      QString ext;
      strFile = u.path();
      ArchType newArchType = Arch::getArchType(strFile, ext, u);
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
    file_open(mSaveAsURL);
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
          KMessageBox::error(this, i18n("Cannot access the archive %1").arg(strFile.local8Bit()));
        }
      else
        KMessageBox::error(this, i18n("Unknown error"));
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
          KMessageBox::error(this, i18n("You don't have permission to access that archive.") );
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
      // raise the window containing the already open archive
      ArkApplication::getInstance()->raiseArk(strFile);

      // MJ: Is this causing a wierd segfault?
      // close this window
      window_close();

      // notify the user what's going on
      KMessageBox::information(0, i18n("The archive %1 is already open and has been raised.\nNote: if the filename does not match, it only means that one of the two is a symbolic link.").arg(strFile));
      return;
    }

  // no errors if we made it this far.

  if (isArchiveOpen())
    file_close();  // close old zip

  // Set the current archive filename to the filename
  // m_url is already set
  m_strArchName = strFile;

  // display the archive contents
  showZip(strFile);

  createGUI();
  kdDebug(1601) << "-ArkWidget::file_open(const QString & strFile)" << endl;
}

//////////////////////////////////////////////////////////////////////
////////////////////////////// download //////////////////////////////
//////////////////////////////////////////////////////////////////////


bool ArkWidget::download(const KURL &url, QString &strFile)
{
  kdDebug(1601) << "+ArkWidget::download(const KURL &url, QString &strFile)" << endl;

  // downloads url into strFile, making sure strFile has the same extension
  // as url.
  if (!url.isLocalFile())
    {
      QString extension;
      Arch::getArchType(url.path(), extension);
      QString directory = locateLocal( "tmp", "ark" );
      mpTempFile = new KTempFile(directory , extension);
      strFile = url.fileName();
      kdDebug(1601) << "Downloading " << url.path() << " as $HOME/" <<
        strFile << endl;
    }
  kdDebug(1601) << "-ArkWidget::download - return KIO::NetAccess::download(url, strFile)" << endl;
  return KIO::NetAccess::download(url, strFile);
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

     //the user chose to save as the current archive
     //or wanted to create a new one with the same name
     //no need to do anything
      if (strFile == m_strArchName && m_bIsArchiveOpen)
	return QString::null;

      kdDebug(1601) << "Trying to create an archive named " <<
        strFile << endl;
      if (stat(strFile.local8Bit(), &statbuffer) != -1)  // already exists!
        {
          choice =
            KMessageBox::warningYesNoCancel(0, i18n("Archive already exists. Do you wish to overwrite it?"), i18n("Archive Already Exists"));
          if (choice == KMessageBox::Yes)
            {
              unlink(QFile::encodeName(strFile));
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
          Arch::getArchType(strFile, temp, url) == COMPRESSED_FORMAT)
        {
          KMessageBox::information(0, i18n("You need to create an archive, not a new\ncompressed file. Please try again."));
          continue;
        }

      // if we made it here, it's a go.
      if ((m_strArchName.contains('.') && !strFile.contains('.')) ||
          (m_strArchName.isNull() && !strFile.contains('.')))
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
  if (!strFile.isEmpty())
    {
        m_settings->clearShellOutput();
        file_close();
        createArchive( strFile );
    }
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
      createGUI();
      m_bIsArchiveOpen = true;
      arch = _newarch;
      QString extension;
      m_bIsSimpleCompressedFile =
        (m_archType == COMPRESSED_FORMAT);
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
      KMessageBox::error(this, i18n("ark cannot create an archive of that type.\n\n  [Hint: The filename should have an extension such as '.zip' to\n  indicate the archive type. Please see the help pages for\nmore information on supported archive formats.]"));
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
      if (download(url, strFile))
      {
          m_settings->clearShellOutput();
          recent->addURL(url);
          m_url = url;
          file_open(strFile);
      }
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
    if (download(url, strFile))
    {
        m_url = url;
        m_settings->clearShellOutput();
        kdDebug(1601) << "Recent open: " << strFile << endl;
        file_open(strFile);
    }
  }
  kdDebug(1601) << "-ArkWidget::file_open(const KURL& url)" << endl;
}

void ArkWidget::showZip( QString _filename )
{
  kdDebug(1601) << "+ArkWidget::showZip" << endl;
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
        QString dirtmp;
        QString directory("tmp.");
        dirtmp = locateLocal( "tmp", directory );

        if (_filename.left(dirtmp.length()) == dirtmp ||
            !fi.isWritable())
          {
            _newarch->setReadOnly(true);
            QApplication::restoreOverrideCursor(); // no wait cursor during a msg box (David)
            KMessageBox::information(this, i18n("This archive is read-only. If you want to save it under\na new name, go to the File menu and select Save As."));
            QApplication::setOverrideCursor( waitCursor );
          }
        setCaption( _filename );
        // createActionMenu( _flag );
        arch = _newarch;
        updateStatusTotals();
        m_bIsArchiveOpen = true;
        QString extension;
        m_bIsSimpleCompressedFile =
          (m_archType == COMPRESSED_FORMAT);

        ArkApplication::getInstance()->addOpenArk(_filename, this);
    }

  fixEnables();
  QApplication::restoreOverrideCursor();

  if(m_extractOnly)
  {
    if(_success)
    {
      int oldMode = m_settings->getExtractDirMode();
      QString oldFixedExtractDir = m_settings->getFixedExtractDir();
      m_settings->setExtractDirCfg(m_url.upURL().path(), ArkSettings::FIXED_EXTRACT_DIR);
      bool done = action_extract();
      // Extract should have started before this returns, so hopefully
      // safe.
      m_settings->setExtractDirCfg(oldFixedExtractDir, oldMode);
      // last extract dir is still set, but this is not a problem
      if(!done)
        file_quit();
    }
    else
    {
        // TODO: Error
    }
  }

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

void
ArkWidget::slotExtractDone()
{
	kdDebug(1601) << "+ArkWidget::slotExtractDone" << endl;
	QApplication::restoreOverrideCursor();

	if ( m_bViewInProgress )
	{
		m_bViewInProgress = false;
		if ( m_bEditInProgress )
		{
			kdDebug(1601) << "Edit in progress..." << endl;
			KURL::List list;
			// edit will be in progress until the KProcess terminates.
         KOpenWithDlg l( list, i18n("Edit With:"), QString::null, (QWidget*)0L );
			if ( l.exec() )
			{
				KProcess *kp = new KProcess;
				m_strFileToView = m_strFileToView.right(m_strFileToView.length() - 5 );
				*kp << l.text() << m_strFileToView;
				connect( kp, SIGNAL(processExited(KProcess *)), this, SLOT(slotEditFinished(KProcess *)) );
				if ( kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false )
				{
					KMessageBox::error(0, i18n("Trouble editing the file..."));
				}
			}
			else
			{
				m_bEditInProgress = false;
			}
		}
		else
		{
			m_pKRunPtr = new KRun( m_strFileToView );
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
		for (QStringList::Iterator it = mDragFiles.begin(); it != mDragFiles.end(); ++it)
		{
			QString URL;
			URL = m_settings->getTmpDir();
			URL += *it;
			list.append( QUriDrag::localFileToUri(URL) );
		}

		QUriDrag *d = new QUriDrag(list, archiveContent->viewport());
      m_bDropSourceIsSelf = true;
      d->dragCopy();
      m_bDropSourceIsSelf = false;
	}
	if(m_extractList != 0) delete m_extractList;
	m_extractList = 0;
	if( archiveContent ) // avoid race condition, don't do updates if application is exiting
	{
		archiveContent->setUpdatesEnabled(true);
		fixEnables();
	}

        if ( m_extractRemote )
	{
		KURL srcDirURL( m_settings->getTmpDir() + "extrtmp/" );
		KURL src;
		QString srcDir( m_settings->getTmpDir() + "extrtmp/" );
	   	QDir dir( srcDir );
	       	QStringList lst( dir.entryList() );
		lst.remove( "." );
		lst.remove( ".." );

		KURL::List srcList;
	       	for( QStringList::ConstIterator it = lst.begin(); it != lst.end() ; ++it)
                {
			src = srcDirURL;
			src.addPath( *it );
			srcList.append( src );
		}

		m_extractURL.adjustPath( 1 );

		KIO::CopyJob *job = KIO::copy( srcList, m_extractURL );
    		connect( job, SIGNAL(result(KIO::Job*)),
             		this, SLOT(slotExtractRemoteDone(KIO::Job*)) );

		m_extractRemote = false;
	}

	if(m_extractOnly)
	{
		file_quit();  // Close ark window if we are doing an Extract Here...
	}

  kdDebug(1601) << "-ArkWidget::slotExtractDone" << endl;
}


void ArkWidget::slotExtractRemoteDone(KIO::Job *job)
{
  QDir dir( m_settings->getTmpDir() + "extrtmp/" );
  dir.rmdir( dir.absPath()  );

  if ( job->error() )
    job->showErrorDialog();
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
      // is this necessary? Maybe risky. The tmp/ark.### directory
      // will be removed anyhow...
      KIO::del( *mpDownloadedList, false, false );
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

  archiveContent->setUpdatesEnabled(true);
  QApplication::setOverrideCursor( waitCursor );

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
  if (m_archType == ZOO_FORMAT || m_archType == AA_FORMAT
      || m_archType == COMPRESSED_FORMAT)
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
      closeArch();
      setCaption(QString::null);
//      setCentralWidget(0);
      ArkApplication::getInstance()->removeOpenArk(m_strArchName);
      if (mpTempFile)
        {
          kdDebug(1601) << "Removing temp file " <<
            mpTempFile->name() << endl;
          mpTempFile->unlink();
          delete mpTempFile;
          mpTempFile = NULL;
        }
      updateStatusTotals();
      updateStatusSelection();
      fixEnables();
      createGUI();
    }
  else closeArch();
  kdDebug(1601) << "-ArkWidget::file_close" << endl;
}

void ArkWidget::window_close()
{
    kdDebug(1601) << "+ArkWidget::window_close" << endl;
    file_close();
    saveProperties();
    kdDebug(1601) << "-ArkWidget::window_close" << endl;
    close();
}


bool ArkWidget::queryClose()
{
    window_close();
    return true;
}


void ArkWidget::saveProperties( KConfig* config )
{
    config->writeEntry("SMOpenedFile",m_strArchName);
    config->sync();
    kdDebug(1601) << "ArkWidget::saveProperties( KConfig* config )" << endl;
}

void ArkWidget::readProperties( KConfig* config )
{
    QString file = config->readEntry("SMOpenedFile");
    kdDebug(1601) << "ArkWidget::readProperties( KConfig* config ) file=" << file << endl;
    if ( !file.isEmpty() )
        file_open( file );
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
              if ( reg_exp.search(flvi->getFileName())==0 )
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
        viewShellOutput();
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
  m_compressedFile = flvi->getFileName();
  QString tmpdir = m_settings->getTmpDir();
  m_compressedFile = "file:" + tmpdir + "/" + m_compressedFile;
  kdDebug(1601) << "The compressed file is " << m_compressedFile << endl;
  createArchive(strFilename);
  // the file will be moved into the new archive in slotCreate.
}

// Action menu /////////////////////////////////////////////////////////

void ArkWidget::action_add()
{
  QString extension;
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
  kdDebug(1601) << "Add dir: " << m_settings->getAddDir() << endl;

  //AddDlg fileDlg(AddDlg::File, m_settings->getAddDir(), m_settings,
  //               this, "adddlg");
  KFileDialog fileDlg( m_settings->getAddDir(), QString::null, this, "adddlg", true );
  fileDlg.setMode( KFile::Mode( KFile::Files | KFile::ExistingOnly ) );
  fileDlg.setCaption(i18n("Select Files to Add"));

  if(fileDlg.exec())
  {
    KURL::List addList;
    addList = fileDlg.selectedURLs();
    mpAddList = new QStringList();
    for (KURL::List::ConstIterator it = addList.begin(); it != addList.end(); it++)
       mpAddList->append( KURL::decode_string( (*it).url() ) );

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

void 
ArkWidget::addFile(QStringList *list)
{
  if (!Utilities::diskHasSpace(m_strArchName, Utilities::getSizes(list)))
    return;

  // takes a list of KURLs.
  disableAll();
  if ( m_bEditInProgress )
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
      // if they are URLs, we have to download them, replace the URLs
      // with filenames, and remember to delete the temporaries later.
      for (QStringList::Iterator it = list->begin(); it != list->end(); ++it)
        {
          QString str = *it;
          kdDebug(1601) << "Want to add " << str<< endl;
          KURL url( toLocalFile(str) );
	  *it = url.prettyURL();

	}

      }
  arch->addFile(list);
}

void ArkWidget::action_add_dir() {

        KFileDialog addDirDlg( m_settings->getAddDir(), QString::null, this, "adddirdlg", true );
        addDirDlg.setMode( KFile::Mode( KFile::Directory ) );
        addDirDlg.setCaption(i18n("Select a Directory to Add"));
        addDirDlg.exec();

        KURL u( addDirDlg.selectedURL() );
        QString dir = KURL::decode_string( u.url(-1) );
        if ( !dir.isEmpty() ) {
            disableAll();
            u = toLocalFile(dir);
            arch->addDir( u.prettyURL() );
        }

}

KURL ArkWidget::toLocalFile( QString & str)
{
    KURL url = str;

    if(!url.isLocalFile())
        {
	    if(!mpDownloadedList)
	        mpDownloadedList = new QStringList();
	    QString tempfile = m_settings->getTmpDir();
	    tempfile += str.right(str.length() - str.findRev("/") - 1);
	    if( !KIO::NetAccess::dircopy(url, tempfile) )
               return KURL();
	    mpDownloadedList->append(tempfile);        // remember for deletion
            url = tempfile;
	}
    return url;
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
  bool bIsTar = (TAR_FORMAT == m_archType);
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
              if (re.search(strFile) != -1)
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

  disableAll();
  arch->remove(&list);
  kdDebug(1601) << "-ArkWidget::action_delete" << endl;
}

void 
ArkWidget::slotOpenWith()
{
	FileLVI *pItem = archiveContent->currentItem();
	if (pItem  != NULL )
	{
		QString name = pItem->getFileName();
		
		m_extractList = new QStringList;
		m_extractList->append(name);
		
		QString fullname;
		fullname = "file:";
		fullname += m_settings->getTmpDir();
		fullname += QString::number( getArkInstanceId() );
		fullname += "/";
		fullname += name;
		
		m_extractList = new QStringList;
		m_extractList->append(name);
		m_bOpenWithInProgress = true;
		m_strFileToView = fullname;
		if ( Utilities::diskHasSpace( m_settings->getTmpDir(), pItem->text( getSizeColumn() ).toInt() ) )
		{
			disableAll();
			prepareViewFiles( m_extractList );
		}
	}
}

bool 
ArkWidget::reportExtractFailures( const QString & _dest, QStringList *_list )
{
	// reports extract failures when Overwrite = False and the file
	// exists already in the destination directory.
	// If list is null, it means we are extracting all files.
	// Otherwise the list contains the files we are to extract.

	QString strFilename, tmp;
	struct stat statbuffer;
	bool bRedoExtract = false;
	
	QApplication::restoreOverrideCursor();
	
	Q_ASSERT(_list != NULL);
	QString strDestDir = _dest;

	// make sure the destination directory has a / at the end.
	if (strDestDir.at(0) != '/')
	{
		strDestDir += '/';
	}

	if (_list->isEmpty())
	{
		// make the list
		FileListView *flw = fileList();
		FileLVI *flvi = (FileLVI*)flw->firstChild();
		while (flvi)
		{
			tmp = flvi->getFileName();
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
		{
			existingFiles.append(strFilename);
		}
	}
	
	int numFilesToReport = existingFiles.count();
	
	kdDebug(1601) << "There are " << numFilesToReport << " files to report existing already." << endl;
	
	// now report on the contents
  	if (numFilesToReport == 1)
	{
		kdDebug(1601) << "One to report" << endl;
		strFilename = *(existingFiles.at(0));
      QString message = 
			i18n("%1 will not be extracted because it will overwrite an existing file.\nGo back to Extract Dialog?").arg(strFilename);
      bRedoExtract =	KMessageBox::questionYesNo(this, message) == KMessageBox::Yes;
	}
	else if (numFilesToReport != 0)
	{
		ExtractFailureDlg *fDlg = new ExtractFailureDlg( &existingFiles, this );
		bRedoExtract = !fDlg->exec();
	}
	return bRedoExtract;
}

bool 
ArkWidget::action_extract()
{
	kdDebug(1601) << "+action_extract" << endl;
	//before we start, make sure the archive is still there
	if (!KIO::NetAccess::exists(KURL(arch->fileName()))){
		KMessageBox::error(0,
			i18n("The archive to extract from no longer exists."));
		file_quit();
		return false;
	}
	
	ExtractDlg *dlg = new ExtractDlg(m_settings);
	
	// if they choose pattern, we have to tell arkwidget to select
	// those files... once we're in the dialog code it's too late.
	connect( dlg, SIGNAL( pattern( const QString & ) ), this, SLOT( selectByPattern( const QString & ) ) );
	bool bRedoExtract = false;

	if (m_nNumSelectedFiles == 0)
	{
		dlg->disableSelectedFilesOption();
	}
	if (archiveContent->currentItem() == NULL)
	{
		dlg->disableCurrentFileOption();
	}
	
	// list of files to be extracted
  	m_extractList = new QStringList;
	if ( dlg->exec() )
	{
		int extractOp = dlg->extractOp();
		kdDebug(1601) << "Extract op: " << extractOp << endl;
		
		//m_extractURL will always be the location the user chose to
		//m_extract to, whether local or remote
		m_extractURL = dlg->extractDir();
				
		//extractDir will either be the real, local extract dir,
		//or in case of a extract to remote location, a local tmp dir
		QString extractDir;
		
		if ( !m_extractURL.isLocalFile() )
		{
			extractDir = m_settings->getTmpDir() + "extrtmp/";
			m_extractRemote = true;
			//make sure it's empty since all of it's contents
			//will be copied to the remote extract location
			KIO::NetAccess::del( extractDir );
			if ( !KIO::NetAccess::mkdir( extractDir ) )
			{
				kdWarning(1601) << "Unable to create " << extractDir << endl;
				m_extractRemote = false;
				delete dlg;
				return false;
			}
		}
		else
		{
			extractDir = m_extractURL.path();
		}
		
		// if overwrite is false, then we need to check for failure of
		// extractions.
		bool bOvwrt = m_settings->getExtractOverwrite();
		
		switch(extractOp)
		{
			case ExtractDlg::All:
				if (!bOvwrt)  // send empty list to indicate we're extracting all
				{
					bRedoExtract = reportExtractFailures(extractDir, m_extractList);
				}

				if (!bRedoExtract) // if the user's OK with those failures, go ahead
				{
					// unless we have no space!
					if (Utilities::diskHasSpace(extractDir, m_nSizeOfFiles))
					{
						disableAll();
						arch->unarchFile(0, extractDir);
					}
				}
				break;
			case ExtractDlg::Pattern:
			case ExtractDlg::Selected:
			case ExtractDlg::Current:
				{
					int nTotalSize = 0;
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
								nTotalSize += flvi->text(getSizeColumn()).toInt();
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
							return true;
						}
						QString tmp = pItem->getFileName();  // no text(0)
						nTotalSize += pItem->text(getSizeColumn()).toInt();
						m_extractList->append( QFile::encodeName(tmp) );
					}
					if (!bOvwrt)
					{
						bRedoExtract = reportExtractFailures(extractDir, m_extractList);
					}
					if (!bRedoExtract)
					{
						if (Utilities::diskHasSpace(extractDir, nTotalSize))
						{
							disableAll();
							arch->unarchFile(m_extractList, extractDir); // extract selected files
						}
					}
					break;
				}
			default:
				Q_ASSERT(0);
				// never happens
				break;
		}

		delete dlg;
	}
	else
	{
		return false;
	}

	// user might want to change some options or the selection...
	if (bRedoExtract)
	{
		return action_extract();
	}
	
	return true;
}

void 
ArkWidget::action_edit()
{
	// begin an edit. This is like a view, but once the process exits,
	// the file is put back into the archive. If the user tries to quit or
	// close the archive, there will be a warning that any changes to the
	// files open under "Edit" will be lost unless the archive remains open.
	// [hmm, does that really make sense? I'll leave it for now.]

	m_bEditInProgress = true;
	action_view();
}

void 
ArkWidget::action_view()
{
	FileLVI *pItem = archiveContent->currentItem();
	m_bViewInProgress = true;
	if (pItem  != NULL )
	{
		showFile(pItem);
	}
}

void 
ArkWidget::showFile( FileLVI *_pItem )
{
	QString name = _pItem->getFileName(); // no text(0)
	
	QString fullname;
	fullname = "file:";
	fullname += m_settings->getTmpDir();
	fullname += QString::number( getArkInstanceId() );
	fullname += "/";
	fullname += name;

	kdDebug(1601) << "File to be viewed: " << fullname << endl;
	
	m_extractList = new QStringList;
	m_extractList->append(name);
	
	m_strFileToView = fullname;
	if (Utilities::diskHasSpace( m_settings->getTmpDir(),	_pItem->text( getSizeColumn() ).toLong() ) )
	{
		disableAll();
		prepareViewFiles( m_extractList );
	}
}

// Options menu //////////////////////////////////////////////////////

void 
ArkWidget::options_dirs()
{
	GeneralOptDlg *dd = new GeneralOptDlg( m_settings, this );
	dd->exec();
	delete dd;
}

void 
ArkWidget::options_keys()
{
	KKeyDialog::configureKeys(actionCollection(), xmlFile());
}

void 
ArkWidget::options_saveNow()
{
	m_settings->writeConfigurationNow();
}

// Popup /////////////////////////////////////////////////////////////


void 
ArkWidget::doPopup( QListViewItem *pItem, const QPoint &pPoint, int nCol ) // slot
// do the right-click popup menus
{
	kdDebug(1601) << "+ArkWidget::doPopup" << endl;
	if (nCol == 0)
	{
		archiveContent->setCurrentItem(pItem);
		archiveContent->setSelected(pItem, true);
		QWidget* filePopup = factory()->container("file_popup", this);
		if ( !filePopup )
		{
			kdError() << "No file_popup container !!!" << endl;
		}
		else if ( !filePopup->inherits("QPopupMenu") )
		{
			kdError() << "file_popup is a " << filePopup->className() << endl;
		}
		else
		{
			static_cast<QPopupMenu *>(factory()->container("file_popup", this))->popup(pPoint);
		}
	}
	else // clicked anywhere else but the name column
	{
		//m_archivePopup->popup(pPoint);

		((QPopupMenu *)factory()->container("archive_popup", this))->popup(pPoint);
	}
	kdDebug(1601) << "-ArkWidget::doPopup" << endl;
}

// Service functions /////////////////////////////////////////////////

void 
ArkWidget::slotSelectionChanged()
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
                  flvi->text(m_currentSizeColumn).toInt();
            }
          flvi = (FileLVI*)flvi->itemBelow();
        }
    }
  QString strInfo;
  if (m_nNumSelectedFiles == 0)
    {
      strInfo = i18n("0 Files Selected");
    }
  else if (m_nNumSelectedFiles != 1)
    {
      strInfo = i18n("%1 Files Selected  %2")
        .arg(KGlobal::locale()->formatNumber(m_nNumSelectedFiles, 0))
        .arg(KIO::convertSize(m_nSizeOfSelectedFiles));
    }
  else
    {
    strInfo = i18n("1 File Selected  %2")
        .arg(KIO::convertSize(m_nSizeOfSelectedFiles));
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
      if (glob->search(flvi->getFileName()) != -1)
        archiveContent->setSelected(flvi, true);
      flvi = (FileLVI*)flvi->itemBelow();
    }

  delete glob;
}

// Drag & Drop ////////////////////////////////////////////////////////

#if 0 // not sure I need this
void ArkWidget::dragEnterEvent(QDragEnterEvent* event)
{
  event->accept(QUriDrag::canDecode(event));
}

#endif

void ArkWidget::dragMoveEvent(QDragMoveEvent *e)
{
  if (QUriDrag::canDecode(e) &&
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

  if (QUriDrag::decodeToUnicodeUris(e, *mpAddList))
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
  if (1 == list->count() &&  (UNKNOWN_FORMAT != Arch::getArchType(str, extension)))
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
      if ( (Arch::getArchType(name, extension)!=-1) || (isDirectory) )
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

  createGUI();

  //    writeStatusMsg( i18n( "Archive Directory") );
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
  kdDebug() << "ArkWidget::createFileListView" << endl;
  //delete archiveContent;
  if ( !archiveContent )
  {
    archiveContent = new FileListView(this, this);
    archiveContent->setMultiSelection(true);
    setCentralWidget(archiveContent);
    archiveContent->show();
    connect( archiveContent, SIGNAL( selectionChanged()),
           this, SLOT( slotSelectionChanged() ) );
    connect(archiveContent,
          SIGNAL(rightButtonPressed(QListViewItem *,
                                    const QPoint &, int)),
          this, SLOT(doPopup(QListViewItem *,
                             const QPoint &, int)));
  }
  archiveContent->clear();
}

//////////////////////////////////////////////////////////////////////
//////////////////////// badBzipName /////////////////////////////////
//////////////////////////////////////////////////////////////////////

bool
ArkWidget::badBzipName(const QString & _filename)
{
	if ( _filename.right(3) == ".BZ" || _filename.right(4) == ".TBZ" )
	{
		KMessageBox::error( this,
				i18n("bzip does not support filename extensions that use capital letters.") );
	}
	else if ( _filename.right(4) == ".tbz" )
	{
		KMessageBox::error(this, i18n("bzip only supports filenames with the extension \".bz\"."));
	}
	else if (_filename.right(4) == ".BZ2" ||  _filename.right(5) == ".TBZ2")
	{
		KMessageBox::error(this,
				i18n("bzip2 does not support filename extensions that use capital letters."));
	}
	else if ( _filename.right(5) == ".tbz2" )
	{
		KMessageBox::error(this,
				i18n("bzip2 only supports filenames with the extension \".bz2\".") );
	}
	else
	{
		return false;
	}

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

  ArchType archtype = Arch::getArchType(_filename, extension);
  if(0 == (newArch = Arch::archFactory(archtype, m_settings, this,
           _filename)))
    {
      if (!badBzipName(_filename))
        KMessageBox::error(this, i18n("Unknown archive format or corrupted archive") );
      return;
    }

  if (!newArch->utilityIsAvailable())
    {
      KMessageBox::error(this, i18n("The utility %1 is not in your PATH.\nPlease install it or contact your system administrator.").arg(newArch->getUtility()));
      return;
    }

  m_archType = archtype;

  connect( newArch, SIGNAL(sigCreate(Arch *, bool, const QString &, int)),
           this, SLOT(slotCreate(Arch *, bool, const QString &, int)) );
  connect( newArch, SIGNAL(sigDelete(bool)), this, SLOT(slotDeleteDone(bool)));
  connect( newArch, SIGNAL(sigAdd(bool)),
           this, SLOT(slotAddDone(bool)));
  connect( newArch, SIGNAL(sigExtract(bool)),
           this, SLOT(slotExtractDone()));

  archiveContent->setUpdatesEnabled(true);
  QApplication::setOverrideCursor( waitCursor );
  newArch->create();
  recent->addURL(_filename);
}

//////////////////////////////////////////////////////////////////////
//////////////////////// openArchive /////////////////////////////////
//////////////////////////////////////////////////////////////////////

void
ArkWidget::openArchive( const QString & _filename )
{
	QString extension;
	Arch *newArch = 0;
	ArchType archtype = Arch::getArchType( _filename, extension, m_url );

	if( 0 == ( newArch = Arch::archFactory( archtype, m_settings, this, _filename ) ) )
	{
		kdDebug( 1601 ) << "BadBZip name test..." << endl;
		if ( !badBzipName( _filename ) )
		{
			// it's still a bad name, so let's try to figure out what it is.
			// Maybe it just needs the proper extension!
			KMimeMagic *mimePtr = KMimeMagic::self();
			KMimeMagicResult * mimeResultPtr = mimePtr->findFileType(_filename);
			QString mimetype = mimeResultPtr->mimeType();
			if (mimetype == "application/x-gzip")
			{
				KMessageBox::error(this, i18n("Gzip archives need to have the extension `gz'."));
				return;
			}
			else if (mimetype == "application/x-zoo")
			{
				KMessageBox::error(this, i18n("Zoo archives need to have the extension `zoo'."));
				return;
			}
			else
			{
				KMessageBox::error( this, i18n("Unknown archive format or corrupted archive") );
				// and just leave the old archive displayed
				return;
			}
		}
		else
		{
			return;
		}
	}

	if (!newArch->utilityIsAvailable())
	{
		KMessageBox::error(this, i18n("The utility %1 is not in your PATH.\nPlease install it or contact your system administrator.").arg(newArch->getUtility()));
		return;
	}

	m_archType = archtype;

	connect( newArch, SIGNAL(sigOpen(Arch *, bool, const QString &, int)),
			this, SLOT(slotOpen(Arch *, bool, const QString &,int)) );
	connect( newArch, SIGNAL(sigDelete(bool)),
			this, SLOT(slotDeleteDone(bool)));
	connect( newArch, SIGNAL(sigAdd(bool)),
			this, SLOT(slotAddDone(bool)));
	connect( newArch, SIGNAL(sigExtract(bool)),
			this, SLOT(slotExtractDone()));

	disableAll();
	newArch->open();
}

#include "arkwidget.moc"
