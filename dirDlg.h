//  -*-C++-*-           emacs magic for .h files
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

#ifndef DIRDLG_H
#define DIRDLG_H

class QString;
class QWidget;
class QListBox;
class QRadioButton;
class QWidgetStack;

class KURLRequester;

class ArkSettings;

enum DirType { StartupDir, OpenDir, ExtractDir, AddDir };

class DirWidget : public QWidget
{
  Q_OBJECT

public:

  DirWidget( DirType type, QWidget *parent=0, const char *name=0 );

signals:

  void favDirChanged( const QString & );

protected slots:

  void slotFavDirChanged( const QString & );

public:

  KURLRequester *dirFav, *dirFixed;
  QButtonGroup *btnGroup;
  QRadioButton *rbFav, *rbFixed, *rbLast;
};

class DirDlg : public QWidget
{
  Q_OBJECT

public:

  DirDlg(ArkSettings *d, QWidget *parent=0, const char *name=0);
  ~DirDlg();

public slots:

  void saveConfig();
  void dirTypeChanged(int _dirType);

signals:

  void favDirChanged( const QString & );

private: // methods  

  void initConfig();
  QWidgetStack* createWidgetStack();
  void hideWidgets();

private: // data

  QWidgetStack *stack;
  ArkSettings *data;
  QListBox *pListBox;
};

#endif /* DIRDLG_H */

