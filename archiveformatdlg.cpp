/*
 ark -- archiver for the KDE project

 Copyright (C) 2003: Georg Robbers <Georg.Robbers@urz.uni-hd.de>

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include "archiveformatdlg.h"
#include "archiveformatinfo.h"
#include "archive.h"

#include <klocale.h>
#include <kvbox.h>

#include <QLabel>

ArchiveFormatDlg::ArchiveFormatDlg( QWidget * parent, const QString & defaultType )
                        :KDialog( parent),
                          m_combo( 0 )
{
    setCaption( i18n( "Choose Archive Format" ) );
    setModal( true );
    setButtons( KDialog::Ok|KDialog::Cancel );
    setDefaultButton( KDialog::Ok );
    QString defaultDescription = ArchiveFormatInfo::self()->descriptionForMimeType( defaultType );
    QString text;
    if ( defaultDescription.isNull() )
        text = i18n( "This file appears to be of type %1,\n"
                     "which is not a supported archive format.\n"
                     "In order to proceed, please choose the format\n"
                     "of the file.", defaultType );
    else
        text = i18n( "You are about to open a file that has a non-standard extension.\n"
                    "Ark has detected the format: %1\n"
                    "If this is not correct, please choose "
                    "the appropriate format.", defaultDescription );

    KVBox * page = new KVBox( this );
    setMainWidget( page );

    QLabel * label;
    label = new QLabel( text, page );

    m_combo = new KComboBox( page );
    QStringList list = ArchiveFormatInfo::self()->allDescriptions();
    list.sort();
    m_combo->addItems( list );
    m_combo->setCurrentIndex( list.indexOf( defaultDescription ) );
}

QString ArchiveFormatDlg::mimeType()
{
    if (m_combo && !m_combo->currentText().isEmpty())
        return ArchiveFormatInfo::self()->mimeTypeForDescription( m_combo->currentText() );
    else
        return QString();
}

#include "archiveformatdlg.moc"

