/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)

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
#include <qpushbutton.h>
#include <qlistbox.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>

// KDE includes
#include <kfiledialog.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
// ark includes
#include "dirDlg.h"
#include "dirDlg.moc"

#define BROWSE_WIDTH 40
#define DLG_WIDTH 530
#define DLG_HEIGHT 310
// don't forget to change common_texts.cpp if you change a text here
#define STARTUPDIR i18n("(used as part of a sentence)","start-up directory")
#define OPENDIR i18n("directory for opening files (used as part of a sentence)","open directory")
#define EXTRACTDIR i18n("directory for extracting files (used as part of a sentence)","extract directory")
#define ADDDIR i18n("directory for adding files (used as part of a sentence)","add directory")

void WidgetHolder::hide()
{
  lDirType->hide();
  fixedLE->hide();
  lHorizLine->hide();
  buttonGroup->hide();
  for (int j = 0; j < NUM_RADIOS; ++j)
    radios[j]->hide();
}

void WidgetHolder::show()
{
  lDirType->show();
  fixedLE->show();
  lHorizLine->show();
  for (int j = 0; j < NUM_RADIOS; ++j)
    radios[j]->show();
  buttonGroup->show();
}

DirDlg::DirDlg( ArkSettings *d, QWidget *parent, const char *name )
	: QDialog( parent, name, true )
{
  data = d;
	
  setCaption( i18n("Directories Preferences - ark") );

  QLabel *l1 = new QLabel( this, "Label_1" );
  l1->setGeometry( 10, 20, 300, 20 );
  l1->setText( i18n("Favorite directory:") );

  favLE = new QLineEdit( this, "LineEdit_1" );
  favLE->setGeometry( 10, 40, 510, 20 );

  QPushButton* browseFav;
  browseFav = new QPushButton( this, "PushButton_1" );
  browseFav->setText( i18n("Browse...") );
  browseFav->setMinimumSize(80, 30);  // improved i18n support, 5.9.2000, Gregor Zumstein
  browseFav->adjustSize();
  browseFav->move( 520 - browseFav->width(), 70);
  connect( browseFav, SIGNAL(clicked()), SLOT(getFavDir()) );
  
  QLabel* l2;
  l2 = new QLabel( this, "Label_2" );
  l2->setGeometry( 10, 100, 130, 20 );
  l2->setText( i18n("Directories:") );

  pListBox = new QListBox( this, "ListBox_1" );
  pListBox->setGeometry( 10, 120, 180, 130 );
  pListBox->insertItem(i18n("Start-up directory"), 0);
  pListBox->insertItem(i18n("directory for opening files","Open directory"), 1);
  pListBox->insertItem(i18n("directory for extracting files","Extract directory"), 2);
  pListBox->insertItem(i18n("directory for adding files","Add directory"), 3);
  connect(pListBox, SIGNAL(highlighted(int)),
	  this, SLOT(dirTypeChanged(int)));

  createRepeatingWidgets();

  QPushButton *browseFixed = new QPushButton( this, "PushButton_2" );
  browseFixed->setText( i18n("Browse...") );
  browseFixed->setMinimumSize(80, 30);  // GZ
  browseFixed->adjustSize();
  browseFixed->move( 520 - browseFixed->width(), 215);
  connect( browseFixed, SIGNAL(clicked()), SLOT(getFixedDir()) );

  QLabel *lHoriz2 = new QLabel( this, "Label_5" );
  lHoriz2->setGeometry( 10, 255, 510, 10 );
  lHoriz2->setFrameStyle( 52 );
  lHoriz2->setLineWidth( 1 );

  QPushButton* cancelButton = new QPushButton( this, "PushButton_3" );
  cancelButton->setGeometry( 440, 270, 80, 30 );
  cancelButton->setText( i18n("Cancel") );
  connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

  QPushButton* OKButton = new QPushButton( this, "PushButton_4" );
  OKButton->setGeometry( 350, 270, 80, 30 );
  OKButton->setText(i18n( "OK") );
  connect(OKButton, SIGNAL(clicked()), this, SLOT(saveConfig()));
  OKButton->setFocus();

  pListBox->setCurrentItem(0); // this will make the right buttons show
  setFixedSize( DLG_WIDTH, DLG_HEIGHT );
  initConfig();
}

void DirDlg::createRepeatingWidgets()
{
  for (int i = 0; i < NUM_DIR_TYPES; ++i)
    {
      widgets[i] = new WidgetHolder;
      widgets[i]->lDirType = new QLabel(this);
      widgets[i]->fixedLE = new QLineEdit(this);
      widgets[i]->fixedLE->setGeometry( 250, 190, 270, 20 );
      widgets[i]->buttonGroup = new QButtonGroup(this);
      widgets[i]->buttonGroup->setFrameShape(QFrame::NoFrame);
      widgets[i]->buttonGroup->setGeometry(230, 125, DLG_WIDTH-220, 60);

      for (int j = 0; j < NUM_RADIOS; ++j)
	// radio buttons
	{
	  widgets[i]->radios[j] = new QRadioButton(widgets[i]->buttonGroup);
	  widgets[i]->radios[j]->setGeometry(0, j*20, 200, 20);
	  widgets[i]->buttonGroup->insert(widgets[i]->radios[j]);
	  if (j == 0) widgets[i]->radios[j]->setText(i18n("Favorite directory") );
	  if (j == 2) widgets[i]->radios[j]->setText(i18n("Fixed directory"));
	}
      widgets[i]->radios[0]->adjustSize(); // GZ
      widgets[i]->radios[2]->adjustSize(); // GZ
    }
  widgets[0]->lDirType->setText(i18n("Default start-up directory"));
  widgets[1]->lDirType->setText(i18n("Default open directory"));
  widgets[2]->lDirType->setText(i18n("Default extract directory"));
  widgets[3]->lDirType->setText(i18n("Default add directory"));
  widgets[0]->radios[1]->setText(i18n("Last start-up directory"));
  widgets[1]->radios[1]->setText(i18n("Last open directory"));
  widgets[2]->radios[1]->setText(i18n("Last extract directory"));
  widgets[3]->radios[1]->setText(i18n("Last add directory"));
  for (int i = 0; i < NUM_DIR_TYPES; ++i)  // GZ
    {
      widgets[i]->radios[1]->adjustSize();
    }

  int x;
  for (int i = 0; i < NUM_DIR_TYPES; ++i)
    {
      widgets[i]->lDirType->setGeometry(220, 100, widgets[i]->lDirType->sizeHint().width(), 20 );
      // set up horizontal line based on how wide the corresponding dir type is
      x = widgets[i]->lDirType->x() + widgets[i]->lDirType->width() + 5;
      widgets[i]->lHorizLine = new QLabel(this);
      widgets[i]->lHorizLine->setGeometry(x, 105, DLG_WIDTH-10-x, 10);
      widgets[i]->lHorizLine->setFrameStyle( 52 );
      widgets[i]->lHorizLine->setLineWidth( 1 );
    }
}

void DirDlg::hideWidgets()
{
  for (int i = 0; i < NUM_DIR_TYPES; ++i)
    {
      widgets[i]->hide();
    }
}

DirDlg::~DirDlg()
{
  for (int i = 0; i < NUM_DIR_TYPES; ++i)
    {
      for (int j = 0; j < NUM_RADIOS; ++j)
	{
	  delete widgets[i]->radios[j];
	}
      delete widgets[i]->lDirType;
      delete widgets[i]->lHorizLine;
      delete widgets[i]->fixedLE;
      delete widgets[i];
    }
}

QString DirDlg::getDirType(int item)
{
  if (item == 0) return STARTUPDIR;
  else if (item == 1) return OPENDIR;
  else if (item == 2) return EXTRACTDIR ;
  else if (item == 3) return ADDDIR;

  ASSERT(0);
  return "";
}

void DirDlg::dirTypeChanged(int _dirType)
{
  // hide the widgets and show the ones pertaining to this dir type.
  hideWidgets();
  widgets[_dirType]->show();
}

void DirDlg::initConfig()
  // get existing settings and plunk into the widgets
{
  favLE->setText( data->getFavoriteDir() );
  widgets[0]->fixedLE->setText( data->getFixedStartDir() );
  widgets[1]->fixedLE->setText( data->getFixedOpenDir() );
  widgets[2]->fixedLE->setText( data->getFixedExtractDir() );
  widgets[3]->fixedLE->setText( data->getFixedAddDir() );

  switch( data->getStartDirMode() )
    {
    case ArkSettings::FAVORITE_DIR:
      widgets[0]->radios[0]->setChecked( true );
      break; 
    case ArkSettings::LAST_OPEN_DIR: 
      widgets[0]->radios[1]->setChecked( true );
      break;        
    case ArkSettings::FIXED_START_DIR:
      widgets[0]->radios[2]->setChecked( true );
      break;      
    }
  switch( data->getOpenDirMode() )
    {
    case ArkSettings::FAVORITE_DIR:
      widgets[1]->radios[0]->setChecked( true ); 
      break; 
    case ArkSettings::LAST_OPEN_DIR:
      widgets[1]->radios[1]->setChecked( true );
      break;        
    case ArkSettings::FIXED_OPEN_DIR:
      widgets[1]->radios[2]->setChecked( true ); 
      break;       
    }
  
  switch( data->getExtractDirMode() )
    {
    case ArkSettings::FAVORITE_DIR:
      widgets[2]->radios[0]->setChecked( true ); 
      break; 
    case ArkSettings::LAST_EXTRACT_DIR:
      widgets[2]->radios[1]->setChecked( true ); 
      break;     
    case ArkSettings::FIXED_EXTRACT_DIR:
      widgets[2]->radios[2]->setChecked( true );
      break;    
    }
        
  switch( data->getAddDirMode() )
    {
    case ArkSettings::FAVORITE_DIR:
      widgets[3]->radios[0]->setChecked( true ); 
      break;        
    case ArkSettings::LAST_ADD_DIR:
      widgets[3]->radios[1]->setChecked( true );
      break;
    case ArkSettings::FIXED_ADD_DIR:
      widgets[3]->radios[2]->setChecked( true );
      break;
    }
}

void DirDlg::getFavDir( )
{
  QString dir
    = KFileDialog::getExistingDirectory(favLE->text(), 0,
					i18n("Favorite directory"));

  if (!dir.isEmpty())
    favLE->setText(dir);
}


void DirDlg::getFixedDir( )
{
  int i;
  // see which fixedLE is visible, and use that.
  for (i = 0; i <  NUM_DIR_TYPES; ++i)
    {
      if (widgets[i]->fixedLE->isVisible())
	break;
    }
  ASSERT(i < NUM_DIR_TYPES);
  QString dir
    = KFileDialog::getExistingDirectory(widgets[i]->fixedLE->text(), 0,
					i18n("Fixed directory"));
  if (!dir.isEmpty())
    widgets[i]->fixedLE->setText(dir);

}

void DirDlg::saveConfig()
{

  QDir *fav = new QDir( favLE->text());
  if ( !fav->exists() )
    {
      KMessageBox::error( this, i18n("The directory specified as your favorite does not exist."));
      return;
    }

  for (int i = 0; i <  NUM_DIR_TYPES; ++i)
    {
      QString fixedStr = widgets[i]->fixedLE->text();
      if (!fixedStr.isEmpty())
	{
	  QDir *fixed = new QDir (fixedStr);
	  if (!fixed->exists())
	    {
	      KMessageBox::error(this, i18n("The fixed directory specified for your %1 does not exist.").arg(getDirType(i)));
	      return;
	    }
	}
    }
  
  int mode;
  data->setFavoriteDir( favLE->text() );
        
  mode = widgets[0]->radios[0]->isChecked() ? 
    ArkSettings::FAVORITE_DIR :
    widgets[0]->radios[1]->isChecked() ?
    ArkSettings::LAST_OPEN_DIR :
    ArkSettings::FIXED_START_DIR;
  data->setStartDirCfg( widgets[0]->fixedLE->text(), mode );

  mode = widgets[1]->radios[0]->isChecked() ?
    ArkSettings::FAVORITE_DIR :
    widgets[1]->radios[1]->isChecked() ?
    ArkSettings::LAST_OPEN_DIR :
    ArkSettings::FIXED_OPEN_DIR;
  data->setOpenDirCfg( widgets[1]->fixedLE->text(), mode );

  mode = widgets[2]->radios[0]->isChecked() ? 
    ArkSettings::FAVORITE_DIR :
    widgets[2]->radios[1]->isChecked() ? 
    ArkSettings::LAST_EXTRACT_DIR :
    ArkSettings::FIXED_EXTRACT_DIR;
  data->setExtractDirCfg( widgets[2]->fixedLE->text(), mode );

  mode = widgets[3]->radios[0]->isChecked() ? 
    ArkSettings::FAVORITE_DIR :
    widgets[3]->radios[1]->isChecked() ? 
    ArkSettings::LAST_ADD_DIR : 
    ArkSettings::FIXED_ADD_DIR;
  data->setAddDirCfg( widgets[3]->fixedLE->text(), mode );
  accept();
}
