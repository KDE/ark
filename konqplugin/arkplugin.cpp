/* This file is part of the KDE project

   Copyright (C) 2003 Georg Robbers <Georg.Robbers@urz.uni-hd.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; version 2
   of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "arkplugin.h"

#include <kapplication.h>
#include <kstandarddirs.h>
#include <kmimetype.h>
#include <kdebug.h>
#include <kaction.h>
#include <kinstance.h>
#include <klocale.h>
#include <konq_popupmenu.h>
#include <kpopupmenu.h>
#include <kgenericfactory.h>
#include <kurl.h>

#include <qdir.h>
#include <qcstring.h>
#include <qobject.h>

typedef KGenericFactory<ArkMenu, KonqPopupMenu> ArkMenuFactory;
K_EXPORT_COMPONENT_FACTORY( libarkplugin, ArkMenuFactory("arkplugin") );

ArkMenu::ArkMenu( KonqPopupMenu * popupmenu, const char *name, const QStringList& /* list */ )
                : KonqPopupMenuPlugin( popupmenu, name)
{
    if ( QCString( kapp->name() ) == "kdesktop" && !kapp->authorize("editable_desktop_icons" ) )
        return;

    m_conf = new KConfig( "arkrc" );
    m_conf->setGroup( "ArkPlugin" );

    if ( !m_conf->readBoolEntry( "Enable", true ) )
	    return;
    
    KGlobal::locale()->insertCatalogue("arkplugin");
    extMimeTypes();
    m_list = popupmenu->fileItemList();
    unsigned int itemCount = m_list.count();
    KFileItemListIterator it( m_list );
    KFileItem * item;
    bool hasArchives = false;
    bool hasOther = false;
    while ( ( item = it.current() ) != 0 )
    {
        ++it;
        if ( m_extractMimeTypes.contains( item->mimetype() ) )
        {
            hasArchives = true;
        }
        else
        {
            hasOther = true;
        }

        if ( hasArchives && hasOther )
            break;
    }

    QString ext;
    KActionMenu * actionMenu;
    KAction * action;
    if ( hasOther )
    {
        compMimeTypes();
        actionMenu = new KActionMenu( i18n( "Compress" ), "ark", actionCollection(), "ark_compress_menu"  );
        m_ext = m_conf->readEntry( "LastExtension", ".tar.gz" );
        if ( itemCount == 1 )
        {
            item = m_list.first();
            m_name = m_list.first()->name();
            action = new KAction( i18n( "Compress as %1" ).arg( m_name + m_ext ), 0, this,
                                    SLOT( slotCompressAsDefault() ), actionCollection() );
        }
        else
        {
            action = new KAction( KMimeType::mimeType( m_conf->readEntry(
                                    "LastMimeType", "application/x-tgz" ) )->comment(),
                                    0, this, SLOT( slotCompressAsDefault() ), actionCollection() );
        }
        actionMenu->insert( action );

        m_compAsMenu = new KActionMenu( i18n( "Compress as" ), actionCollection(), "arkcmpasmnu" );
        actionMenu->insert( m_compAsMenu );


        m_addToMenu = new KActionMenu( i18n( "Add to" ), actionCollection(), "arkaddtomnu" );
        if ( m_list.first()->url().isLocalFile() )
            actionMenu->insert( m_addToMenu );

        connect( m_compAsMenu->popupMenu(), SIGNAL( aboutToShow() ),
                this, SLOT( slotPrepareCompAsMenu() ) );
        connect( m_addToMenu->popupMenu(), SIGNAL( aboutToShow() ),
                this, SLOT( slotPrepareAddToMenu() ) );


        action = new KAction( i18n( "Add to Archive..." ), 0, this,
                                SLOT( slotAdd() ), actionCollection() );
        actionMenu->insert( action );
        addAction( actionMenu );
    }

    if ( !hasOther && hasArchives )
    {
        actionMenu = new KActionMenu( i18n( "Extract" ), "ark", actionCollection(), "ark_extract_menu"  );

        action = new KAction( i18n( "Extract Here" ), 0, this,
                                SLOT( slotExtractHere() ), actionCollection() );
        actionMenu->insert( action );
        // stolen from arkwidget.cpp
        if ( itemCount == 1 )
        {
            QString targetName = m_list.first()->name();
            stripExtension( targetName );
            action = new KAction( i18n( "Extract to %1" ).arg( targetName ), 0, this,
                                    SLOT( slotExtractToSubfolders() ), actionCollection() );
        }
        else
        {
            action = new KAction( i18n( "Extract to Subfolders" ), 0, this,
                                    SLOT( slotExtractToSubfolders() ), actionCollection() );
        }
        actionMenu->insert( action );
        action = new KAction( i18n( "Extract to..." ), 0 , this,
                                SLOT( slotExtractTo() ), actionCollection() );
        actionMenu->insert( action );
        addAction( actionMenu );
    }
    addSeparator();
}

ArkMenu::~ArkMenu()
{
}

void ArkMenu::slotPrepareCompAsMenu()
{
    disconnect( m_compAsMenu->popupMenu(), SIGNAL( aboutToShow() ),
                this, SLOT( slotPrepareCompAsMenu() ) );

    KAction * action;
    QString ext;
    QStringList newExt;
    unsigned int counter = 0;
    QCString actionName;
    QStringList::Iterator eit;
    QStringList::Iterator mit;
    mit = m_archiveMimeTypes.begin();
    for ( ; mit != m_archiveMimeTypes.end(); ++mit )
    {
        newExt = KMimeType::mimeType(*mit)->patterns();
        eit = newExt.begin();
        (*eit).remove( '*' );
        if ( *eit == ".tar.bz" )                   // tbz mimetype, has tar.bz as first entry :}
            *eit = ".tar.bz2";
        if ( m_list.count() == 1 )
        {
            action = new KAction( m_name + (*eit), 0, this,
                    SLOT( slotCompressAs() ), actionCollection(), actionName.setNum( counter ) );
            m_compAsMenu->insert( action );
        }
        else
        {
            ext = KMimeType::mimeType(*mit)->comment();
            action = new KAction( ext, 0, this,
                    SLOT( slotCompressAs() ), actionCollection(), actionName.setNum( counter ) );
            m_compAsMenu->insert( action );
        }

        ++counter;
        ++eit;
        while( eit != newExt.end() )
        {
            (*eit).remove( '*' );
            ++eit;
            ++counter;
        }
        m_extensionList += newExt;
    }
}

void ArkMenu::slotPrepareAddToMenu()
{
    disconnect( m_addToMenu->popupMenu(), SIGNAL( aboutToShow() ),
                this, SLOT( slotPrepareAddToMenu() ) );


    if ( m_extensionList.isEmpty() ) // is filled in slotPrepareCompAsMenu
        slotPrepareCompAsMenu();

    unsigned int counter = 0;
    KAction * action;
    QCString actionName;
    QStringList::Iterator mit;
    KURL archive;
    QDir dir( m_list.first()->url().directory() );
    QStringList entries = dir.entryList();
    QStringList::Iterator uit = entries.begin();
    for ( ; uit != entries.end(); ++uit )
    {
        for ( mit = m_extensionList.begin(); mit != m_extensionList.end(); ++mit )
            if ( (*uit).endsWith(*mit) )
            {
                action = new KAction( *uit, 0, this, SLOT( slotAddTo() ),
                        actionCollection(), actionName.setNum( counter ) );
                m_addToMenu->insert( action );
                archive.setPath( *uit );
                m_archiveList << archive;
                counter++;
                break;
            }
    }
}

void ArkMenu::compMimeTypes()
{
    bool havegz = false;
    if ( KStandardDirs::findExe( "gzip" ) != QString::null && m_conf->readBoolEntry( "UseGz", true ) )
    {
        havegz = true;
        m_archiveMimeTypes << "application/x-gzip";
    }

    bool havebz2 = false;
    if ( KStandardDirs::findExe( "bzip2" ) != QString::null && m_conf->readBoolEntry( "UseBzip2", true ) )
    {
        havebz2 = true;
        m_archiveMimeTypes << "application/x-bzip2";
    }

    bool havelzop = false;
    if ( KStandardDirs::findExe( "lzop" ) != QString::null && m_conf->readBoolEntry( "UseLzop", false ) )
    {
        havelzop = true;
        m_archiveMimeTypes << "application/x-lzop";
    }

    if ( KStandardDirs::findExe( "tar" ) != QString::null && m_conf->readBoolEntry( "UseTar", true ) )
    {
        m_archiveMimeTypes << "application/x-tar";
        if ( havegz )
            m_archiveMimeTypes << "application/x-tgz";
        if ( havebz2 )
            m_archiveMimeTypes << "application/x-tbz";
        if ( havelzop )
            m_archiveMimeTypes << "application/x-tzo";
    }

    if ( KStandardDirs::findExe( "lha" ) != QString::null && m_conf->readBoolEntry( "UseLha", false ) )
        m_archiveMimeTypes << "application/x-lha";

    if ( KStandardDirs::findExe( "zip" ) != QString::null && m_conf->readBoolEntry( "UseZip", true ) )
    {
        m_archiveMimeTypes << "application/x-zip";
	
	if ( m_conf->readBoolEntry( "UseJar", false ) )
		m_archiveMimeTypes << "application/x-jar";
    }

    if ( KStandardDirs::findExe( "rar" ) != QString::null && m_conf->readBoolEntry( "UseRar", true ) )
        m_archiveMimeTypes << "application/x-rar";

    if ( KStandardDirs::findExe( "zoo" ) != QString::null && m_conf->readBoolEntry( "UseZoo", false ) )
        m_archiveMimeTypes << "application/x-zoo";

    if ( KStandardDirs::findExe( "compress" ) != QString::null && m_conf->readBoolEntry( "UseCompress", false ) )
        m_archiveMimeTypes << "application/x-compress";

    if ( KStandardDirs::findExe( "bzip" ) != QString::null && m_conf->readBoolEntry( "UseBzip", false ) )
        m_archiveMimeTypes << "application/x-bzip";

    if ( KStandardDirs::findExe( "ar" ) != QString::null && m_conf->readBoolEntry( "UseAr", false ) )
        m_archiveMimeTypes << "application/x-archive";
}

void ArkMenu::extMimeTypes()
{
    bool havegz = false;
    if ( KStandardDirs::findExe( "gunzip" ) != QString::null )
    {
        havegz = true;
        m_extractMimeTypes << "application/x-gzip";
    }

    bool havebz2 = false;
    if ( KStandardDirs::findExe( "bunzip2" ) != QString::null )
    {
        havebz2 = true;
        m_extractMimeTypes << "application/x-bzip2";
    }

    bool havelzop = false;
    if ( KStandardDirs::findExe( "lzop" ) != QString::null )
    {
        havelzop = true;
        m_extractMimeTypes << "application/x-lzop";
    }

    if ( KStandardDirs::findExe( "tar" ) != QString::null )
    {
        m_extractMimeTypes << "application/x-tar";
        if ( havegz )
            m_extractMimeTypes << "application/x-tgz";
        if ( havebz2 )
            m_extractMimeTypes << "application/x-tbz";
        if ( havelzop )
            m_extractMimeTypes << "application/x-tzo";
    }

    if ( KStandardDirs::findExe( "lha" ) != QString::null )
        m_extractMimeTypes << "application/x-lha";

    if ( KStandardDirs::findExe( "zip" ) != QString::null )
        m_extractMimeTypes << "application/x-zip" << "application/x-jar";

    if ( KStandardDirs::findExe( "unrar" ) != QString::null )
        m_extractMimeTypes << "application/x-rar";

    if ( KStandardDirs::findExe( "zoo" ) != QString::null )
        m_extractMimeTypes << "application/x-zoo";

    if ( KStandardDirs::findExe( "uncompress" ) != QString::null )
        m_extractMimeTypes << "application/x-compress";

    if ( KStandardDirs::findExe( "bunzip" ) != QString::null )
        m_extractMimeTypes << "application/x-bzip";

    if ( KStandardDirs::findExe( "ar" ) != QString::null )
        m_extractMimeTypes << "application/x-archive";
}

void ArkMenu::stripExtension( QString & name )
{
    QStringList patternList = KMimeType::findByPath( name )->patterns();
    QStringList::Iterator it = patternList.begin();
    QString ext;
    for ( ; it != patternList.end(); ++it )
    {
        ext = (*it).remove( '*' );
        if ( name.endsWith( ext ) )
        {
            name = name.left( name.findRev( ext ) ) + '/';
            break;
        }
    }
}

void ArkMenu::slotCompressAs()
{
    QCString name;
    QString extension, mimeType;
    KURL target;
    KFileItemListIterator it( m_list );
    KFileItem * item;
    while (  ( item = it.current() ) != 0 )
    {
        ++it;
        name = sender()->name();
        target = item->url();
        target.setPath( target.path( -1 ) + m_extensionList[ name.toUInt() ] );
        compressAs( item->url(), target );
    }

    extension = m_extensionList[ name.toUInt() ];
    m_conf->setGroup( "ArkPlugin" );
    m_conf->writeEntry( "LastExtension", extension );

    QStringList extensions;
    QStringList::Iterator eit;
    QStringList::Iterator mit = m_archiveMimeTypes.begin();
    bool done = false;
    for ( ; mit != m_archiveMimeTypes.end() && !done; ++mit )
    {
        extensions = KMimeType::mimeType(*mit)->patterns();
        eit = extensions.begin();
        for ( ; eit != extensions.end(); ++eit )
        {
            (*eit).remove( '*' );
            if ( (*eit) == extension )
            {
                m_conf->writeEntry( "LastMimeType", *mit );
                done = true;
                break;
            }
        }
    }
}

void ArkMenu::slotCompressAsDefault()
{
    KFileItemListIterator it( m_list );
    KFileItem * item;
    KURL name;
    while (  ( item = it.current() ) != 0 )
    {
        ++it;
        name = item->url();
        name.setPath( name.path( -1 ) + m_ext );
        compressAs( item->url(), name );
    }
}

// make work for URLs
void ArkMenu::compressAs( const KURL & name, const KURL & compressed )
{
    QStringList args;
    args << "--add-to" << name.prettyURL() << compressed.prettyURL();
    kapp->kdeinitExec( "ark", args );
}

void ArkMenu::slotAddTo()
{
    QCString name = sender()->name();
    QStringList args;
    args << "--add-to";
    KFileItemListIterator it( m_list );
    KFileItem * item;
    while (  ( item = it.current() ) != 0 )
    {
        ++it;
        args << item->url().prettyURL();
    }
    args << m_archiveList[ name.toUInt() ].prettyURL();
    kapp->kdeinitExec( "ark", args );
}

void ArkMenu::slotAdd()
{
    QStringList args;
    args << "--add";
    KFileItemListIterator it( m_list );
    KFileItem * item;
    while (  ( item = it.current() ) != 0 )
    {
        ++it;
        args << item->url().prettyURL();
    }
    kapp->kdeinitExec( "ark", args );
}

void ArkMenu::slotExtractHere()
{
    QStringList args;
    KFileItemListIterator it( m_list );
    KFileItem * item;
    while (  ( item = it.current() ) != 0 )
    {
        args.clear();
        ++it;
        KURL targetDirectory = item->url();
        targetDirectory.setPath( targetDirectory.directory() );
        args << "--extract-to"  << targetDirectory.prettyURL() << item->url().prettyURL();
        kapp->kdeinitExec( "ark", args );
    }
}

void ArkMenu::slotExtractToSubfolders()
{
    QStringList args;
    QString dirName;
    KURL targetDir;
    KFileItemListIterator it( m_list );
    KFileItem * item;
    while (  ( item = it.current() ) != 0 )
    {
        args.clear();
        ++it;
        targetDir = item->url();
        dirName = targetDir.path();
        stripExtension( dirName );
        targetDir.setPath( dirName );
        args << "--extract-to"  << targetDir.prettyURL() << item->url().prettyURL();
        kapp->kdeinitExec( "ark", args );
    }
}

void ArkMenu::slotExtractTo()
{
    QStringList args;
    KFileItemListIterator it( m_list );
    KFileItem * item;
    while (  ( item = it.current() ) != 0 )
    {
        ++it;
        args << "--extract"  << item->url().prettyURL();
        kapp->kdeinitExec( "ark", args );
    }
}

#include "arkplugin.moc"
