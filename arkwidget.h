#ifndef ARKWIDGET_H
#define ARKWIDGET_H

// Qt includes
#include <drag.h>
#include <qlist.h>
#include <qlistview.h>
#include <qpopupmenu.h>
#include <qstring.h>
#include <qstrlist.h>
#include <qtimer.h>
#include <qwidget.h>


// KDE includes
#include <ktmainwindow.h>
#include <kfm.h>
#include <kpopmenu.h>
#include <kaccel.h>

// ark includes
#include "ar.h"
#include "arch.h"
#include "adddlg.h"
#include "arkdata.h"
#include "filelistview.h"
// #include "lha.h"
#include "tar.h"
#include "zip.h"

#define ARK_WARNING i18n("ark - warning")
#define ARK_ERROR i18n("ark - error")

#define ARK_VERSION "0.5"

class ArkWidget : public KTMainWindow {

Q_OBJECT

public:
	ArkWidget( QWidget *parent=0, const char *name=0 );
	~ArkWidget();

public slots:
	void getTarExe();
	void doPopup( QListViewItem *item );
	void newWindow();
	void createZip();
	void fileDrop( KDNDDropZone * );
	void getAddOptions();
	void getFav();
	void openZip();
	void closeZip();
	void extract();
	void deleteFile();
	void deleteFile( int );
	void showFavorite();
	void quit();
	void showFile( int, int col=0 );
	void showFile();
	void help();
	void showZip( QString name );
	void options_keyconf();
	void timeout();
	void openRecent( int );
	void selectAll();
	void deselectAll();
	void configDirs();
	void testdlg();
		
protected:
	static QList<ArkWidget> *windowList;
	void closeEvent( QCloseEvent * );


private:
	enum ArchType{ TAR_FORMAT, ZIP_FORMAT, AA_FORMAT, LHA_FORMAT };

	Arch *arch;
	FileListView *archiveContent;
	QStrList *listing;
	QString tmpdir;
	KFM *kfm;
	KDNDDropZone *dz;
	bool contextRow;
	KPopupMenu *pop;
	ArkData *data;
	QTimer *statusBarTimer;
	KAccel *accelerators;
	QPopupMenu *editMenu, *recentPopup;
	int idExtract, idDelete, idAdd, idView;
        bool archiverMode;

	void writeStatus(const QString text);
	void clearCurrentArchive();
	
	void arkWarning(const QString msg);
	void arkError(const QString msg);
	
	void setupMenuBar();
	void setupStatusBar();
	void setupToolBar();
	void createRecentPopup();
	
	void newCaption(const QString& filename);
	void createFileListView();
	
	int getArchType(QString archname);
	bool createArchive(QString name);
	bool openArchive(QString name);
};

#endif /* ARKWIDGET_H*/
