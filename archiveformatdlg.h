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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef ARCHIVEFORMATDLG_H
#define ARCHIVEFORMATDLG_H

#include "arch.h"

#include <kdialogbase.h>
#include <kcombobox.h>

class ArchiveFormatDlg: public KDialogBase
{
    Q_OBJECT
public:
    ArchiveFormatDlg( QWidget * parent, const QString & defaultType );
    QString mimeType();

private:
    KComboBox * m_combo;
};
#endif // ARCHIVEFORMATDLG_H

