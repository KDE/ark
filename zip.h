#ifndef ZIPARCH_H
#define ZIPARCH_H

// Qt includes
#include <qstring.h>
#include <qstrlist.h>

// ark includes
#include "arch.h"
#include "arkdata.h"
#include "arkprocess.h"
#include "filelistview.h"

class ZipArch : public Arch {

public:
	ZipArch(ArkData *data);
	virtual ~ZipArch();
	virtual unsigned char setOptions( bool p, bool l, bool o );
	virtual void openArch( QString, FileListView * flw );
	virtual void createArch( QString );
	virtual int addFile( QStrList *);
	virtual void extractTo( QString );
	virtual void extraction();
	virtual const QStrList *getListing();
	virtual QString unarchFile( int , QString );
	virtual void deleteFile( int );
private:
	QStrList *listing;
	ArkProcess archProcess;
	ArkData *data;
	bool perms, tolower, overwrite;
};

#endif /* ZIPARCH_H */
