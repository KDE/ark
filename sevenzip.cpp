/*

  ark -- archiver for the KDE project

  Copyright (C) 2004 Henrique Pinto <henrique.pinto@kdemail.net>
  Copyright (C) 2003 Helio Chissini de Castro <helio@conectiva.com>
  Copyright (C) 2000 Corel Corporation (author: Emily Ezust <emilye@corel.com>)

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

#include "sevenzip.h"
#include "arkwidget.h"
#include "settings.h"

#include <QDir>

#include <KGlobal>
#include <KLocale>
#include <KDebug>
#include <KUrl>
#include <KMessageBox>
#include <K3Process>
#include <KStandardDirs>

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

  m_archCols.append( ArchColumns( 5, QRegExp( "[0-2][0-9][0-9][0-9]" ), 4 ) ); // Year
  m_archCols.append( ArchColumns( 6, QRegExp( "[01][0-9]" ), 2 ) ); // Month
  m_archCols.append( ArchColumns( 7, QRegExp( "[0-3][0-9]" ), 2 ) ); // Day
  m_archCols.append( ArchColumns( 8, QRegExp( "[0-9:]+" ), 8 ) ); // Time
  m_archCols.append( ArchColumns( 4, QRegExp( "[^\\s]+" ) ) ); // Attributes
  m_archCols.append( ArchColumns( 1, QRegExp( "[0-9]+" ) ) ); // Size
  m_archCols.append( ArchColumns( 2, QRegExp( "[0-9]+" ) ) ); // Compressed Size
  m_archCols.append( ArchColumns( 0, QRegExp( "[^\\n]+" ), 4096 ) ); // Name
}

SevenZipArch::~SevenZipArch()
{
}

void SevenZipArch::setHeaders()
{
  ColumnList columns;
  columns.append( FILENAME_COLUMN );
  columns.append( SIZE_COLUMN );
  columns.append( PACKED_COLUMN );
  columns.append( TIMESTAMP_COLUMN );
  columns.append( PERMISSION_COLUMN );

  emit headers( columns );
}

void SevenZipArch::open()
{
  setHeaders();

  m_buffer = "";
  m_header_removed = false;
  m_finished = false;

  K3Process *kp = m_currentProcess = new K3Process;
  *kp << m_archiver_program << "l" << m_filename;

  connect( kp, SIGNAL( receivedStdout(K3Process*, char*, int) ),
           SLOT( slotReceivedTOC(K3Process*, char*, int) ) );
  connect( kp, SIGNAL( receivedStderr(K3Process*, char*, int) ),
           SLOT( slotReceivedOutput(K3Process*, char*, int) ) );
  connect( kp, SIGNAL( processExited(K3Process*) ),
           SLOT( slotOpenExited(K3Process*) ) );

  if ( !kp->start( K3Process::NotifyOnExit, K3Process::AllOutput ) )
  {
    KMessageBox::error( 0, i18n( "Could not start a subprocess." ) );
    emit sigOpen( this, false, QString(), 0 );
  }
}

void SevenZipArch::create()
{
  emit sigCreate( this, true, m_filename,
                  Arch::Extract | Arch::Delete | Arch::Add | Arch::View );
}

void SevenZipArch::addFile( const QStringList & urls )
{
  K3Process *kp = m_currentProcess = new K3Process;

  kp->clearArguments();
  *kp << m_archiver_program << "a" ;

  KUrl url( urls.first() );
  QDir::setCurrent( url.directory() );

  *kp << m_filename;

  QStringList::ConstIterator iter;
  for ( iter = urls.begin(); iter != urls.end(); ++iter )
  {
    KUrl url( *iter );
    *kp << url.fileName();
  }

  connect( kp, SIGNAL( receivedStdout(K3Process*, char*, int) ),
           SLOT( slotReceivedOutput(K3Process*, char*, int) ) );
  connect( kp, SIGNAL( receivedStderr(K3Process*, char*, int) ),
           SLOT( slotReceivedOutput(K3Process*, char*, int) ) );
  connect( kp, SIGNAL( processExited(K3Process*) ),
           SLOT( slotAddExited(K3Process*) ) );

  if ( !kp->start( K3Process::NotifyOnExit, K3Process::AllOutput ) )
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

  K3Process *kp = m_currentProcess = new K3Process;
  kp->clearArguments();

  *kp << m_archiver_program << "d" << m_filename;

  QStringList::Iterator it;
  for ( it = list->begin(); it != list->end(); ++it )
  {
    *kp << *it;
  }

  connect( kp, SIGNAL( receivedStdout(K3Process*, char*, int) ),
           SLOT( slotReceivedOutput(K3Process*, char*, int) ) );
  connect( kp, SIGNAL( receivedStderr(K3Process*, char*, int) ),
           SLOT( slotReceivedOutput(K3Process*, char*, int) ) );
  connect( kp, SIGNAL( processExited(K3Process*) ),
           SLOT( slotDeleteExited(K3Process*) ) );

  if ( !kp->start( K3Process::NotifyOnExit, K3Process::AllOutput ) )
  {
    KMessageBox::error( 0, i18n( "Could not start a subprocess." ) );
    emit sigDelete( false );
  }
}

void SevenZipArch::unarchFileInternal( )
{
  if ( m_destDir.isEmpty() || m_destDir.isNull() )
  {
    kError( 1601 ) << "There was no extract directory given." << endl;
    return;
  }

  K3Process *kp = m_currentProcess = new K3Process;
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
  if ( m_fileList )
  {
    QStringList::Iterator it;
    for ( it = m_fileList->begin(); it != m_fileList->end(); ++it )
    {
      *kp << (*it);
    }
  }

  *kp << "-o" + m_destDir ;

  connect( kp, SIGNAL( receivedStdout(K3Process*, char*, int) ),
           SLOT( slotReceivedOutput(K3Process*, char*, int) ) );
  connect( kp, SIGNAL( receivedStderr(K3Process*, char*, int) ),
           SLOT( slotReceivedOutput(K3Process*, char*, int) ) );
  connect( kp, SIGNAL( processExited(K3Process*) ),
           SLOT( slotExtractExited(K3Process*) ) );

  if ( !kp->start( K3Process::NotifyOnExit, K3Process::AllOutput ) )
  {
    KMessageBox::error( 0, i18n( "Could not start a subprocess." ) );
    emit sigExtract( false );
  }
}

#include "sevenzip.moc"
