#ifndef ARARCH_H
#define ARARCH_H

#include <qstring.h>
#include <qstrlist.h>
#include <qobject.h>
#include "arch.h"
#include "kzipprocess.h"

class ArArch : public Arch {

public:
	ArArch();
	virtual ~ArArch();
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
	virtual const char *getHeaders() { return klocale->translate( "Permissions\tOwner/Group\tSize      \tTimeStamp         \tName\t"); };
private:
	KZipProcess archProcess;
	QStrList *listing;
	bool onlyupdate;
	bool storefullpath;
};

#endif /* AARCH_H */
