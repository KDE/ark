#include <kdebug.h>
#include <qstring.h>
#include <kurl.h>
#include <kfiledialog.h>
#include <qvbox.h>
#include <qmessagebox.h>
#include <qcheckbox.h>
#include <qfileinfo.h>
#include <klocale.h>
#include <kfilewidget.h>
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
  kdebug(0, 1601, "+AddDlg::setupFirstTab");

  QFrame *frame = addPage(i18n("Add"));
  QVBoxLayout *vlay = new QVBoxLayout(frame, 0, spacingHint());

  m_dirList = new KDirOperator(m_sourceDir, frame, "dirlist");

  m_dirList->setView(KDirOperator::Simple, true);

  m_dirList->setGeometry(x(), y(), 500, 500);  // this doesn't do a thing
  vlay->addWidget(m_dirList);  
}

void AddDlg::setupSecondTab()
{
  QHBox *secondpage = addHBoxPage(i18n("Advanced"), i18n("Test"));

  switch(m_archtype)
    {
    case ZIP_FORMAT:
      {
	QButtonGroup *bg = new QButtonGroup( 1, QGroupBox::Horizontal,
					     "ZIP Options", secondpage );
	m_cbRecurse = new QCheckBox(i18n("Recurse into directories"), bg);
	if (m_settings->getZipAddRecurseDirs())
	  m_cbRecurse->setChecked(true);
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
      }
      break;
    case TAR_FORMAT:
      break;
    case AA_FORMAT:
    case LHA_FORMAT:
    case RAR_FORMAT:
    case ZOO_FORMAT:
    case UNKNOWN_FORMAT:
      break;
    default:
      // shouldn't ever get here!
      break;
    }
 
}

void AddDlg::accept()
{
  kdebug(0, 1601, "+AddDlg::accept");

  // Put the settings data into the settings object

  switch(m_archtype)
    {
    case ZIP_FORMAT:
      {
	m_settings->setZipAddRecurseDirs(m_cbRecurse->isChecked());
	m_settings->setZipAddJunkDirs(m_cbJunkDirNames->isChecked());
	m_settings->setZipAddMSDOS(m_cbForceMS->isChecked());
	m_settings->setZipAddConvertLF(m_cbConvertLF2CRLF->isChecked());
      }
    case AA_FORMAT:
    case LHA_FORMAT:
    case RAR_FORMAT:
    case ZOO_FORMAT:
    case UNKNOWN_FORMAT:
      break;
    default:
      break;
    }

  const KFileView *pView = m_dirList->view();
  KFileViewItemList *pList = pView->selectedItems();

  kdebug(0, 1601, "There are %d items in my KFileViewItemList.",
	 pList->count());

  m_fileList = new QStringList;

  KFileViewItem *pItem;
  for ( pItem=pList->first(); pItem != 0; pItem=pList->next() )
    {
      kdebug(0, 1601, "%s", (const char *)pItem->url());
      m_fileList->append(pItem->url());
    }

  KDialogBase::accept();
  kdebug(0, 1601, "-AddDlg::accept");
}

#include "adddlg.moc"
