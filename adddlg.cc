/*

 $Id$ 

 ark -- archiver for the KDE project

 Copyright (C)

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
#include <kdebug.h>
#include <qstring.h>
#include <kurl.h>
#include <kdiroperator.h>
#include <kfile.h>
#include <qvbox.h>
#include <qmessagebox.h>
#include <qcheckbox.h>
#include <qfileinfo.h>
#include <qlayout.h>
#include <klocale.h>
#include <kcombiview.h>
#include <kfileiconview.h>
#include <qbuttongroup.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qlineedit.h>
#include <kdialogbase.h>
#include "adddlg.h"
#include "arksettings.h"

AddDlg::AddDlg(ArchType _archtype, const QString & _sourceDir,
	       ArkSettings *_settings,
	       QWidget *parent, const char *name)
  : KDialogBase(KDialogBase::Tabbed, i18n("Add"),
		KDialogBase::Ok | KDialogBase::Cancel,
		KDialogBase::Ok, parent, name), m_sourceDir(_sourceDir),
    m_archtype(_archtype), m_settings(_settings), m_fileList(0)
{
  // this has some sizing problems
  setupFirstTab();
  setupSecondTab();
  showButtonOK(true);
  showButtonCancel(true);
}

void AddDlg::setupFirstTab()
{
  kdDebug(1601) << "+AddDlg::setupFirstTab" << endl;

  QFrame *frame = addPage(i18n("Add File(s)"));
  m_dirList = new KDirOperator(m_sourceDir, frame, "dirlist");
  KCombiView *pCombiView = new KCombiView(m_dirList, "fileview");
  KFileIconView *pFileView = new KFileIconView(pCombiView, "fileview2");
  pCombiView->setRight(pFileView);
  pFileView->setSelectionMode( KFile::Multi ); 
  m_dirList->setView(pCombiView);

  QVBoxLayout *vlay = new QVBoxLayout(frame, 0, spacingHint());
  vlay->addWidget(m_dirList);  
  
}

void AddDlg::setupSecondTab()
{
  QHBox *secondpage = addHBoxPage(i18n("Advanced"), i18n("Test"));
  QButtonGroup *bg = new QButtonGroup( 1, QGroupBox::Horizontal,
				       "", secondpage );
  m_cbReplaceOnlyWithNewer =
    new QCheckBox(i18n("Replace old files only with newer files"), bg);

  switch(m_archtype)
    {
    case ZIP_FORMAT:
      kdDebug(1601) << "AddDlg::setupSecondTab - zip format" << endl;
      bg->setTitle(i18n("ZIP Options"));
      if (m_settings->getZipReplaceOnlyWithNewer())
	m_cbReplaceOnlyWithNewer->setChecked(true);
      
      m_cbRecurseSubdirs = new QCheckBox(i18n("Recurse into subdirectories"),
					 bg);
      if (m_settings->getZipAddRecurseDirs())
	m_cbRecurseSubdirs->setChecked(true);
      
      
      m_cbJunkDirNames = new QCheckBox(i18n("Junk directory names"), bg);
      if (m_settings->getZipAddJunkDirs())
	m_cbJunkDirNames->setChecked(true);
      
      
      m_cbForceMS = new QCheckBox(i18n("Force MS-style (8+3) filenames"),
				  bg);
      if (m_settings->getZipAddMSDOS())
	m_cbForceMS->setChecked(true);
      
      m_cbConvertLF2CRLF = new QCheckBox(i18n("Convert LF to CRLF"), bg);
      if (m_settings->getZipAddConvertLF())
	m_cbConvertLF2CRLF->setChecked(true);
      
      m_cbStoreSymlinks = new QCheckBox(i18n("Store symlinks as such"), bg);
      if (m_settings->getZipStoreSymlinks())
	m_cbStoreSymlinks->setChecked(true);
      break;
    case TAR_FORMAT:
      bg->setTitle(i18n("TAR Options"));
      m_cbJunkDirNames = new QCheckBox(i18n("Junk directory names"), bg);
      if (!m_settings->getaddPath())
	m_cbJunkDirNames->setChecked(true);
      if (m_settings->getTarReplaceOnlyWithNewer())
	m_cbReplaceOnlyWithNewer->setChecked(true);
#if 0 // too dangerous? I'm omitting but feeling wafflish.
      m_cbAbsPathNames =
	new QCheckBox(i18n("Use absolute pathnames"), bg);
      if (m_settings->getTarUseAbsPathnames())
	m_cbAbsPathNames->setChecked(true);
#endif
      break;
    case AA_FORMAT:
      bg->setTitle(i18n("AR Options"));
      if (m_settings->getArReplaceOnlyWithNewer())
	m_cbReplaceOnlyWithNewer->setChecked(true);
      break;
    case LHA_FORMAT:
      bg->setTitle(i18n("LHA Options"));
      if (m_settings->getLhaReplaceOnlyWithNewer())
	m_cbReplaceOnlyWithNewer->setChecked(true);
      m_cbMakeGeneric =  new QCheckBox(i18n("Keep entries generic"), bg);
      if (m_settings->getLhaGeneric())
	m_cbMakeGeneric->setChecked(true);
      break;
    case RAR_FORMAT:
      bg->setTitle(i18n("RAR Options"));
      m_cbStoreSymlinks = new QCheckBox(i18n("Store symlinks as such"), bg);
      if (m_settings->getRarStoreSymlinks())
	m_cbStoreSymlinks->setChecked(true);
      if (m_settings->getRarReplaceOnlyWithNewer())
	m_cbReplaceOnlyWithNewer->setChecked(true);
      m_cbRecurseSubdirs = new QCheckBox(i18n("Recurse into subdirectories"),
					 bg);
      if (m_settings->getRarRecurseSubdirs())
	m_cbRecurseSubdirs->setChecked(true);
      break;
    case ZOO_FORMAT:
      if (m_settings->getZooReplaceOnlyWithNewer())
	m_cbReplaceOnlyWithNewer->setChecked(true);
      break;
    case UNKNOWN_FORMAT:
      break;
    default:
      // shouldn't ever get here!
      break;
    }
 
}

void AddDlg::accept()
{
  kdDebug(1601) << "+AddDlg::accept" << endl;

  // Put the settings data into the settings object

  switch(m_archtype)
    {
    case ZIP_FORMAT:
      m_settings->setZipAddRecurseDirs(m_cbRecurseSubdirs->isChecked());
      m_settings->setZipAddJunkDirs(m_cbJunkDirNames->isChecked());
      m_settings->setZipAddMSDOS(m_cbForceMS->isChecked());
      m_settings->setZipAddConvertLF(m_cbConvertLF2CRLF->isChecked());
      m_settings->setZipStoreSymlinks(m_cbStoreSymlinks->isChecked());
      m_settings->setZipReplaceOnlyWithNewer(m_cbReplaceOnlyWithNewer->isChecked());
      break;
    case TAR_FORMAT:
      m_settings->setTarReplaceOnlyWithNewer(m_cbReplaceOnlyWithNewer->isChecked());
      m_settings->setaddPath(!m_cbJunkDirNames->isChecked());
      //      m_settings->setTarUseAbsPathnames(m_cbAbsPathNames->isChecked());
      break;
    case AA_FORMAT:
      m_settings->setArReplaceOnlyWithNewer(m_cbReplaceOnlyWithNewer->isChecked());
      break;
    case LHA_FORMAT:
      m_settings->setLhaGeneric(m_cbMakeGeneric->isChecked());
      m_settings->setLhaReplaceOnlyWithNewer(m_cbReplaceOnlyWithNewer->isChecked());
      break;
    case RAR_FORMAT:
      m_settings->setRarStoreSymlinks(m_cbStoreSymlinks->isChecked());
      m_settings->setRarReplaceOnlyWithNewer(m_cbReplaceOnlyWithNewer->isChecked());
      m_settings->setRarRecurseSubdirs(m_cbRecurseSubdirs->isChecked());
      break;
    case ZOO_FORMAT:
      m_settings->setZooReplaceOnlyWithNewer(m_cbReplaceOnlyWithNewer->isChecked());
      break;
    case UNKNOWN_FORMAT:
      break;
    default:
      break;
    }

  const KFileView *pView = m_dirList->view();
  KFileViewItemList *pList =
    const_cast<KFileViewItemList *>(pView->selectedItems());

  kdDebug(1601) << "There are " << pList->count() << " items in my KFileViewItemList." << endl;

  m_fileList = new QStringList;

  KFileViewItem *pItem;
  for ( pItem=pList->first(); pItem != 0; pItem=pList->next() )
    {
      kdDebug(1601) << (const char *)pItem->url() << endl;
      m_fileList->append(pItem->url());
    }

  KDialogBase::accept();
  kdDebug(1601) << "-AddDlg::accept" << endl;
}

#include "adddlg.moc"
