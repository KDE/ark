#ifndef TAR_H
#define TAR_H

#include "arch.h"
#include <unistd.h>

class TarArch : public Arch {

public:
	TarArch();
	virtual ~TarArch();
	virtual unsigned char setOptions( bool p, bool l, bool o );
	virtual void openArch( QString );
	virtual void createArch( QString );
	virtual int addFile( QStrList *);
	virtual void extractTo( QString );
	virtual void onlyUpdate( bool );
	virtual void addPath( bool );
	virtual const QStrList *getListing();
	virtual QString unarchFile( int, QString );
	virtual void deleteFile( int );
	virtual const char *getHeaders() { return klocale->translate( "Permissions\tOwner/Group\tSize        \tTimeStamp\tName\t" ); };

private:
	bool storefullpath;
	bool onlyupdate;
	QStrList *listing;
	QString archname;
	QString tmpfile;
	bool compressed;

	int updateArch();
	void createTmp();
};

#endif /* TAR_H */
