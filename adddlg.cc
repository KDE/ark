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

AddDlg::AddDlg(ArchType _archtype, const QString & _sourceDir,
	       QWidget *parent, const char *name)
  : KDialogBase(KDialogBase::Tabbed, i18n("Add"),
		KDialogBase::Ok | KDialogBase::Cancel,
		KDialogBase::Ok, parent, name), m_sourceDir(_sourceDir),
    m_cbRecurse(0), m_cbJunkDirNames(0), m_cbForceMS(0), m_cbConvertLF2CRLF(0)
{
  setupFirstTab();
  setupSecondTab(_archtype);

  showButtonOK(true);
  showButtonCancel(true);
}

void AddDlg::setupFirstTab()
{
  // set up a grid

  QFrame *parent = addPage(i18n("Add"));

  QVBox *firstpage = new QVBox(parent);
  firstpage->setMargin( 5 );

  KFileWidget *dirList = new KFileWidget(KFileWidget::Simple,
					 firstpage, "dirlist");
  
  KURL url = QString("home/emilye/arks");
  dirList->setURL(url);
}

void AddDlg::setupSecondTab(ArchType _archtype)
{

  QFrame *parent = addPage(i18n("Advanced"));

  QVBox *secondpage = new QVBox(parent);
  secondpage->setMargin( 5 );

  // use __archtype to determine what goes here... 
  // these are the advanced options

  switch(_archtype)
    {
    case ZIP_FORMAT:
      {
	QButtonGroup *bg = new QButtonGroup( 1, QGroupBox::Horizontal,
					     "ZIP Options", secondpage );
	m_cbRecurse = new QCheckBox(i18n("Recurse into directories"), bg);
	m_cbJunkDirNames = new QCheckBox(i18n("Junk directory names"), bg);
	m_cbForceMS = new QCheckBox(i18n("Force MS-style (8+3) filenames"),
				    bg);
	m_cbConvertLF2CRLF = new QCheckBox(i18n("Convert LF to CRLF"), bg);
      }
      break;
    case TAR_FORMAT:
      break;
    default:
      // shouldn't ever get here!
      break;
    }
 
}

#include "adddlg.moc"
