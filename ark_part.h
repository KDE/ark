/*
  Copyright (C)

  2001: Macadamian Technologies Inc (author: Jian Huang, jian@macadamian.com)
  2005: Henrique Pinto <henrique.pinto@kdemail.net>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef ARK_PART_H
#define ARK_PART_H

#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <kparts/statusbarextension.h>
#include <kparts/factory.h>
#include <kaction.h>
#include <kprogress.h>

#include <qlabel.h>

class KAboutData;
class KPushButton;

class ArkWidget;

namespace KIO
{
	class Job;
}


class ArkBrowserExtension: public KParts::BrowserExtension
{
    Q_OBJECT
public:
    ArkBrowserExtension( KParts::ReadOnlyPart * parent, const char * name = 0L );
public slots:
    void slotOpenURLRequested( const KURL & url );
};

class ArkStatusBarExtension: public KParts::StatusBarExtension
{
    Q_OBJECT
public:
    ArkStatusBarExtension( KParts::ReadWritePart * parent );
    ~ArkStatusBarExtension();

    void setProgress( unsigned long progress );
    KPushButton* cancelButton() const { return m_cancelButton; }

public slots:
    void slotSetStatusBarSelectedFiles( const QString & text );
    void slotSetStatusBarText( const QString & text );
    void slotSetBusy( const QString & text, bool showCancelButton = false, bool detailedProgress = false );
    void slotSetReady();
    void slotProgress();

protected:
    void setupStatusBar();

private:
    bool m_bBusy;
    QLabel *m_pStatusLabelSelect; // How many files are selected
    QLabel *m_pStatusLabelTotal;  // How many files in archive
    QLabel *m_pBusyText;
    KPushButton *m_cancelButton; // Cancel an operation
    KProgress *m_pProgressBar;
    QTimer *m_pTimer;
};


class ArkPart: public KParts::ReadWritePart
{
    Q_OBJECT
public:
    ArkPart( QWidget *parentWidget, const char *widgetName, QObject *parent,
             const char *name, const QStringList &, bool readWrite );
    virtual ~ArkPart();

    static KAboutData* createAboutData();

public slots:
    void fixEnables();//rename to slotFixEnables()...
    void disableActions();
    void slotFilePopup( const QPoint & pPoint );
    void file_save_as();
    bool saveFile();
    bool openURL( const KURL & url );
    bool closeURL();
    void transferStarted( KIO::Job * );
    void transferCompleted();
    void transferCanceled( const QString& errMsg );
    void progressInformation( KIO::Job *, unsigned long );
    void cancelTransfer();

signals:
    void fixActionState( const bool & bHaveFiles );
    void removeRecentURL( const KURL & url );
    void addRecentURL( const KURL & url );

protected:
    virtual bool openFile();  //Opening an archive file
    bool closeArchive();
    void setupActions();
    void initialEnables();
    void init();

private:
    ArkWidget  *awidget;
    ArkBrowserExtension *m_ext;
    ArkStatusBarExtension *m_bar;

    KAction *saveAsAction;
    KAction *addFileAction;
    KAction *addDirAction;
    KAction *extractAction;
    KAction *deleteAction;
    KAction *selectAllAction;
    KAction *viewAction;
    KAction *helpAction;
    KAction *openWithAction;
    KAction *deselectAllAction;
    KAction *invertSelectionAction;
    KAction *editAction;

    // the following have different enable rules from the above KActions
    KAction *popupViewAction;
    KAction *popupOpenWithAction;
    KToggleAction *showSearchBar;

    KIO::Job *m_job;
};

#endif // ARK_PART_H
