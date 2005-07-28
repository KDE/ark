/*

  ark -- archiver for the KDE project

  Copyright (C)

  2004: Henrique Pinto <henrique.pinto@kdemail.net>
  2003: Helio Chissini de Castro <helio@conectiva.com>
  2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)

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

#include <qdir.h>

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kstandarddirs.h>

#include "sevenzip.h"
#include "arkwidget.h"
#include "settings.h"

SevenZipArch::SevenZipArch( ArkWidget *gui, const QString &filename )
  : Arch( gui, filename )
{
  // Check if 7z is available
  bool have_7z = !KGlobal::dirs()->findExe( "7z" ).isNull();

  if ( have_7z ) 
    m_archiver_program = m_unarchiver_program = "7z";  // Use 7z
  else
    m_archiver_program = m_unarchiver_program = "7za"; // Try 7za
    
  verifyUtilityIsAvailable( m_archiver_program );
  
  m_headerString = "------------------";
  
  m_repairYear = 5; m_fixMonth = 6; m_fixDay = 7; m_fixTime = 8;
  m_dateCol = 3;
  m_numCols = 5;

  m_archCols.append( new ArchColumns( 5, QRegExp( "[0-2][0-9][0-9][0-9]" ), 4 ) ); // Year
  m_archCols.append( new ArchColumns( 6, QRegExp( "[01][0-9]" ), 2 ) ); // Month
  m_archCols.append( new ArchColumns( 7, QRegExp( "[0-3][0-9]" ), 2 ) ); // Day
  m_archCols.append( new ArchColumns( 8, QRegExp( "[0-9:]+" ), 8 ) ); // Time
  m_archCols.append( new ArchColumns( 4, QRegExp( "[^\\s]+" ) ) ); // Attributes
  m_archCols.append( new ArchColumns( 1, QRegExp( "[0-9]+" ) ) ); // Size
  m_archCols.append( new ArchColumns( 2, QRegExp( "[0-9]+" ) ) ); // Compressed Size
  m_archCols.append( new ArchColumns( 0, QRegExp( "[^\\n]+" ), 4096 ) ); // Name
}

SevenZipArch::~SevenZipArch()
{
}

void SevenZipArch::setHeaders()
{
  QStringList columns;
  columns.append( FILENAME_STRING );
  columns.append( SIZE_STRING );
  columns.append( PACKED_STRING );
  columns.append( TIMESTAMP_STRING );
  columns.append( PERMISSION_STRING );
  
  int *rightAlignedColumns = new int[ 2 ];
  rightAlignedColumns[ 0 ] = 1;
  rightAlignedColumns[ 1 ] = 2;

  m_gui->setHeaders( &columns, rightAlignedColumns, 2 );
  delete [] rightAlignedColumns;
}

void SevenZipArch::open()
{
  setHeaders();

  m_buffer = "";
  m_header_removed = false;
  m_finished = false;

  KProcess *kp = new KProcess;
  *kp << m_archiver_program << "l" << m_filename;
  
  connect( kp, SIGNAL( receivedStdout(KProcess*, char*, int) ),
           SLOT( slotReceivedTOC(KProcess*, char*, int) ) );
  connect( kp, SIGNAL( receivedStderr(KProcess*, char*, int) ),
           SLOT( slotReceivedOutput(KProcess*, char*, int) ) );
  connect( kp, SIGNAL( processExited(KProcess*) ),
           SLOT( slotOpenExited(KProcess*) ) );

  if ( !kp->start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
  {
    KMessageBox::error( 0, i18n( "Could not start a subprocess." ) );
    emit sigOpen( this, false, QString::null, 0 );
  }
}

void SevenZipArch::create()
{
  emit sigCreate( this, true, m_filename,
                  Arch::Extract | Arch::Delete | Arch::Add | Arch::View );
}

void SevenZipArch::addFile( const QStringList & urls )
{
  KProcess *kp = new KProcess;
  
  kp->clearArguments();
  *kp << m_archiver_program << "a" ;

  KURL url( urls.first() );
  QDir::setCurrent( url.directory() );
  
  *kp << m_filename;
  
  QStringList::ConstIterator iter;
  for ( iter = urls.begin(); iter != urls.end(); ++iter )
  {
    KURL url( *iter );
    *kp << url.fileName();
  }
  
  connect( kp, SIGNAL( receivedStdout(KProcess*, char*, int) ),
           SLOT( slotReceivedOutput(KProcess*, char*, int) ) );
  connect( kp, SIGNAL( receivedStderr(KProcess*, char*, int) ),
           SLOT( slotReceivedOutput(KProcess*, char*, int) ) );
  connect( kp, SIGNAL( processExited(KProcess*) ),
           SLOT( slotAddExited(KProcess*) ) );

  if ( !kp->start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
  {
    KMessageBox::error( 0, i18n( "Could not start a subprocess." ) );
    emit sigAdd( false );
  }
}

void SevenZipArch::addDir( const QString & dirName )
{
  if ( !dirName.isEmpty() )
  {
    QStringList list;
    list.append( dirName );
    addFile( list );
  }
}

void SevenZipArch::remove( QStringList *list )
{
  if ( !list )
    return;

  KProcess *kp = new KProcess;
  kp->clearArguments();

  *kp << m_archiver_program << "d" << m_filename;
  
  QStringList::Iterator it;
  for ( it = list->begin(); it != list->end(); ++it )
  {
    *kp << *it;
  }

  connect( kp, SIGNAL( receivedStdout(KProcess*, char*, int) ),
           SLOT( slotReceivedOutput(KProcess*, char*, int) ) );
  connect( kp, SIGNAL( receivedStderr(KProcess*, char*, int) ),
           SLOT( slotReceivedOutput(KProcess*, char*, int) ) );
  connect( kp, SIGNAL( processExited(KProcess*) ),
           SLOT( slotDeleteExited(KProcess*) ) );

  if ( !kp->start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
  {
    KMessageBox::error( 0, i18n( "Could not start a subprocess." ) );
    emit sigDelete( false );
  }
}

void SevenZipArch::unarchFile( QStringList *fileList, const QString & destDir,
                               bool /*viewFriendly*/ )
{
  if ( destDir.isEmpty() || destDir.isNull() )
  {
    kdError( 1601 ) << "There was no extract directory given." << endl;
    return;
  }

  KProcess *kp = new KProcess;
  kp->clearArguments();

  // extract (and maybe overwrite)
  *kp << m_unarchiver_program << "x";

  if ( ArkSettings::extractOverwrite() )
  {
    //*kp << "-ao";
  }

  *kp << m_filename;

  // if the file list is empty, no filenames go on the command line,
  // and we then extract everything in the archive.
  if ( fileList )
  {
    QStringList::Iterator it;
    for ( it = fileList->begin(); it != fileList->end(); ++it )
    {
      *kp << (*it);
    }
  }

  *kp << "-o" + destDir ;

  connect( kp, SIGNAL( receivedStdout(KProcess*, char*, int) ),
           SLOT( slotReceivedOutput(KProcess*, char*, int) ) );
  connect( kp, SIGNAL( receivedStderr(KProcess*, char*, int) ),
           SLOT( slotReceivedOutput(KProcess*, char*, int) ) );
  connect( kp, SIGNAL( processExited(KProcess*) ),
           SLOT( slotExtractExited(KProcess*) ) );

  if ( !kp->start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
  {
    KMessageBox::error( 0, i18n( "Could not start a subprocess." ) );
    emit sigExtract( false );
  }
}

#include "sevenzip.moc"
