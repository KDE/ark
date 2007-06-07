/*

 ark -- archiver for the KDE project

 Copyright (C) 2000 Corel Corporation (author: Emily Ezust <emilye@corel.com>)
 Copyright (C) 2001 Corel Corporation (author: Michael Jarrett <michaelj@corel.com>)

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

// ark includes
#include "zoo.h"
#include "settings.h"
#include "arkwidget.h"
#include "archive.h"
#include "arkutils.h"
#include "filelistview.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <string.h>

// QT includes
#include <QFile>
#include <QDir>
#include <kdefakes.h>
//Added by qt3to4:
#include <QByteArray>

// KDE includes
#include <KDebug>
#include <KLocale>
#include <KMessageBox>
#include <K3Process>

static QString fixTime( const QString &_strTime );

ZooArch::ZooArch( ArkWidget *gui, const QString & fileName )
  : Arch( gui, fileName )
{
  m_archiver_program = "zoo";
  m_unarchiver_program = QString();
  verifyUtilityIsAvailable( m_archiver_program, m_unarchiver_program );

  m_headerString = "----";
}

bool ZooArch::processLine( const QByteArray &line )
{
  const char *_line = ( const char * )line;
  char columns[11][80];
  char filename[4096];

  // Note: I'm reversing the ratio and the length for better display

  sscanf( _line,
          " %79[0-9] %79[0-9%] %79[0-9] %79[0-9] %79[a-zA-Z] %79[0-9]%79[ ]%11[ 0-9:+-]%2[C ]%4095[^\n]",
          columns[1], columns[0], columns[2], columns[3], columns[7],
          columns[8], columns[9], columns[4], columns[10], filename );

  QString year = ArkUtils::fixYear( columns[8] );

  QString strDate;
  strDate.sprintf( "%s-%.2d-%.2d", year.toUtf8().data(),
                   ArkUtils::getMonth( columns[7] ), atoi( columns[3] ) );

  strlcpy( columns[3], qPrintable(strDate), sizeof( columns[3]) );
  kDebug( 1601 ) << "New timestamp is " << columns[3] << endl;

  strlcat( columns[3], " ", sizeof( columns[3] ) );
  strlcat( columns[3], qPrintable(fixTime( columns[4] )), sizeof( columns[3] ) );

  QStringList list;
  list.append( QFile::decodeName( filename ) );

  for ( int i=0; i<4; i++ )
  {
    list.append( QString::fromLocal8Bit( columns[i] ) );
  }

  //m_gui->fileList()->addItem( list ); // send to GUI
  emit newEntry( list );

  return true;
}

void ZooArch::open()
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

void ZooArch::setHeaders()
{
  ColumnList list;
  list.append( FILENAME_COLUMN );
  list.append( RATIO_COLUMN );
  list.append( SIZE_COLUMN );
  list.append( PACKED_COLUMN );
  list.append( TIMESTAMP_COLUMN );

  emit headers( list );
}


void ZooArch::create()
{
  emit sigCreate( this, true, m_filename,
                  Arch::Extract | Arch::Delete | Arch::Add | Arch::View);
}

void ZooArch::addDir( const QString & _dirName )
{
  if ( ! _dirName.isEmpty() )
  {
    QStringList list;
    list.append( _dirName );
    addFile( list );
  }
}

void ZooArch::addFile( const QStringList &urls )
{
  K3Process *kp = m_currentProcess = new K3Process;
  kp->clearArguments();
  *kp << m_archiver_program;

  if ( ArkSettings::replaceOnlyWithNewer() )
    *kp << "-update";
  else
    *kp << "-add";

  *kp << m_filename;

  KUrl url( urls.first() );
  QDir::setCurrent( url.directory() );

  QStringList::ConstIterator iter;

  for ( iter = urls.begin(); iter != urls.end(); ++iter )
  {
    KUrl fileURL( *iter );
    *kp << fileURL.fileName();
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

void ZooArch::unarchFileInternal()
{
  // if _fileList is empty, we extract all.
  // if _destDir is empty, abort with error.

  if ( m_destDir.isEmpty() || m_destDir.isNull() )
  {
    kError( 1601 ) << "There was no extract directory given." << endl;
    return;
  }

  // zoo has no option to specify the destination directory
  // so we have to change to it.

  bool ret = QDir::setCurrent( m_destDir );
  // We already checked the validity of the dir before coming here
  Q_ASSERT(ret);

  K3Process *kp = m_currentProcess = new K3Process;
  kp->clearArguments();

  *kp << m_archiver_program;

  if ( ArkSettings::extractOverwrite() )
  {
    *kp << "xOOS";
  }
  else
  {
    *kp << "x";
  }

  *kp << m_filename;

  // if the list is empty, no filenames go on the command line,
  // and we then extract everything in the archive.
  if (m_fileList)
  {
    QStringList::Iterator it;
    for ( it = m_fileList->begin(); it != m_fileList->end(); ++it )
    {
      *kp << (*it);
    }
  }

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

void ZooArch::remove( QStringList *list )
{
  if (!list)
    return;

  K3Process *kp = m_currentProcess = new K3Process;
  kp->clearArguments();

  *kp << m_archiver_program << "D" << m_filename;

  QStringList::Iterator it;
  for ( it = list->begin(); it != list->end(); ++it )
  {
    QString str = *it;
    *kp << str;
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

QString fixTime( const QString &_strTime )
{
  // it may have come from a different time zone... get rid of trailing
  // +3 or -3 etc.
  QString strTime = _strTime;

  if ( strTime.contains("+") || strTime.contains("-") )
  {
    QChar c = strTime.at( 8 );
    int offset = strTime.right( strTime.length() - 9 ).toInt();
    QString strHour = strTime.left( 2 );
    int nHour = strHour.toInt();
    if ( c == '+' || c == '-' )
    {
      if ( c == '+' )
        nHour = ( nHour + offset ) % 24;
      else if ( c == '-' )
      {
        nHour -= offset;
        if ( nHour < 0 )
          nHour += 24;
      }
      strTime = strTime.left( 8 );
      strTime.sprintf( "%2.2d%s", nHour, strTime.right( 6 ).toUtf8().data() );
    }
  }
  else
  {
    strTime = strTime.left( 8 );
  }
  return strTime;
}

#include "zoo.moc"
