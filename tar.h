//  -*-C++-*-           emacs magic for .h files
/*

  ark -- archiver for the KDE project

  Copyright (C)

  1997-1999: Rob Palmbos palm9744@kettering.edu
  1999: Francois-Xavier Duranceau duranceau@kde.org
  1999-2000: Corel Corporation (Emily Ezust, emilye@corel.com)

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

#ifndef TAR_H
#define TAR_H

#include <unistd.h>

// Qt includes
#include <qdir.h>
#include <qobject.h>
#include <qstring.h>
#include <qstrlist.h>


// KDE includes
#include <kprocess.h>
#include <ktar.h>

// ark includes

#include "arch.h"
#include "arksettings.h"

class Viewer;

// TarArch can read Tar files and Tar files compressed with gzip.
// It doesn't yet know how to list Tar files compressed with other
// compressors because the listing part is done through KTar.
// If it could be listed though, the rest would work.
// The reason we use KTar for listing is that the output tar -tvf is
// of unreliable format.

class TarArch : public Arch
{
  Q_OBJECT
public:
  TarArch( ArkSettings *_settings, Viewer *_gui, const QString & _filename);
  virtual ~TarArch();
	
  virtual void open();
  virtual void create();
	
  virtual int addFile( QStringList *);
  virtual int addDir(const QString &);
  virtual void remove(QStringList *);
  virtual QString unarchFile(QStringList *, const QString & _destDir="");
	
  virtual int getEditFlag();
	
  int actionFlag() { return 0; }
  QString getCompressor();
  QString getUnCompressor();

public slots:
  void updateProgress( KProcess *_kp, char *_buffer, int _bufflen);
  void extractProgress( KProcess *, char *_buffer, int _bufflen );
  void openFinished( KProcess * );
  void updateFinished( KProcess * );
  void createTmpFinished( KProcess * );
  void createTmpProgress( KProcess *_kp, char *_buffer, int _bufflen);
  void extractFinished( KProcess * );
  void slotAddFinished(KProcess *);

protected slots:
  void slotOpenDataStdout(KProcess*, char*, int);

private:  // methods
  int updateArch();
  void createTmp();
  void setHeaders();
  void processDir(const KTarDirectory *tardir, const QString & root);

private: // data
  QString tmpfile;
  bool compressed;
  bool perms, tolower, overwrite;
  QString tmpdir; // easily recalculated, but I may need it a lot

  // for use with createTmp and updateArch
  bool createTmpInProgress;
  bool updateInProgress;
  FILE *fd;
};

#endif /* TAR_H */
