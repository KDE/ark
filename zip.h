#ifndef ZIPARCH_H
#define ZIPARCH_H

#include <qstring.h>
#include <qstrlist.h>
#include <qobject.h>
#include "arch.h"
#include "kzipprocess.h"

class ZipArch : public Arch {

public:
	ZipArch();
	virtual ~ZipArch();
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
	virtual const char *getHeaders() { return klocale->translate( "Length     \tMethod\tSize         \tRatio\tDate     \tTime\tCRC-32\tName\t" ); };
private:
	QStrList *listing;
	bool onlyupdate;
	bool storefullpath;
	KZipProcess archProcess;
};

#endif /* ARCH_H */
