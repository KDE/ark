#ifndef ARCH_H
#define ARCH_H

#include <qstring.h>
#include <qstrlist.h>
#include <qobject.h>
#include <qdialog.h>
#include <qpushbt.h>
#include <klocale.h>
#include <kapp.h>
#include <kprogress.h>

class Arch : public QObject {
	Q_OBJECT
	
public slots:
	void setProgress( long int );

public:
	Arch();
	virtual ~Arch();
	virtual unsigned char setOptions( bool p, bool l, bool o );
	virtual void openArch( QString ) = 0;
	virtual void createArch( QString );
	virtual int addFile( QStrList *);
	virtual void extractTo( QString );
	virtual void onlyUpdate( bool );
	virtual void addPath( bool );
	virtual const QStrList *getListing();
	virtual QString unarchFile( int , QString );
	virtual void deleteFile( int );
	virtual void newProgressDialog( long int, long int );
	virtual int isCanceled();
	virtual const char *getHeaders();

protected:
	bool perms, tolower, overwrite;

private:
	bool canceled;
	int total;
	QDialog *pd;
	KProgress *kp;

private slots:
	void cancel();
};

#endif /* ARCH_H */
