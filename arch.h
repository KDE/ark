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
	virtual unsigned char setOptions( bool p, bool l, bool o ) = 0;
	virtual void openArch( QString ) = 0;
	virtual void createArch( QString ) = 0;
	virtual int addFile( QStrList *) = 0;
	virtual void extractTo( QString ) = 0;
	virtual void onlyUpdate( bool ) = 0;
	virtual void addPath( bool ) = 0;
	virtual const QStrList *getListing() = 0;
	virtual QString unarchFile( int , QString ) = 0;
	virtual void deleteFile( int ) = 0;
	virtual void newProgressDialog( long int, long int );
	virtual int isCanceled();
	virtual const char *getHeaders() = 0;
	virtual const char *getName() { return archname; };

protected:
	QString archname;
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
