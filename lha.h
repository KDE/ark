#ifndef LHAARCH_H
#define LHAARCH_H

// Qt includes
#include <qstring.h>
#include <qstrlist.h>

#include "arch.h"
#include "arkprocess.h"
#include "filelistview.h"

class LhaArch : public Arch {

public:
	LhaArch( ArkData *d );
	virtual ~LhaArch();
	virtual unsigned char setOptions( bool p, bool l, bool o );
	virtual void openArch( QString, FileListView * );
	virtual void createArch( QString );
	virtual int addFile( QStrList *);
	virtual void extractTo( QString );
	virtual const QStrList *getListing();
	virtual QString unarchFile( int , QString );
	virtual void deleteFile( int );

private:
	QStrList *listing;
	ArkProcess archProcess;
};

#endif /* ARCH_H */
