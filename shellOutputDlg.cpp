/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org

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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// Qt includes
#include <qlayout.h>
#include <qtextedit.h>

#include <klocale.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>

// ark includes
#include "shellOutputDlg.h"
#include "shellOutputDlg.moc"
#include "arksettings.h"

ShellOutputDlg::ShellOutputDlg( ArkSettings *_data, QWidget *_parent,
				const char *_name )
	: KDialogBase( _parent, _name, true, i18n( "Shell Output" ), KDialogBase::Ok, KDialogBase::Ok, true )
{
  QTextEdit *outputViewer = new QTextEdit( this );
  outputViewer->setTextFormat( Qt::PlainText );
  outputViewer->setReadOnly( true );

  outputViewer->setText( *( _data->getLastShellOutput() ) );
  outputViewer->setCursorPosition( outputViewer->lines(), 0 );

  setMainWidget( outputViewer );

  resize( 520, 380 );
}
