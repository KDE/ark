#include <kdebug.h>
#include <kurl.h>
#include <kfiledialog.h>
#include <qvbox.h>
#include <qmessagebox.h>
#include <qcheckbox.h>
#include <qfileinfo.h>
#include <klocale.h>
#include <qbuttongroup.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qlineedit.h>
#include "arksettings.h"
#include "extractdlg.h"

ExtractDlg::ExtractDlg(ArchType _archtype, ArkSettings *_settings)
  : QTabDialog(0, "extractdialog", true), m_settings(_settings),
    m_archtype(_archtype)
{
  setCaption(i18n("ark - Extract"));

  setupFirstTab();
  setupSecondTab();

  setOKButton();
  setCancelButton();

  connect(m_patternLE, SIGNAL(textChanged(const QString &)),
	  this, SLOT(choosePattern()));

  connect(m_extractDirLE, SIGNAL(returnPressed()), this, SLOT(accept()));
  connect(m_patternLE, SIGNAL(returnPressed()), this, SLOT(accept()));
}

void ExtractDlg::setupFirstTab()
{
  QVBox *firstpage = new QVBox( this );
  firstpage->setMargin( 5 );

  QLabel *extractToLabel = new QLabel(firstpage);
  extractToLabel->setText(i18n("Extract to: "));
  m_extractDirLE = new QLineEdit(firstpage);
  m_extractDirLE->setText(m_settings->getExtractDir());

  QPushButton *browseButton = new QPushButton(firstpage);
  browseButton->setText(i18n("Browse..."));

  QButtonGroup *bg = new QButtonGroup( 1, QGroupBox::Horizontal,
				       i18n("Files to Extract"), firstpage );
  m_radioCurrent = new QRadioButton("Current", bg);
  m_radioCurrent->setText(i18n("Current"));
  m_radioAll = new QRadioButton("All", bg);
  m_radioAll->setText(i18n("All"));
  m_radioSelected = new QRadioButton("Selected Files", bg);
  m_radioSelected->setText(i18n("Selected Files"));
  m_radioSelected->setChecked(true);
  m_radioPattern = new QRadioButton("By Pattern", bg);
  m_radioPattern->setText(i18n("Pattern"));

  QLabel *patternLabel = new QLabel(firstpage, "label");
  patternLabel->setText(i18n("Pattern:"));

  m_patternLE = new QLineEdit(firstpage, "le");

  addTab(firstpage, i18n("Extract"));

  QObject::connect(browseButton, SIGNAL(clicked()),
		   this, SLOT(browse()));
}

void ExtractDlg::setupSecondTab()
{

  QVBox *secondpage = new QVBox( this );
  secondpage->setMargin( 5 );

  // use m_archtype to determine what goes here... 
  // these are the advanced options

  switch(m_archtype)
    {
    case ZIP_FORMAT:
      {
	QButtonGroup *bg = new QButtonGroup( 1, QGroupBox::Horizontal,
					     i18n("ZIP Options"), secondpage );
	m_cbOverwrite = new QCheckBox(i18n("Overwrite files"), bg);
	if (m_settings->getZipExtractOverwrite())
	  m_cbOverwrite->setChecked(true);
	m_cbPreservePerms = new QCheckBox(i18n("Preserve permissions"), bg);
	m_cbToLower = new QCheckBox(i18n("Convert filenames to lowercase"),
				    bg);
	if (m_settings->getZipExtractLowerCase())
	  m_cbToLower->setChecked(true);
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
  
  addTab(secondpage, i18n("Advanced"));
}


void ExtractDlg::accept()
{
  //  kDebugInfo( 1601, "+ExtractDlg::accept");
  if (! QFileInfo(m_extractDirLE->text()).isDir())
  {
    QMessageBox::warning(this, i18n("Error"),
			   i18n("Please provide a valid directory"));
    return;
  }

  // you need to change the settings to change the fixed dir.
  m_settings->setLastExtractDir(m_extractDirLE->text());

  // save settings

  switch(m_archtype)
    {
    case ZIP_FORMAT:
      {
	m_settings->setZipExtractOverwrite(m_cbOverwrite->isChecked());
	m_settings->setZipExtractLowerCase(m_cbToLower->isChecked());
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

  if (m_radioPattern->isChecked())
  {
    if (strcmp(m_patternLE->text(), "") == 0)
    {
      // pattern selected but no pattern? Ask user to select a pattern.
      QMessageBox::warning(this, i18n("Error"),
			   i18n("Please provide a pattern"));
      return;
    }
    else
      {
	emit pattern(m_patternLE->text());
      }
  }

  // I made it! so nothing's wrong.
  QTabDialog::accept();
  kDebugInfo( 1601, "-ExtractDlg::accept");
}


void ExtractDlg::browse() // slot
{
  QString dirName
    = KFileDialog::getExistingDirectory(m_settings->getExtractDir(), 0,
					i18n("Select an Extract Directory"));
  if (! dirName.isEmpty())
  {
    m_extractDirLE->setText(dirName);
  }
}




int ExtractDlg::extractOp()
{
// which kind of extraction shall we do?

  if (m_radioCurrent->isChecked())
    return ExtractDlg::Current;
  if(m_radioAll->isChecked())
    return ExtractDlg::All;
  if(m_radioSelected->isChecked())
    return ExtractDlg::Selected;
  if(m_radioPattern->isChecked())
    return ExtractDlg::Pattern;
  return -1;
}



#include "extractdlg.moc"
