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
#include <qlabel.h>
#include <qlayout.h>
#include <qregexp.h>
#include <qdialog.h>
#include <qlineedit.h>

// KDE includes
#include <klocale.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>

// ark includes
#include "selectDlg.h"
#include "selectDlg.moc"
#include "arksettings.h"

SelectDlg::SelectDlg( ArkSettings *_data, QWidget *_parent, const char *_name )
    : QDialog( _parent, _name, true )
{
    m_settings = _data;

    setCaption( i18n("Selection") );
    QVBoxLayout *mainLayout = new QVBoxLayout( this, 10 );

    /**
    * Tar command horizontal layout
    */
    QHBoxLayout *hbl1 = new QHBoxLayout();
    mainLayout->addLayout( hbl1 );

    QLabel *l1 = new QLabel( i18n("Select files:"), this );
    l1->setFixedSize( l1->sizeHint() );
    hbl1->addWidget( l1 );

    m_ok = new KPushButton( KStdGuiItem::ok(), this );

    QString pattern = m_settings->getSelectRegExp();
    m_regExp = new QLineEdit( this );
    m_regExp->setFixedSize( m_regExp->sizeHint() );
    m_regExp->setText( pattern );
    m_regExp->setSelection(0, pattern.length() );
    regExpChanged( pattern );
    hbl1->addWidget( m_regExp );
    connect( m_regExp, SIGNAL(textChanged(const QString&)), SLOT(regExpChanged(const QString&)) );

    QHBoxLayout *hbl = new QHBoxLayout();
    mainLayout->addStretch( 1 );
    mainLayout->addLayout( hbl );

    hbl->addStretch( 1 );
    m_ok->setFixedSize( m_ok->sizeHint() );
    m_ok->setDefault(true);
    connect( m_ok, SIGNAL( clicked() ), SLOT( saveConfig() ) );
    hbl->addWidget( m_ok );

    QPushButton *cancel = new KPushButton( KStdGuiItem::cancel(), this );
    cancel->setFixedSize( cancel->sizeHint() );
    connect( cancel, SIGNAL( clicked() ), SLOT( reject() ) );
    hbl->addWidget( cancel );

    mainLayout->activate();
    setFixedSize( sizeHint() );
    m_regExp->setFocus();
}


void SelectDlg::saveConfig()
{
    if( !m_regExp->text().isEmpty() )
        m_settings->setSelectRegExp( m_regExp->text() );
    accept();
}

void SelectDlg::regExpChanged(const QString& _exp)
{
    QRegExp reg_exp(_exp, true, true);
    if(reg_exp.isValid())
        m_ok->setEnabled(true);
    else
        m_ok->setEnabled(false);
}

QString SelectDlg::getRegExp() const
{
    return m_regExp->text();
}
