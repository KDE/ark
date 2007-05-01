/*

 ark -- archiver for the KDE project

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

// ark includes
#include "rar.h"
#include "arkwidget.h"
#include "archive.h"
#include "settings.h"
#include "arkutils.h"
#include "filelistview.h"

// Std includes
#include <sys/errno.h>
#include <unistd.h>
#include <iostream>
#include <string>

// QT includes
#include <QFile>
#include <QDir>
//Added by qt3to4:
#include <QByteArray>

// KDE includes
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <k3process.h>
#include <kstandarddirs.h>

RarArch::RarArch( ArkWidget *_gui, const QString & _fileName )
  : Arch( _gui, _fileName )
{
  // Check if rar is available
  bool have_rar = !KGlobal::dirs()->findExe( "rar" ).isNull();

  if ( have_rar )
  {
    // If it is, then use it as archiver and unarchiver
    m_archiver_program = m_unarchiver_program = "rar";
  }
  else
  {
    // If rar is not available, try to use unrar to open the archive read-only
    m_archiver_program = m_unarchiver_program = "unrar";
    setReadOnly( true );
  }

  verifyUtilityIsAvailable( m_archiver_program );

  m_headerString = "-------------------------------------------------------------------------------";

  m_isFirstLine = true;
}

bool RarArch::processLine( const QByteArray &line )
{
  if ( m_isFirstLine )
  {
    m_entryFilename = line;
    m_entryFilename.remove( 0, 1 );
    m_isFirstLine = false;
    return true;
  }

  QStringList list;

  QStringList l2 = QString(line).split( ' ' );

  list << m_entryFilename; // filename
  list << l2[ 0 ]; // size
  list << l2[ 1 ]; // packed
  list << l2[ 2 ]; // ratio

  QStringList date =  l2[3].split( '-' );
  list << ArkUtils::fixYear( date[ 2 ].toAscii() ) + '-' + date[ 1 ] + '-' + date [ 0 ] + ' ' + l2[4]; // date
  list << l2[ 5 ]; // attributes
  list << l2[ 6 ]; // crc
  list << l2[ 7 ]; // method
  list << l2[ 8 ]; // Version

  m_gui->fileList()->addItem( list ); // send to GUI

  m_isFirstLine = true;
  return true;
}

void RarArch::open()
{
  setHeaders();

  m_buffer = "";
  m_header_removed = false;
  m_finished = false;

  K3Process *kp = m_currentProcess = new K3Process;
  *kp << m_archiver_program << "v" << "-c-" << m_filename;

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

void RarArch::setHeaders()
{
  ColumnList list;
  list.append( FILENAME_COLUMN );
  list.append( SIZE_COLUMN );
  list.append( PACKED_COLUMN );
  list.append( RATIO_COLUMN );
  list.append( TIMESTAMP_COLUMN );
  list.append( PERMISSION_COLUMN );
  list.append( CRC_COLUMN );
  list.append( METHOD_COLUMN );
  list.append( VERSION_COLUMN );

  emit headers( list );
}

void RarArch::create()
{
  emit sigCreate( this, true, m_filename,
                  Arch::Extract | Arch::Delete | Arch::Add | Arch::View );
}

void RarArch::addDir( const QString & _dirName )
{
  if ( !_dirName.isEmpty() )
  {
    QStringList list;
    list.append( _dirName );
    addFile( list );
  }
}

void RarArch::addFile( const QStringList & urls )
{
  K3Process *kp = m_currentProcess = new K3Process;

  kp->clearArguments();
  *kp << m_archiver_program;

  if ( ArkSettings::replaceOnlyWithNewer() )
    *kp << "u";
  else
    *kp << "a";

  if ( ArkSettings::rarStoreSymlinks() )
    *kp << "-ol";
  if ( ArkSettings::rarRecurseSubdirs() )
    *kp << "-r";

  *kp << m_filename;

  KUrl dir( urls.first() );
  QDir::setCurrent( dir.directory() );

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

void RarArch::unarchFileInternal()
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

  if ( !m_password.isEmpty() )
    *kp << "-p=" + m_password;

  if ( !ArkSettings::extractOverwrite() )
  {
    *kp << "-o+";
  }
  else
  {
    *kp << "-o-";
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

  *kp << m_destDir ;

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

bool RarArch::passwordRequired()
{
    return m_lastShellOutput.lastIndexOf("password incorrect ?)")+1;
}

void RarArch::remove( QStringList *list )
{
  if ( !list )
    return;

  K3Process *kp = m_currentProcess = new K3Process;
  kp->clearArguments();

  *kp << m_archiver_program << "d" << m_filename;

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

#include "rar.moc"
