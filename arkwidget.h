#ifndef ARKWIDGET_H
#define ARKWIDGET_H

// Qt includes
#include <qdir.h>
#include <qwidget.h>
#include <qlistbox.h>
#include <qmenubar.h>
#include <drag.h>
#include <qframe.h>

// KDE includes
#include <ktopwidget.h>
#include <ktoolbar.h>
#include <kstatusbar.h>
#include <kfm.h>
#include <kpopmenu.h>
#include <ktablistbox.h>
#include <kaccel.h>

#include "karchive.h"
#include "adddlg.h"
#include "arkdata.h"

#define ARK_WARNING i18n("ark - warning")
#define ARK_ERROR i18n("ark - error")

class ArkWidget : public KTMainWindow {

Q_OBJECT

public:
	ArkWidget( QWidget *parent=0, const char *name=0 );
	~ArkWidget();

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
	void options_keyconf();
	void timeout();
	
protected:
	static QList<ArkWidget> *windowList;
	void closeEvent( QCloseEvent * );


private:
	bool addonlynew;
	bool storefullpath;
	KArchive *arch;
	KTabListBox *lb;
	QStrList *listing;
	QStrList *flisting;
	KToolBar *tb;
	KMenuBar *menu;
	QDir *fav;
	QFrame *f_main;
	KStatusBar *sb;
	QString tmpdir;
	KFM *kfm;
	bool contextRow;
	KPopupMenu *pop;
	ArkData *data;
	QTimer *statusBarTimer;
	KAccel *accelerators;
	void writeStatus(const QString text);
	void clearCurrentArchive();
	void arkWarning(const QString msg);
	void setupMenus();
};

#endif /* ARKWIDGET_H*/
