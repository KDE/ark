#ifndef KZIP_H
#define  KZIP_H

#include <qdir.h>
#include <qwidget.h>
#include <qlistbox.h>
#include <ktoolbar.h>
#include <qmenubar.h>
#include <drag.h>
#include <ktopwidget.h>
#include <qframe.h>
#include <kstatusbar.h>
#include <kfm.h>
#include <kpopmenu.h>
#include "ktablistbox.h"
#include "karch.h"
#include "adddlg.h"

class KZipWidget : public KTopLevelWidget {

Q_OBJECT

public:
	KZipWidget( QWidget *parent=0, const char *name=0 );
	~KZipWidget();

public slots:
	void getTarExe();
	void doPopup( int, int );
	void newWindow();
	void createZip();
	void fileDrop( KDNDDropZone * );
	void getAddOptions();
	void getFav();
	void openZip();
	void closeZip();
	void extractZip();
	void extractFile();
	void extractFile( int );
	void deleteFile();
	void deleteFile( int );
	void showFavorite();
	void about();
	void quit();
	void showFile( int, int col=0 );
	void showFile();
	void help();
	void showZip( QString name );
	void setupHeaders();
	void aboutQt();
	
protected:
	static QList<KZipWidget> *windowList;
	void resizeEvent( QResizeEvent * );
	void closeEvent( QCloseEvent * );
	virtual void saveProperties( KConfig* );
	virtual void readProperties( KConfig* );


private:
	bool addonlynew;
	bool storefullpath;
	KZipArch *arch;
	KTabListBox *lb;
	QStrList *listing;
	QStrList *flisting;
	KToolBar *tb;
	KMenuBar *menu;
	QDir *fav;
	QString fav_dir;
	QString tar_exe; // why do people insist on having two tars?
	QFrame *f_main;
	KStatusBar *sb;
	QString tmpdir;
	KFM *kfm;
	bool contextRow;
	KPopupMenu *pop;
};

#endif /* KZIP_H*/
