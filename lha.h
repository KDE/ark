//  -*-C++-*-           emacs magic for .h files
#ifndef LHAARCH_H
#define LHAARCH_H

// Qt includes
#include <qstring.h>
#include <qstrlist.h>

#include "arch.h"
#include "filelistview.h"

class LhaArch : public Arch 
{
  Q_OBJECT
public:
  LhaArch( ArkSettings *_settings, Viewer *_gui,
	   const QString & _fileName );
  virtual ~LhaArch() { }
	
  virtual void open();
  virtual void create();
	
  virtual void addFile( QStringList* );
  virtual void addDir(const QString & _dirName);

  virtual void remove(QStringList *);
  virtual void unarchFile(QStringList *, const QString & _destDir="");

protected:
  bool m_header_removed, m_finished, m_error;

protected slots:
  void slotReceivedTOC(KProcess *, char *, int);
	
private:
  void processLine( char* );	
  void initExtract( bool, bool, bool );
  void setHeaders();
};

#endif /* ARCH_H */
