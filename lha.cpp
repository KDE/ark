/*

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
 2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)

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

#include <config.h>

// C includes
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <string.h>

// QT includes
#include <qfile.h>
#include <qdir.h>

// KDE includes
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprocess.h>

// ark includes
#include "arkwidget.h"
#include "settings.h"
#include "arch.h"
#include "lha.h"
#include "arkutils.h"
#include "filelistview.h"

LhaArch::LhaArch( ArkWidget *_gui, const QString & _fileName )
  : Arch( _gui, _fileName )
{
  m_archiver_program = "lha";
  verifyCompressUtilityIsAvailable( m_archiver_program );

  m_headerString = "----";
}

bool LhaArch::processLine( const QCString &line )
{
  const char *_line = ( const char * ) line;
  char columns[13][80];
  char filename[4096];

  if ( line.contains( "[generic]" ) )
  {
    sscanf( _line,
            " %79[]\\[generic] %79[0-9] %79[0-9] %79[0-9.%*] %10[-a-z0-9 ] %3[A-Za-z]%1[ ]%2[0-9 ]%1[ ]%5[ 0-9:]%1[ ]%4095[^\n]",
            columns[0], columns[2], columns[3], columns[4], columns[5],
            columns[6], columns[10], columns[7], columns[11], columns[8],
            columns[9], filename );
    strcpy( columns[1], " " );
  }
  else if ( line.contains( "[MS-DOS]" ) )
  {
    sscanf( _line,
            " %79[]\\[MS-DOS] %79[0-9] %79[0-9] %79[0-9.%*] %10[-a-z0-9 ] %3[A-Za-z]%1[ ]%2[0-9 ]%1[ ]%5[ 0-9:]%1[ ]%4095[^\n]",
            columns[0], columns[2], columns[3], columns[4], columns[5],
            columns[6], columns[10], columns[7], columns[11], columns[8],
            columns[9], filename );
    strcpy( columns[1], " " );
  }
  else
  {
    sscanf( _line,
            " %79[-drlwxst] %79[0-9/] %79[0-9] %79[0-9] %79[0-9.%*] %10[-a-z0-9 ] %3[A-Za-z]%1[ ]%2[0-9 ]%1[ ]%5[ 0-9:]%1[ ]%4095[^\n]",
            columns[0], columns[1], columns[2], columns[3],
            columns[4], columns[5], columns[6], columns[10], columns[7],
            columns[11], columns[8], columns[9], filename );
  }

  // make the time stamp sortable
  QString massagedTimeStamp = ArkUtils::getTimeStamp( columns[6], columns[7],
                                                      columns[8] );
  strlcpy( columns[6], massagedTimeStamp.ascii(), sizeof( columns[6] ) );

  // see if there was a link in filename
  QString file = filename;
  QString name, link;
  bool bLink = false;
  int pos = file.find( " -> " );
  if ( pos != -1 )
  {
    bLink = true;
    name = file.left(pos);
    link = file.right(file.length()-pos-4);
  }
  else
  {
    name = file;
  }

  QStringList list;
  list.append( name );

  for ( int i = 0; i < 7; i++ )
  {
    list.append( QString::fromLocal8Bit( columns[i] ) );
  }

  if ( bLink )
    list.append( link );
  else
    list.append( "" );

  m_gui->fileList()->addItem( list ); // send to GUI

  return true;
}

void LhaArch::open()
{
  setHeaders();

  m_buffer = "";
  m_header_removed = false;
  m_finished = false;


  KProcess *kp = m_currentProcess = new KProcess;
  *kp << m_archiver_program << "v" << m_filename;
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

void LhaArch::setHeaders()
{
  ColumnList list;
  list.append( FILENAME_COLUMN);
  list.append( PERMISSION_COLUMN);
  list.append( OWNER_GROUP_COLUMN);
  list.append( PACKED_COLUMN);
  list.append( SIZE_COLUMN);
  list.append( RATIO_COLUMN);
  list.append( CRC_COLUMN);
  list.append( TIMESTAMP_COLUMN);
  list.append( LINK_COLUMN);
  emit headers( list );
}


void LhaArch::create()
{
  emit sigCreate( this, true, m_filename,
                  Arch::Extract | Arch::Delete | Arch::Add | Arch::View );
}

void LhaArch::addDir( const QString & dirName )
{
  if ( !dirName.isEmpty() )
  {
    QStringList list;
    list.append( dirName );
    addFile( list );
  }
}

void LhaArch::addFile( const QStringList &urls )
{
  KProcess *kp = m_currentProcess = new KProcess;
  kp->clearArguments();
  *kp << m_archiver_program;

  QString strOptions;
  if ( ArkSettings::replaceOnlyWithNewer() )
    strOptions = "u";
  else
    strOptions = "a";

  *kp << strOptions << m_filename;

  KURL url( urls.first() );
  QDir::setCurrent( url.directory() );

  QStringList::ConstIterator iter;
  for ( iter = urls.begin(); iter != urls.end(); ++iter )
  {
    KURL fileURL( *iter );
    *kp << fileURL.fileName();
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

void LhaArch::unarchFileInternal()
{
  // if _fileList is empty, we extract all.
  // if _destDir is empty, abort with error.

  if ( m_destDir.isEmpty() || m_destDir.isNull() )
  {
    kdError( 1601 ) << "There was no extract directory given." << endl;
    return;
  }

  KProcess *kp = m_currentProcess = new KProcess;
  kp->clearArguments();

  *kp << m_archiver_program << "xfw=" + m_destDir << m_filename;

  // if the list is empty, no filenames go on the command line,
  // and we then extract everything in the archive.
  if ( m_fileList )
  {
    QStringList::Iterator it;
    for ( it = m_fileList->begin(); it != m_fileList->end(); ++it )
    {
      *kp << ( *it );
    }
  }

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

void LhaArch::remove( QStringList *list )
{
  if ( !list )
    return;

  KProcess *kp = m_currentProcess = new KProcess;
  kp->clearArguments();

  *kp << m_archiver_program << "df" << m_filename;

  QStringList::Iterator it;
  for ( it = list->begin(); it != list->end(); ++it )
  {
    *kp << ( *it );
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

#include "lha.moc"
