#ifndef TAR_H
#define TAR_H

#include <unistd.h>

// Qt includes
#include <qdir.h>
#include <qstring.h>
#include <qstrlist.h>

#include "arch.h"
#include "arkdata.h"
#include "arkprocess.h"
#include "filelistview.h"

class TarArch : public Arch {

public:
	TarArch( ArkData *d );
	virtual ~TarArch();
	virtual unsigned char setOptions( bool p, bool l, bool o );
	virtual void openArch( QString, FileListView *flw );
	virtual void createArch( QString );
	virtual int addFile( QStrList *);
	virtual void extractTo( QString );
	virtual void extraction();
	virtual const QStrList *getListing();
	virtual QString unarchFile( int, QString );
	virtual void deleteFile( int );
	QString getCompressor();
	QString getUnCompressor();

private:
	QStrList *listing;
	QString tmpfile;
	bool compressed;
	ArkData *data;
	ArkProcess archProcess;

	bool perms, tolower, overwrite;
	int updateArch();
	void createTmp();
};

#endif /* TAR_H */
