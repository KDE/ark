/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
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

// Qt includes
#include <qlineedit.h>
#include <qlabel.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>

// KDE includes
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>

// ark includes
#include "dirDlg.h"
#include "arksettings.h"
#include <qpushbutton.h>

#define BROWSE_WIDTH 40
#define DLG_WIDTH 390
#define DLG_HEIGHT 280
// don't forget to change common_texts.cpp if you change a text here
#define STARTUPDIR i18n("(used as part of a sentence)","start-up directory")
#define OPENDIR i18n("directory for opening files (used as part of a sentence)","open directory")
#define EXTRACTDIR i18n("directory for extracting files (used as part of a sentence)","extract directory")
#define ADDDIR i18n("directory for adding files (used as part of a sentence)","add directory")

void WidgetHolder::hide()
{
  buttonGroup->hide();
  for (int j = 0; j < NUM_RADIOS; ++j)
    radios[j]->hide();
  fixedLE->hide();
}

void WidgetHolder::show()
{
  for (int j = 0; j < NUM_RADIOS; ++j)
    radios[j]->show();
  buttonGroup->show();
  fixedLE->show();
}

DirDlg::DirDlg(ArkSettings *d, QWidget *parent, const char *name)
	: QWidget(parent, name)
{
  data = d;

  createRepeatingWidgets();

  //setCaption( i18n("Directories Preferences - ark") );
  QLabel* l2;
  l2 = new QLabel( this, "Label_2" );
  l2->setGeometry( 10, 10, 130, 20 );
  l2->setText( i18n("Directories:") );

  pListBox = new QListBox( this, "ListBox_1" );
  pListBox->setGeometry( 10, 30, DLG_WIDTH - 20, 80 );
  pListBox->insertItem(i18n("Start-up directory"), 0);
  pListBox->insertItem(i18n("directory for opening files","Open directory"), 1);
  pListBox->insertItem(i18n("directory for extracting files","Extract directory"), 2);
  pListBox->insertItem(i18n("directory for adding files","Add directory"), 3);
  connect(pListBox, SIGNAL(highlighted(int)),
	  this, SLOT(dirTypeChanged(int)));

  favLE = new QLineEdit( this, "LineEdit_1" );
  favLE->setGeometry( 10, 150, DLG_WIDTH-110, 20 );
  QPushButton* browseFav;
  browseFav = new QPushButton( this, "PushButton_1" );
  browseFav->setText( i18n("Browse...") );
  browseFav->setMinimumSize(80, 30);  // improved i18n support, 5.9.2000, Gregor Zumstein
  browseFav->adjustSize();
  browseFav->move( DLG_WIDTH - 10 - browseFav->width(), 145);
  connect( browseFav, SIGNAL(clicked()), SLOT(getFavDir()) );
  
  QPushButton *browseFixed = new QPushButton( this, "PushButton_2" );
  browseFixed->setText( i18n("Browse...") );
  browseFixed->setMinimumSize(80, 30);  // GZ
  browseFixed->adjustSize();
  browseFixed->move( DLG_WIDTH - 10 - browseFixed->width(), 185);
  connect( browseFixed, SIGNAL(clicked()), SLOT(getFixedDir()) );

  pListBox->setCurrentItem(0); // this will make the right buttons show
  setMinimumSize( DLG_WIDTH, DLG_HEIGHT );
  initConfig();
}

void DirDlg::createRepeatingWidgets()
{
  for (int i = 0; i < NUM_DIR_TYPES; ++i)
    {
      widgets[i] = new WidgetHolder;
      widgets[i]->buttonGroup = new QButtonGroup(this);
      widgets[i]->buttonGroup->setFrameShape(QFrame::NoFrame);
      widgets[i]->buttonGroup->setGeometry(10, 130, DLG_WIDTH-220, 160);

      for (int j = 0; j < NUM_RADIOS; ++j)
	// radio buttons
	{
	  widgets[i]->radios[j] = new QRadioButton(widgets[i]->buttonGroup);
	  widgets[i]->radios[j]->setGeometry(0, j*40, 200, 20);
	  //widgets[i]->buttonGroup->insert(widgets[i]->radios[j]);
	  if (j == 0) widgets[i]->radios[j]->setText(i18n("Favorite directory") );
	}
      widgets[i]->radios[0]->setGeometry(0, 0, 200, 20);
      widgets[i]->radios[1]->setGeometry(0, 80, 200, 20);
      widgets[i]->radios[2]->setGeometry(0, 40, 200, 20);

      widgets[i]->radios[0]->adjustSize(); // GZ

      widgets[i]->fixedLE = new QLineEdit(this);
      widgets[i]->fixedLE->setGeometry( 10, 190, DLG_WIDTH-110, 20 );
    }
  widgets[0]->radios[1]->setText(i18n("Last start-up directory"));
  widgets[1]->radios[1]->setText(i18n("Last open directory"));
  widgets[2]->radios[1]->setText(i18n("Last extract directory"));
  widgets[3]->radios[1]->setText(i18n("Last add directory"));
  widgets[0]->radios[2]->setText(i18n("Fixed start-up directory"));
  widgets[1]->radios[2]->setText(i18n("Fixed open directory"));
  widgets[2]->radios[2]->setText(i18n("Fixed extract directory"));
  widgets[3]->radios[2]->setText(i18n("Fixed add directory"));
  for (int i = 0; i < NUM_DIR_TYPES; ++i)  // GZ
    {
      widgets[i]->radios[1]->adjustSize();
      widgets[i]->radios[2]->adjustSize(); // GZ
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
      delete widgets[i]->fixedLE;
      delete widgets[i];
    }
}

QString DirDlg::getDirType(int item)
{
  if (item == 0) return STARTUPDIR;
  else if (item == 1) return OPENDIR;
  else if (item == 2) return EXTRACTDIR;
  else if (item == 3) return ADDDIR;

  Q_ASSERT(0);
  return "";
}

void DirDlg::dirTypeChanged(int _dirType)
{
  // hide the widgets and show the ones pertaining to this dir type.
  hideWidgets();
  widgets[_dirType]->show();
  favLE->show();
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
  Q_ASSERT(i < NUM_DIR_TYPES);
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
}


#include "dirDlg.moc"
