//  -*-C++-*-           emacs magic for .h files
/*                                                                               ark -- archiver for the KDE project
 
 Copyright (C)
 
 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999-2000: Emily Ezust emilye@corel.com
 
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

#ifndef __ADDDLG_H__
#define __ADDDLG_H__

#include <kdialogbase.h>
#include "arkwidget.h"  // for ArchType

class QLineEdit;
class QCheckBox;

class AddDlg : public KDialogBase 
{
  Q_OBJECT
public:
  AddDlg(ArchType _archtype, const QString & _extractDir, 
	 QWidget *parent=0, const char *name=0);
private: // methods
  void setupFirstTab();
  void setupSecondTab(ArchType _archtype);

private: // data
  QString m_sourceDir;  

  // advanced options
  QCheckBox *m_cbRecurse, *m_cbJunkDirNames, *m_cbForceMS, *m_cbConvertLF2CRLF;
};


#endif //  __ADDDLG_H__
