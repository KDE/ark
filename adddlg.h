//  -*-C++-*-           emacs magic for .h files
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

/* The original purpose of this class was a combined Add/preferences dialog.
   This is rather silly, and the UI was terrible. Now this serves as an
   add dialog for both add file and add directory, and for the most part
   can be used exactly like a KFileDialog.
*/

#ifndef __ADDDLG_H__
#define __ADDDLG_H__

class QString;
class QObjectList;
class QVBoxLayout;
class KFileDialog;
class ArkSettings;

#include <kfiledialog.h>

/**
* A file-addition dialog based upon KFileDialog.
* All this really does is add a few tweaks and add a preferences button.
*/
class AddDlg : public KFileDialog
{
  Q_OBJECT
public:
  enum AddTypes {File, Directory};

public:
  AddDlg(AddTypes type, const QString & _sourceDir, ArkSettings *settings,
	 QWidget *parent=0, const char *name=0);

  QString getDirectory();

public slots:
  void openPrefs();

protected:
	virtual void initGUI();

private: // Data
	ArkSettings *m_settings;
};


#endif //  __ADDDLG_H__
