/*
  Copyright (C)
 
  2001: Macadamian Technologies Inc (author: Jian Huang, jian@macadamian.com)
 
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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
*/

#include "ark_part.h"
#include "arksettings.h"
#include "arkwidget.h"

#include <kdebug.h>
#include <kpopupmenu.h>
#include <kaboutdata.h>

#include <qfile.h>

#include "arkfactory.h"

KAboutData *ArkPart::createAboutData()
{
    KAboutData *about = new KAboutData("ark", I18N_NOOP("ark"),
                                       "1.0",
                                       I18N_NOOP("Ark KParts Component"),
                                       KAboutData::License_GPL,
                                       "(c) 1997-2001, The Various Ark Developers");
    about->addAuthor("Robert Palmbos",0, "palm9744@kettering.edu");
    about->addAuthor("Francois-Xavier Duranceau",0, "duranceau@kde.org");
    about->addAuthor("Corel Corporation (author: Emily Ezust)",0,
                     "emilye@corel.com");
    about->addAuthor("Corel Corporation (author: Michael Jarrett)", 0,
                     "michaelj@corel.com");
    about->addAuthor("Jian Huang");
    about->addAuthor( "Roberto Teixeira", 0, "maragato@kde.org" );

    return about;
}



ArkPart::ArkPart( QWidget *parentWidget, const char *widgetName, QObject *parent,
                  const char *name, const QStringList &, bool readWrite )
        : KParts::ReadWritePart(parent, name),
        m_ArchivePopupEnabled( true )
{
    kdDebug()<<"ArkPart::ArkPart"<<endl;
    setInstance(ArkFactory::instance());
    awidget = new  ArkWidget( parentWidget, widgetName );

    setWidget(awidget);
    connect( awidget, SIGNAL( fixActions() ), this, SLOT( fixEnables() ) );
    connect( awidget, SIGNAL( disableAllActions() ), this, SLOT( disableActions() ) );
    connect( awidget, SIGNAL( signalFilePopup( const QPoint& ) ), this, SLOT( slotFilePopup( const QPoint& ) ) );
    connect( awidget, SIGNAL( signalArchivePopup( const QPoint& ) ), this, SLOT( slotArchivePopup( const QPoint& ) ) );
    connect( awidget, SIGNAL( setWindowCaption( const QString & ) ), this, SIGNAL( setWindowCaption( const QString & ) ) );
    connect( awidget, SIGNAL( removeRecentURL( const QString & ) ), this, SIGNAL( removeRecentURL(  const QString & ) ) );
    connect( awidget, SIGNAL( addRecentURL( const QString & ) ), this, SIGNAL( addRecentURL(  const QString & ) ) );

    if( readWrite )
        setXMLFile( "ark_part.rc" );
    else
    {
        setArchivePopupEnabled( false );
        setXMLFile( "ark_part_readonly.rc" );
    }
    setReadWrite( readWrite );

    setupActions();
}

ArkPart::~ArkPart()
{}

void
ArkPart::setupActions()
{
    shellOutputAction  = new KAction(i18n("&View Shell Output"), 0, awidget,
                                     SLOT(edit_view_last_shell_output()), actionCollection(), "shell_output");

    addFileAction = new KAction(i18n("Add &File..."), "ark_addfile", 0, awidget,
                                SLOT(action_add()), actionCollection(), "addfile");

    addDirAction = new KAction(i18n("Add &Directory..."), "ark_adddir", 0, awidget,
                               SLOT(action_add_dir()), actionCollection(), "adddir");

    extractAction = new KAction(i18n("E&xtract..."), "ark_extract", 0, awidget,
                                SLOT(action_extract()),	actionCollection(), "extract");

    deleteAction = new KAction(i18n("De&lete"), "ark_delete", 0, awidget,
                               SLOT(action_delete()), actionCollection(), "delete");

    viewAction = new KAction(i18n("to view something","&View"), "ark_view", 0, awidget,
                             SLOT(action_view()), actionCollection(), "view");


    openWithAction = new KAction(i18n("&Open With..."), 0, awidget,
                                 SLOT(slotOpenWith()), actionCollection(), "open_with");


    editAction = new KAction(i18n("Edit &With..."), 0, awidget,
                             SLOT(action_edit()), actionCollection(), "edit");

    selectAction =  new KAction(i18n("&Select..."), 0, awidget,
                                SLOT(edit_select()),	actionCollection(), "select");

    selectAllAction = KStdAction::selectAll(awidget,
                                            SLOT(edit_selectAll()),	actionCollection(), "select_all");

    deselectAllAction =  new KAction(i18n("&Deselect All"), 0, awidget,
                                     SLOT(edit_deselectAll()), actionCollection(), "deselect_all");

    invertSelectionAction = new KAction(i18n("&Invert Selection"), 0, awidget,
                                        SLOT(edit_invertSel()), actionCollection(), "invert_selection");

    KStdAction::preferences(awidget, SLOT(options_dirs()), actionCollection());

    initialEnables();
}


void ArkPart::fixEnables()
{
    bool bHaveFiles = ( awidget->getNumFilesInArchive() > 0 );
    bool bReadOnly = false;
    bool bAddDirSupported = true;
    QString extension;
    if ( awidget->archiveType() == ZOO_FORMAT || awidget->archiveType() == AA_FORMAT
            || awidget->archiveType() == COMPRESSED_FORMAT)
        bAddDirSupported = false;

    if (awidget->archive())
        bReadOnly = awidget->archive()->isReadOnly();

    selectAction->setEnabled(bHaveFiles);
    selectAllAction->setEnabled(bHaveFiles);
    deselectAllAction->setEnabled(bHaveFiles);
    invertSelectionAction->setEnabled(bHaveFiles);

    deleteAction->setEnabled(bHaveFiles && awidget->numSelectedFiles() > 0
                             && awidget->archive() && !bReadOnly);
    addFileAction->setEnabled(awidget->isArchiveOpen() &&
                              !bReadOnly);
    addDirAction->setEnabled(awidget->isArchiveOpen() &&
                             !bReadOnly && bAddDirSupported);
    extractAction->setEnabled(bHaveFiles);

    bool b = ( bHaveFiles && awidget->numSelectedFiles() == 1 );
    viewAction->setEnabled( b );
    openWithAction->setEnabled( b );
    editAction->setEnabled( b );
    emit fixActionState( bHaveFiles );
}

void ArkPart::initialEnables()
{
    selectAction->setEnabled(false);
    selectAllAction->setEnabled(false);
    deselectAllAction->setEnabled(false);
    invertSelectionAction->setEnabled(false);

    viewAction->setEnabled(false);

    deleteAction->setEnabled(false);
    extractAction->setEnabled(false);
    addFileAction->setEnabled(false);
    addDirAction->setEnabled(false);
    openWithAction->setEnabled(false);
    editAction->setEnabled(false);
}

void ArkPart::disableActions()
{
    selectAction->setEnabled(false);
    selectAllAction->setEnabled(false);
    deselectAllAction->setEnabled(false);
    invertSelectionAction->setEnabled(false);

    viewAction->setEnabled(false);
    deleteAction->setEnabled(false);
    extractAction->setEnabled(false);
    addFileAction->setEnabled(false);
    addDirAction->setEnabled(false);
    openWithAction->setEnabled(false);
    editAction->setEnabled(false);

}

bool ArkPart::openFile()
{
    KURL url;
    url.setPath( m_file );
    if( !QFile::exists( m_file ) )
    {
        emit setWindowCaption(  QString::null );
        emit removeRecentURL( m_file );
        return false;
    }
    emit addRecentURL( url.prettyURL() );
    awidget->file_open( url );
    return true;
}

bool ArkPart::saveFile()
{
    KURL url;
    url.setPath(  m_file );
    if ( awidget->allowedArchiveName( url ) )
        return awidget->file_save_as( url );
    else
        return false;
}

bool ArkPart::closeURL()
{
    awidget->file_close();
    return ReadWritePart::closeURL();
}

void ArkPart::setArchivePopupEnabled ( const bool b )
{
    if ( b==m_ArchivePopupEnabled )
        return;
    if( b )
    {
        connect( awidget, SIGNAL( signalArchivePopup( const QPoint& ) ), this,
                 SLOT( slotArchivePopup( const QPoint& ) ) );
    }

    else
    {
        disconnect( awidget, SIGNAL( signalArchivePopup( const QPoint& ) ), this,
                    SLOT( slotArchivePopup( const QPoint& ) ) );
    }
    m_ArchivePopupEnabled = b;
}

void ArkPart::slotFilePopup( const QPoint &pPoint )
{
    static_cast<KPopupMenu *>(factory()->container("file_popup", this))->popup(pPoint);
}

void ArkPart::slotArchivePopup( const QPoint &pPoint )
{
    static_cast<KPopupMenu *>(factory()->container("archive_popup", this))->popup(pPoint);
}

void ArkPart::slotSaveProperties()
{
    awidget->settings()->writeConfiguration();

    kdDebug(1601) << "-saveProperties (exit)" << endl;
}


#include "ark_part.moc"
