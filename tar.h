#ifndef TAR_H
#define TAR_H

#include "arch.h"
#include <unistd.h>
#include "kzipprocess.h"

class TarArch : public Arch {

public:
	TarArch( QString );
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
	QString getCompressor();
	QString getUnCompressor();

private:
	bool storefullpath;
	bool onlyupdate;
	QStrList *listing;
	QString tmpfile;
	bool compressed;
	KZipProcess archProcess;
	QString tar_exe;

	int updateArch();
	void createTmp();
};

#endif /* TAR_H */
