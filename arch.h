#ifndef ARCH_H
#define ARCH_H

// Qt includes
#include <qstring.h>
#include <qstrlist.h>

// ark includes
#include "filelistview.h"

class Arch : public QObject {

Q_OBJECT
	
public:
	Arch();
	virtual ~Arch();
	virtual unsigned char setOptions( bool p, bool l, bool o ) = 0;
	virtual void openArch( QString, FileListView *flw ) = 0;
	virtual void createArch( QString ) = 0;
	virtual int addFile( QStrList *) = 0;
	virtual void extractTo( QString ) = 0;
	virtual void extraction() = 0;
	virtual const QStrList *getListing() = 0;
	virtual QString unarchFile( int , QString ) = 0;
	virtual void deleteFile( int ) = 0;
//	virtual void newProgressDialog( long int, long int );
//	virtual int isCanceled();
//	virtual const char *getName() { return archname; };

protected:
	QString archname;

private:
//	bool canceled;
//	int total;
//	QDialog *pd;
//	KProgress *kp;

};

#endif /* ARCH_H */
