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
#include <qlabel.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qlistbox.h>
#include <qwidgetstack.h>
#include <qlayout.h>

// KDE includes
#include <klocale.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kurlrequester.h>

// ark includes
#include "dirDlg.h"
#include "arksettings.h"

DirWidget::DirWidget( DirType type, QWidget *parent, const char *name  )
    : QWidget( parent, name )
{
  int mode = KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly;

  QVBoxLayout *vbox = new QVBoxLayout( this, 0, KDialog::spacingHint() );

  btnGroup = new QButtonGroup( this );
  btnGroup->setFrameShape( QFrame::NoFrame );

  rbFav = new QRadioButton( i18n( "Favorite directory" ), this );
  btnGroup->insert( rbFav );
  vbox->addWidget( rbFav );

  dirFav = new KURLRequester( this );
  dirFav->setMode( mode );
  dirFav->setEnabled( false );
  vbox->addWidget( dirFav );

  connect( rbFav, SIGNAL( toggled( bool ) ), 
            dirFav, SLOT( setEnabled( bool ) ) );

  rbFixed = new QRadioButton( this );
  btnGroup->insert( rbFixed );
  vbox->addWidget( rbFixed );

  dirFixed = new KURLRequester( this );
  dirFixed->setMode( mode );
  dirFixed->setEnabled( false );
  vbox->addWidget( dirFixed );

  connect( rbFixed, SIGNAL( toggled( bool ) ), 
            dirFixed, SLOT( setEnabled( bool ) ) );

  rbLast = new QRadioButton( this );
  btnGroup->insert( rbLast );
  vbox->addWidget( rbLast );

  connect( dirFav, SIGNAL( textChanged( const QString & ) ),
            SIGNAL( favDirChanged( const QString & ) ) );

  connect( parent, SIGNAL( favDirChanged( const QString & ) ),
            SLOT( slotFavDirChanged( const QString & ) ) );

  switch ( type )
  {
    case StartupDir:

      rbFixed->setText( i18n( "Fixed start-up directory" ) );
      rbLast->setText( i18n( "&Last start-up directory" ) );
      break;

    case OpenDir:

      rbFixed->setText( i18n( "Fixed open directory" ) );
      rbLast->setText( i18n( "&Last open directory" ) );
      break;

    case ExtractDir:

      rbFixed->setText( i18n( "Fixed extract directory" ) );
      rbLast->setText( i18n( "&Last extract directory" ) );
      break;

    case AddDir:

      rbFixed->setText( i18n( "Fixed add directory" ) );
      rbLast->setText( i18n( "&Last add directory" ) );
      break;

    default:

      break;
  }
}

void DirWidget::slotFavDirChanged( const QString &url )
{
  if ( dirFav->url() != url )
    dirFav->setURL( url );
}

DirDlg::DirDlg(ArkSettings *d, QWidget *parent, const char *name)
	: QWidget(parent, name)
{
  data = d;

  QVBoxLayout *vbox = new QVBoxLayout( this, 0, KDialog::spacingHint() );

  //setCaption( i18n("Directories Preferences - ark") );
  QLabel* l2 = new QLabel( this, "Label_2" );
  l2->setText( i18n("Directories:") );
  vbox->addWidget( l2 );

  pListBox = new QListBox( this, "ListBox_1" );
  pListBox->insertItem(i18n("Start-up directory"), 0);
  pListBox->insertItem(i18n("directory for opening files","Open Directory"), 1);
  pListBox->insertItem(i18n("directory for extracting files","Extract Directory"), 2);
  pListBox->insertItem(i18n("directory for adding files","Add Directory"), 3);
  pListBox->setFixedHeight( 80 );
  vbox->addWidget( pListBox );

  connect(pListBox, SIGNAL(highlighted(int)), SLOT(dirTypeChanged(int)));

  stack = createWidgetStack();
  vbox->addWidget( stack );

  vbox->addSpacing( 20 );
  vbox->addStretch( 1 );

  pListBox->setCurrentItem(0); // this will make the right buttons show

  initConfig();
}

QWidgetStack *DirDlg::createWidgetStack()
{
  DirWidget *w;
  DirType types[] = { StartupDir, OpenDir, ExtractDir, AddDir };

  QWidgetStack *_stack = new QWidgetStack( this );

  for ( int i = 0; i < 4; ++i )
  {
    w = new DirWidget( types[ i ], this );
    connect( w, SIGNAL( favDirChanged( const QString & ) ),
            SIGNAL( favDirChanged( const QString & ) ) );
    _stack->addWidget( w, i );
  }

  return _stack;
}

DirDlg::~DirDlg()
{
}

void DirDlg::dirTypeChanged(int index)
{
  stack->raiseWidget( index );
}

void DirDlg::initConfig()
{
  DirWidget *startup = static_cast<DirWidget *>( stack->widget( 0 ) );
  DirWidget *open = static_cast<DirWidget *>( stack->widget( 1 ) );
  DirWidget *extract = static_cast<DirWidget *>( stack->widget( 2 ) );
  DirWidget *add = static_cast<DirWidget *>( stack->widget( 3 ) );

  startup->dirFav->setURL( data->getFavoriteDir() );

  startup->dirFixed->setURL( data->getFixedStartDir() );
  open->dirFixed->setURL( data->getFixedOpenDir() );
  extract->dirFixed->setURL( data->getFixedExtractDir() );
  add->dirFixed->setURL( data->getFixedAddDir() );

  switch( data->getStartDirMode() )
  {
    case ArkSettings::FAVORITE_DIR:
      startup->rbFav->setChecked( true );
      startup->dirFav->setEnabled( true );
      break; 
    case ArkSettings::FIXED_START_DIR:
      startup->rbFixed->setChecked( true );
      startup->dirFixed->setEnabled( true );
      break;
    case ArkSettings::LAST_OPEN_DIR: 
      startup->rbLast->setChecked( true );
      break;
    default:
      break;
  }

  switch( data->getOpenDirMode() )
  {
    case ArkSettings::FAVORITE_DIR:
      open->rbFav->setChecked( true ); 
      open->dirFav->setEnabled( true );
      break; 
    case ArkSettings::FIXED_OPEN_DIR:
      open->rbFixed->setChecked( true ); 
      open->dirFixed->setEnabled( true );
      break;
    case ArkSettings::LAST_OPEN_DIR:
      open->rbLast->setChecked( true );
      break;        
    default:
      break;
  }
 
  switch( data->getExtractDirMode() )
  {
    case ArkSettings::FAVORITE_DIR:
      extract->rbFav->setChecked( true ); 
      extract->dirFav->setEnabled( true );
      break; 
    case ArkSettings::FIXED_EXTRACT_DIR:
      extract->rbFixed->setChecked( true );
      extract->dirFixed->setEnabled( true );
      break;
    case ArkSettings::LAST_EXTRACT_DIR:
      extract->rbLast->setChecked( true ); 
      break;     
    default:
      break;
  }
 
  switch( data->getAddDirMode() )
  {
    case ArkSettings::FAVORITE_DIR:
      add->rbFav->setChecked( true ); 
      add->dirFav->setEnabled( true );
      break;        
    case ArkSettings::FIXED_ADD_DIR:
      add->rbFixed->setChecked( true );
      add->dirFixed->setEnabled( true );
      break;
    case ArkSettings::LAST_ADD_DIR:
      add->rbLast->setChecked( true );
      break;
    default:
      break;
  }
}

void DirDlg::saveConfig()
{
  DirWidget *startup = static_cast<DirWidget *>( stack->widget( 0 ) );
  DirWidget *open = static_cast<DirWidget *>( stack->widget( 1 ) );
  DirWidget *extract = static_cast<DirWidget *>( stack->widget( 2 ) );
  DirWidget *add = static_cast<DirWidget *>( stack->widget( 3 ) );

  int mode;
  data->setFavoriteDir( startup->dirFav->url() );

  mode = startup->rbFav->isChecked() ?  ArkSettings::FAVORITE_DIR :
    startup->rbLast->isChecked() ?  ArkSettings::LAST_OPEN_DIR :
    ArkSettings::FIXED_START_DIR;
  data->setStartDirCfg( startup->dirFixed->url(), mode );

  mode = open->rbFav->isChecked() ?  ArkSettings::FAVORITE_DIR :
    open->rbLast->isChecked() ?  ArkSettings::LAST_OPEN_DIR :
    ArkSettings::FIXED_OPEN_DIR;
  data->setOpenDirCfg( open->dirFixed->url(), mode );

  mode = extract->rbFav->isChecked() ?  ArkSettings::FAVORITE_DIR :
    extract->rbLast->isChecked() ?  ArkSettings::LAST_EXTRACT_DIR :
    ArkSettings::FIXED_EXTRACT_DIR;
  data->setExtractDirCfg( extract->dirFixed->url(), mode );

  mode = add->rbFav->isChecked() ?  ArkSettings::FAVORITE_DIR :
    add->rbLast->isChecked() ?  ArkSettings::LAST_ADD_DIR : 
    ArkSettings::FIXED_ADD_DIR;
  data->setAddDirCfg( add->dirFixed->url(), mode );
}


#include "dirDlg.moc"
