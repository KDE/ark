#ifndef KARCH_H
#define KARCH_H

#include <qstring.h>
#include <qstrlist.h>
#include <qobject.h>
#include "arch.h"
#include "tar.h"
#include "zip.h"
#include "lha.h"
#include "ar.h"

class KZipArch {

public:
	KZipArch( QString te="tar" );
	~KZipArch();
	bool openArch( QString name );
	bool createArch( QString file );
	int addFile( QStrList *urls );
	void extractTo( QString dir );
	void onlyUpdate( bool );
	void addPath( bool );
	const QStrList *getListing();
	QString unarchFile( int index, QString dest );
	void deleteFile( int indx );
	unsigned char setOptions( bool p, bool l, bool o );
	const char *getHeaders() { return arch->getHeaders(); };
	const char *getName() { return arch->getName(); };
	

private:
	enum ArchType{ Tar, Zip, AA, Lha };
	int getArchType( QString );
	Arch *arch;
	QString tar_exe;

};

#endif /* KARCH_H */
