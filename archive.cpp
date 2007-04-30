/*

 ark -- archiver for the KDE project

 Copyright (C)

 2002: Helio Chissini de Castro <helio@conectiva.com.br>
 2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
 1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1997-1999: Rob Palmbos palm9744@kettering.edu

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
#include "archive.h"
#include "arkwidget.h"
#include "arkutils.h"
#include "filelistview.h"

// C includes
#include <stdlib.h>
#include <time.h>

// QT includes
#include <QApplication>
#include <QFile>
#include <QByteArray>

// KDE includes
#include <kdebug.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <klocale.h>
#include <kpassworddialog.h>
#include <k3process.h>
#include <kstandarddirs.h>

// the archive types
#include "tar.h"
#include "zip.h"
#include "lha.h"
#include "compressedfile.h"
#include "zoo.h"
#include "rar.h"
#include "ar.h"
#include "sevenzip.h"
#include "ace.h"

Arch::ArchColumns::ArchColumns( int col, const QRegExp &reg, int length, bool opt )
  : colRef( col ), pattern( reg ), maxLength( length ), optional( opt )
{
}

Arch::Arch( ArkWidget *gui, const QString &filename )
  : m_filename( filename ), m_buffer( "" ), m_gui( gui ),
    m_bReadOnly( false ), m_bNotifyWhenDeleteFails( true ),
    m_header_removed( false ), m_finished( false ),
    m_numCols( 0 ), m_dateCol( -1 ), m_fixYear( -1 ), m_fixMonth( -1 ),
    m_fixDay( -1 ), m_fixTime( -1 ), m_repairYear( -1 ), m_repairMonth( -1 ),
    m_repairTime( -1 ), m_currentProcess( 0 )
{
}

Arch::~Arch()
{
    if ( m_currentProcess )
        m_currentProcess->kill();
}

void Arch::verifyUtilityIsAvailable( const QString &utility1,
                                     const QString &utility2)
{
  // see if the utility is in the PATH of the user. If there is a
  // second utility specified, it must also be present.
  QString cmd1 = KGlobal::dirs()->findExe( utility1 );

  if( utility2.isNull() )
  {
    m_bUtilityIsAvailable = !cmd1.isEmpty();
  }
  else
  {
    QString cmd2 = KGlobal::dirs()->findExe( utility2 );
    m_bUtilityIsAvailable = ( !cmd1.isEmpty() && !cmd2.isEmpty() );
  }
}

void Arch::slotOpenExited( K3Process* _kp )
{
  int exitStatus = 100; // arbitrary bad exit status

  if ( _kp->normalExit() )
    exitStatus = _kp->exitStatus();

  if ( exitStatus == 1 )
  {
    exitStatus = 0;    // because 1 means empty archive - not an error.
                       // Is this a safe assumption?
  }

  if ( !exitStatus )
    emit sigOpen( this, true, m_filename,
                  Arch::Extract | Arch::Delete | Arch::Add | Arch::View );
  else
    emit sigOpen( this, false, QString(), 0 );

  delete _kp;
  _kp = m_currentProcess = 0;
}

void Arch::slotDeleteExited( K3Process *_kp )
{
  bool success = ( _kp->normalExit() && ( _kp->exitStatus() == 0 ) );

  if ( !success )
  {
    QApplication::restoreOverrideCursor();

    QString msg = i18n( "The deletion operation failed." );

    if ( !getLastShellOutput().isNull() )
    {
      msg += i18n( "\nUse \"Details\" to view the last shell output." );
      KMessageBox::detailedError( m_gui, msg, getLastShellOutput() );
    }
    else
    {
      KMessageBox::error( m_gui, msg );
    }
  }

  emit sigDelete( success );
  delete _kp;
  _kp = m_currentProcess = 0;
}

void Arch::slotExtractExited( K3Process *_kp )
{
  bool success = ( _kp->normalExit() && ( _kp->exitStatus() == 0 ) );

  if( !success )
  {
    if ( passwordRequired() )
    {
        QString msg;
        if ( !m_password.isEmpty() )
            msg = i18n("The password was incorrect. ");
        KPasswordDialog dlg(m_gui);
        dlg.setPrompt( msg+i18n("You must enter a password to extract the file:")  );
        if( dlg.exec() == KPasswordDialog::Accepted )
        {
            m_password=dlg.password().toLocal8Bit();
            delete _kp;
            _kp = m_currentProcess = 0;
            clearShellOutput();
            unarchFileInternal(); // try to extract the file again with a password
            return;
        }
        emit sigExtract( true );
        delete _kp;
        _kp = m_currentProcess = 0;
        return;
    }
    else if ( m_password.isEmpty() || _kp->exitStatus() > 1 )
    {
        QApplication::restoreOverrideCursor();

        QString msg = i18n( "The extraction operation failed." );

        if ( !getLastShellOutput().isNull() )
        {
            msg += i18n( "\nUse \"Details\" to view the last shell output." );
            KMessageBox::detailedError( m_gui, msg, getLastShellOutput() );
        }
        else
        {
            KMessageBox::error( m_gui, msg );
        }
    }
  }
  m_password = "";
  emit sigExtract( success );
  delete _kp;
  _kp = m_currentProcess = 0;
}

void Arch::unarchFile( QStringList *fileList, const QString & destDir,
                         bool viewFriendly )
{
    m_fileList = fileList;
    m_destDir = destDir;
    m_viewFriendly = viewFriendly;
    unarchFileInternal();
}

void Arch::slotAddExited( K3Process *_kp )
{
  bool success = ( _kp->normalExit() && ( _kp->exitStatus() == 0 ) );

  if( !success )
  {
    QApplication::restoreOverrideCursor();

    QString msg = i18n( "The addition operation failed." );

    if ( !getLastShellOutput().isNull() )
    {
      msg += i18n( "\nUse \"Details\" to view the last shell output." );
      KMessageBox::detailedError( m_gui, msg, getLastShellOutput() );
    }
    else
    {
      KMessageBox::error( m_gui, msg );
    }
  }

  emit sigAdd( success );
  delete _kp;
  _kp = m_currentProcess = 0;
}

void Arch::slotReceivedOutput( K3Process*, char* data, int length )
{
  char c = data[ length ];
  data[ length ] = '\0';

  appendShellOutputData( data );
  data[ length ] = c;
}


void Arch::slotReceivedTOC( K3Process*, char* data, int length )
{
  char c = data[ length ];
  data[ length ] = '\0';

  appendShellOutputData( data );

  int lfChar, startChar = 0;

  while ( !m_finished )
  {
    for ( lfChar = startChar; data[ lfChar ] != '\n' && lfChar < length;
         lfChar++ );

    if ( data[ lfChar ] != '\n')
      break; // We are done all the complete lines

    data[ lfChar ] = '\0';
    m_buffer.append( data + startChar );
    data[ lfChar ] = '\n';
    startChar = lfChar + 1;

    if ( m_headerString.isEmpty() )
    {
      processLine( m_buffer );
    }
    else if ( m_buffer.indexOf( m_headerString ) == -1 )
    {
      if ( m_header_removed && !m_finished )
      {
        if ( !processLine( m_buffer ) )
        {
          // Have faith - maybe it wasn't a header?
          m_header_removed = false;
          m_error = true;
        }
      }
    }
    else if ( !m_header_removed )
    {
      m_header_removed = true;
    }
    else
    {
      m_finished = true;
    }

    m_buffer = "";
  }

  if ( !m_finished )
    m_buffer.append( data + startChar);	// Append what's left of the buffer

  data[ length ] = c;
}

bool Arch::processLine( const QByteArray &line )
{
  QString columns[ 11 ];
  unsigned int pos = 0;
  int strpos, len;

  // Go through our columns, try to pick out data, return silently on failure
  foreach( const ArchColumns &column, m_archCols )
  {
    strpos = column.pattern.indexIn( line, pos );
    len    = column.pattern.matchedLength();

    if ( ( strpos == -1 ) || ( len > column.maxLength ) )
    {
      if ( column.optional )
        continue; // More?
      else
      {
        kDebug(1601) << "processLine failed to match critical column" << endl;
        return false;
      }
    }

    pos = strpos + len;

    columns[ column.colRef ] = line.mid( strpos, len );
  }


  if ( m_dateCol >= 0 )
  {
    QString year = ( m_repairYear >= 0 ) ?
                   ArkUtils::fixYear( qPrintable(columns[ m_repairYear ]))
                   : columns[ m_fixYear ];
    QString month = ( m_repairMonth >= 0 ) ?
                   QString( "%1" )
                   .arg( ArkUtils::getMonth( qPrintable( columns[ m_repairMonth ] ) ) )
                   : columns[ m_fixMonth ];
    QString timestamp = QString::fromLatin1( "%1-%2-%3 %4" )
                        .arg( year )
                        .arg( month )
                        .arg( columns[ m_fixDay ] )
                        .arg( columns[ m_fixTime ] );

    columns[ m_dateCol ] = timestamp;
  }

  QStringList list;

  for ( int i = 0; i < m_numCols; ++i )
  {
    list.append( columns[ i ] );
  }

  m_gui->fileList()->addItem( list ); // send the entry to the GUI

  return true;
}


Arch *Arch::archFactory( ArchType aType,
                         ArkWidget *parent, const QString &filename,
                         const QString &openAsMimeType )
{
  switch( aType )
  {
    case TAR_FORMAT:
      return new TarArch( parent, filename, openAsMimeType );

    case ZIP_FORMAT:
      return new ZipArch( parent, filename );

    case LHA_FORMAT:
      return new LhaArch( parent, filename );

    case COMPRESSED_FORMAT:
      return new CompressedFile( parent, filename, openAsMimeType );

    case ZOO_FORMAT:
      return new ZooArch( parent, filename );

    case RAR_FORMAT:
      return new RarArch( parent, filename );

    case AA_FORMAT:
      return new ArArch( parent, filename );

    case SEVENZIP_FORMAT:
      return new SevenZipArch( parent, filename );

    case ACE_FORMAT:
      return new AceArch( parent, filename );

    case UNKNOWN_FORMAT:
    default:
      return 0;
  }
}
#include "archive.moc"
