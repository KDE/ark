#ifndef ZIPARCH_H
#define ZIPARCH_H

#include <qstring.h>
#include <qstrlist.h>
#include <qobject.h>

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
//	virtual void onlyUpdate( bool );
//	virtual void addPath( bool );
	virtual const QStrList *getListing();
	virtual QString unarchFile( int , QString );
	virtual void deleteFile( int );
private:
	QStrList *listing;
//	bool onlyupdate;
//	bool storefullpath;
	ArkProcess archProcess;
	ArkData *data;
};

#endif /* ARCH_H */
