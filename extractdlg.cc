/*

 $Id$

 ark -- archiver for the KDE project
 
 Copyright (C)
 
 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999-2000: Corel Corporation (author: Emily Ezust emilye@corel.com)
 2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
 
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

#include <qvbox.h>
#include <qcheckbox.h>
#include <qfileinfo.h>
#include <qbuttongroup.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qapp.h>
#include <qframe.h>

#include <kdebug.h>
#include <kurl.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdialogbase.h>

#include "arksettings.h"
#include "generalOptDlg.h"
#include "extractdlg.h"

#define FIRST_PAGE_WIDTH  390
#define DLG_NAME i18n("Extract")

ExtractDlg::ExtractDlg(ArchType _archtype, ArkSettings *_settings) :
	KDialogBase(KDialogBase::Plain, DLG_NAME, Ok | Cancel, Ok),
	m_settings(_settings), m_archtype(_archtype)
{
  QFrame *mainFrame = plainPage();

  QLabel *extractToLabel = new QLabel(mainFrame);
  extractToLabel->setText(i18n("Extract to: "));
  extractToLabel->setGeometry( 10, 10,
			       extractToLabel->sizeHint().width(), 15 );

  m_extractDirCB = new QComboBox(true, mainFrame);
  m_extractDirCB->insertItem(m_settings->getExtractDir());
  m_extractDirCB->setGeometry( 10, 30, 368, 20 );

  QPushButton *browseButton = new QPushButton(mainFrame);
  browseButton->setText(i18n("Browse..."));
  int x = browseButton->sizeHint().width();
  browseButton->setGeometry( FIRST_PAGE_WIDTH-10-x, 55, x, 30 );

  QLabel *lToExtract = new QLabel(mainFrame);
  lToExtract->setText(i18n("Files to be extracted"));
  int y = lToExtract->sizeHint().width();
  lToExtract->setGeometry( 10, 92, y, 15 );

  QLabel *lHorizLine = new QLabel(mainFrame, "horizontal line");
  lHorizLine->setGeometry( y+15, 100, FIRST_PAGE_WIDTH-25-y, 1 );
  lHorizLine->setFrameStyle( 52 );
  lHorizLine->setLineWidth( 1 );

  QButtonGroup *bg = new QButtonGroup(mainFrame);
  bg->setFrameShape(QFrame::NoFrame);
  bg->setGeometry(30, 120, 200, 80);

  m_radioCurrent = new QRadioButton(bg);
  m_radioCurrent->setText(i18n("Current"));
  m_radioCurrent->setGeometry( 0, 0, m_radioCurrent->sizeHint().width(), 15 );

  m_radioAll = new QRadioButton(bg);
  m_radioAll->setText(i18n("All"));
  m_radioAll->setGeometry(0, 20, m_radioAll->sizeHint().width(), 15);

  m_radioSelected = new QRadioButton(bg);
  m_radioSelected->setText(i18n("Selected Files"));
  m_radioSelected->setChecked(true);
  m_radioSelected->setGeometry(0, 40, m_radioSelected->sizeHint().width(), 15);

  m_radioPattern = new QRadioButton(bg);
  m_radioPattern->setText(i18n("Pattern"));
  m_radioPattern->setGeometry(0, 60, m_radioPattern->sizeHint().width(), 15);

  m_patternLE = new QLineEdit(mainFrame, "le");
  m_patternLE->setGeometry( 50, 200, 250, 20 );

  QPushButton *prefButton = new QPushButton(i18n("&Preferences..."), this);
  prefButton->move(10, 250);

  mainFrame->setMinimumSize(410,250);

  connect(m_patternLE, SIGNAL(textChanged(const QString &)),
	  this, SLOT(choosePattern()));
  connect(m_patternLE, SIGNAL(returnPressed()), this, SLOT(accept()));
  connect(prefButton, SIGNAL(clicked()), this, SLOT(openPrefs()));

  connect(browseButton, SIGNAL(clicked()), this, SLOT(browse()));

}

void ExtractDlg::disableSelectedFilesOption()
{
  m_radioSelected->setEnabled(false); 
  m_radioAll->setChecked(true); 
}


void ExtractDlg::accept()
{
  kdDebug(1601) << "+ExtractDlg::accept" << endl;
  if (! QFileInfo(m_extractDirCB->currentText()).isDir())
  {
    KMessageBox::error(this,
			   i18n("Please provide a valid directory"));
    return;
  }

  // you need to change the settings to change the fixed dir.
  m_settings->setLastExtractDir(m_extractDirCB->currentText());

  if (m_radioPattern->isChecked())
  {
    if (m_patternLE->text().isEmpty())
    {
      // pattern selected but no pattern? Ask user to select a pattern.
      KMessageBox::error(this,
			   i18n("Please provide a pattern"));
      return;
    }
    else
      {
	emit pattern(m_patternLE->text());
      }
  }

  // I made it! so nothing's wrong.
  KDialogBase::accept();
  kdDebug(1601) << "-ExtractDlg::accept" << endl;
}


void ExtractDlg::browse() // slot
{
  QString dirName
    = KFileDialog::getExistingDirectory(m_settings->getExtractDir(), 0,
					i18n("Select an Extract Directory"));
  if (! dirName.isEmpty())
  {
    m_extractDirCB->insertItem(dirName, 0);
    m_extractDirCB->setCurrentItem(0);
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

void ExtractDlg::openPrefs()
{
  GeneralOptDlg dd(m_settings, this);
  dd.exec();
}

/******************************************************************
 *           implementation of ExtractFailureDlg                  *
 ******************************************************************/

ExtractFailureDlg::ExtractFailureDlg(QStringList *list,
				     QWidget *parent, char *name)
  : QDialog(parent, name, true, 0)

{
  int labelHeight, labelWidth, boxHeight = 75, boxWidth, buttonHeight = 30;
  setCaption(i18n("Failure to Extract"));
  QLabel *pLabel = new QLabel(this);
  pLabel->setText(i18n("Some files already exist in your destination directory.\nThe following files will not be extracted if you continue: "));
  labelWidth = pLabel->sizeHint().width();
  labelHeight = pLabel->sizeHint().height();

  pLabel->setGeometry(10, 10, labelWidth, labelHeight);
  boxWidth = labelWidth;

  QListBox *pBox = new QListBox(this);
  pBox->setGeometry(10, 10 + labelHeight + 10,
		    boxWidth, boxHeight);
  pBox->insertStringList(*list);

  QPushButton *pOKButton = new QPushButton(this, "OKButton");
  pOKButton->setGeometry( labelWidth / 2 - 50, boxHeight + labelHeight + 30,
			 70, buttonHeight);
  pOKButton->setText(i18n("Continue"));
  connect(pOKButton, SIGNAL(pressed()), this, SLOT(accept()));

  QPushButton *pCancelButton = new QPushButton(this, "CancelButton");
  pCancelButton->setGeometry( labelWidth / 2 + 20,
			      boxHeight + labelHeight + 30,
			      70, buttonHeight);
  pCancelButton->setText(i18n("Cancel"));
  connect(pCancelButton, SIGNAL(pressed()), this, SLOT(reject()));
  setFixedSize(20+labelWidth, 40+labelHeight+boxHeight+buttonHeight);
  QApplication::restoreOverrideCursor();
}


#include "extractdlg.moc"
