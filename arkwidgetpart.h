/*
  Copyright (C)

  2001: Macadamian Technologies Inc (author: Jian Huang, jian@macadamian.com)

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

#ifndef ARKWIDGETPART_H
#define ARKWIDGETPART_H

#include <qwidget.h>
#include "arkwidgetbase.h"

class QString;
class QWidget;
class QResizeEvent;
class KRun;
class KTempFile;

class Arch;
class ArkWidgetBase;
class FileLVI;


namespace Utilities
{
  bool diskHasSpacePart(const QString &dir, long size);
};

class ArkWidgetPart : public QWidget, public ArkWidgetBase
{
  Q_OBJECT
public:
  ArkWidgetPart( QWidget *parent=0, const char *name=0 );
  virtual ~ArkWidgetPart();
  void showZip( QString name );

public slots:
  void file_open(const QString &);  // opens the specified archive
  void action_extract();   // extracts the specified archive
  void action_view();      // views the specified file in an opened archive
  void edit_view_last_shell_output();

public: //data
  int  goodFileType; 
  
signals:
  void toKpartsView(int, int);

protected:
virtual  void resizeEvent ( QResizeEvent * );
    
private slots:
  void file_close();  
  void slotSelectionChanged();
  void slotOpen(Arch *, bool, const QString &, int);
  void slotExtractDone();
  void selectByPattern(const QString & _pattern);

private: // methods
  void updateStatusSelection();
  void updateStatusTotals();

  // complains if the filename has capital letters or is tbz or tbz2
  bool badBzipName(const QString & _filename);
  bool reportExtractFailures(const QString & _dest,
			     QStringList *_list);
  
  void newCaption(const QString& filename);
  void createFileListView();
  void openArchive(const QString & name);
  void showFile(FileLVI *);

private: // data
 // true if user is trying to view something. For use in slotExtractDone
  bool m_bViewInProgress;

  // for use in slotExtractDone: the url.
  QString m_strFileToView;

  KRun *m_pKRunPtr;
  
  KTempFile *mpTempFile;
};

#endif /* ARKWIDGETPART_H*/
