#ifndef LHAARCH_H
#define LHAARCH_H

#include <qstring.h>
#include <qstrlist.h>
#include <qobject.h>
#include "arch.h"
#include "arkprocess.h"

class LhaArch : public Arch {

public:
	LhaArch();
	virtual ~LhaArch();
	virtual unsigned char setOptions( bool p, bool l, bool o );
	virtual void openArch( QString );
	virtual void createArch( QString );
	virtual int addFile( QStrList *);
	virtual void extractTo( QString );
	virtual void onlyUpdate( bool );
	virtual void addPath( bool );
	virtual const QStrList *getListing();
	virtual QString unarchFile( int , QString );
	virtual void deleteFile( int );
	virtual const char *getHeaders() { return klocale->translate("Permissions\tOwner/Group\tPacked      \tSize       \tRatio\tCRC   \tTimeStamp     \tName\t"); };

private:
	QStrList *listing;
	bool onlyupdate;
	bool storefullpath;
	ArkProcess archProcess;
};

#endif /* ARCH_H */
