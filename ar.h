#ifndef ARARCH_H
#define ARARCH_H

/*
#ifdef __cplusplus
extern "C" {
#endif

void strshort( char *start, int num_rem );

#ifdef __cplusplus
}
#endif
*/


// Qt includes
#include <qstring.h>
#include <qstrlist.h>

// ark includes
#include "arch.h"
#include "arksettings.h"
#include "filelistview.h"

#define UNSUPDIR 1

class ArArch : public Arch {

public:
	ArArch( ArkSettings *d );
	virtual ~ArArch();
	virtual unsigned char setOptions( bool p, bool l, bool o );
	virtual void openArch( QString, FileListView *flw );
	virtual void createArch( QString );
	virtual int addFile( QStringList *);
	virtual void extractTo( QString );
	virtual const QStringList *getListing();
	virtual QString unarchFile( int , QString );
	virtual void deleteFile( int );

private:
	ArkSettings *data;
	QStringList *listing;
	bool perms, tolower, overwrite;
	void strshort( char *start, int num_rem );
};

#endif /* ARARCH_H */
