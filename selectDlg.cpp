/*

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
#include <qlabel.h>
#include <q3vbox.h>
#include <qregexp.h>
#include <qpushbutton.h>

// KDE includes
#include <klocale.h>
#include <kstdguiitem.h>
#include <kpushbutton.h>
#include <kdialogbase.h>
#include <klineedit.h>

// ark includes
#include "selectDlg.h"

SelectDlg::SelectDlg( QWidget *_parent, const char *_name )
    : KDialogBase( _parent, _name, true, i18n("Selection"),
                   KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok )
{
    Q3HBox * box = makeHBoxMainWidget();

    QLabel *l1;
    l1 = new QLabel( i18n("Select files:"), box );

    m_regExp = new KLineEdit( box );
    m_regExp->setMinimumWidth( fontMetrics().maxWidth() * 6 );

    connect( m_regExp, SIGNAL( textChanged( const QString& ) ),
                       SLOT( regExpChanged( const QString& ) ) );

    m_regExp->setFocus();
}

void SelectDlg::regExpChanged( const QString& _exp )
{
    QRegExp reg_exp( _exp, true, true );
    enableButtonOK( reg_exp.isValid() );
}

QString SelectDlg::getRegExp() const
{
    return m_regExp->text();
}

#include "selectDlg.moc"

