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
	void doPopup( QListViewItem *item );
	void fileDrop( KDNDDropZone * );
	
	void file_new();
	void file_newWindow();
	void file_open();
	void file_openRecent( int );
	void file_close();
	void file_quit();
	
	void getAddOptions();
	void edit_add();
	void edit_view();
	void edit_delete();
	void edit_extract();
	void edit_selectAll();
	void edit_deselectAll();
	void edit_invertSel();

	void options_dirs();
	void options_keys();
	void options_general();
	void testdlg();

	void help();
			
	void showFavorite();
	void showZip( QString name );
	void timeout();
		
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
	int idSelectAll, idDeselectAll, idInvertSel;
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

	void deleteFile( int );
	void showFile( int, int col=0 );
};

#endif /* ARKWIDGET_H*/
