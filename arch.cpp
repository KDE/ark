/*

 $Id $

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org

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
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <kdebug.h>
#include <kmessagebox.h>
#include <klocale.h>
// ark includes
#include "arch.h"
#include "viewer.h"


Arch::Arch( ArkSettings *_settings, Viewer *_viewer,
	    const QString & _fileName )
  : m_filename(_fileName), m_settings(_settings), m_gui(_viewer),
    m_bReadOnly(false)
{
  kDebugInfo(1601, "+Arch::Arch");
  kDebugInfo(1601, "-Arch::Arch");
}

Arch::~Arch()
{
}

void Arch::slotCancel()
{
  //  m_kp->kill();
}

void Arch::slotStoreDataStdout(KProcess*, char* _data, int _length)
{
  char c = _data[_length];
  _data[_length] = '\0';

  m_settings->appendShellOutputData( _data );
  _data[_length] = c;
}

void Arch::slotStoreDataStderr(KProcess*, char* _data, int _length)
{
  char c = _data[_length];
  _data[_length] = '\0';
	
  m_shellErrorData.append( _data );
  _data[_length] = c;
}

void Arch::slotOpenExited(KProcess* _kp)
{
  kDebugInfo(1601, "normalExit = %d", _kp->normalExit() );
  kDebugInfo(1601, "exitStatus = %d", _kp->exitStatus() );

  bool bNormalExit = _kp->normalExit();

  int exitStatus = 100; // arbitrary bad exit status
  if (bNormalExit)
    exitStatus = _kp->exitStatus();

  if (1 == exitStatus)
    exitStatus = 0;    // because 1 means empty archive - not an error.
                       // Is this a safe assumption?

  if(!exitStatus) 
    emit sigOpen( this, true, m_filename,
		  Arch::Extract | Arch::Delete | Arch::Add | Arch::View );
  else
    emit sigOpen( this, false, QString::null, 0 );

  delete _kp;
  _kp = NULL;

}

void Arch::slotDeleteExited(KProcess *_kp)
{
  kDebugInfo(1601, "+Arch::slotDeleteExited");

  bool bSuccess = false;

  kDebugInfo(1601, "normalExit = %d", _kp->normalExit() );
  if( _kp->normalExit() )
    kDebugInfo(1601, "exitStatus = %d", _kp->exitStatus() );
  
  if( _kp->normalExit() && (_kp->exitStatus()==0) )
    {
      if(stderrIsError())
	{
	  KMessageBox::error( 0, i18n("You probably don't have sufficient permissions.\n"
				      "Please check the file owner and the integrity\n"
				      "of the archive.") );
	}
      else
	bSuccess = true;
    }
  else
    KMessageBox::sorry( (QWidget *)0, i18n("Error"), i18n("Deletion failed") );
  
  emit sigDelete(bSuccess);
  delete _kp;
  _kp = NULL;

  kDebugInfo(1601, "-Arch::slotDeleteExited");
}

void Arch::slotExtractExited(KProcess *_kp)
{
  kDebugInfo(1601, "+Arch::slotExtractExited");

  bool bSuccess = false;

  kDebugInfo(1601, "normalExit = %d", _kp->normalExit() );
  if( _kp->normalExit() )
    kDebugInfo(1601, "exitStatus = %d", _kp->exitStatus() );

  if( _kp->normalExit() && (_kp->exitStatus()==0) )
    {
      if(stderrIsError())
	{
	  KMessageBox::error( (QWidget *) 0, i18n("Error"), i18n("Something bad happened when trying to extract...") );
	}
      else
	bSuccess = true;
    }
  else
    KMessageBox::sorry((QWidget *)0, i18n("Error"), i18n("Extraction failed"));

  emit sigExtract(bSuccess);
  delete _kp;
  _kp = NULL;

  kDebugInfo(1601, "-Arch::slotExtractExited");
}

void Arch::slotAddExited(KProcess *_kp)
{
  kDebugInfo(1601, "+Arch::slotAddExited");

  bool bSuccess = false;

  kDebugInfo(1601, "normalExit = %d", _kp->normalExit() );
  if( _kp->normalExit() )
    kDebugInfo(1601, "exitStatus = %d", _kp->exitStatus() );

  if( _kp->normalExit() && (_kp->exitStatus()==0) )
    {
      if(stderrIsError())
	{
	  KMessageBox::error( 0, i18n("You probably don't have sufficient permissions\n"
				      "Please check the file owner and the integrity\n"
				      "of the archive.") );
	}
      else
	bSuccess = true;
    }
  else
    KMessageBox::sorry((QWidget *)0, i18n("Error"), i18n("Add failed"));
  
  emit sigAdd(bSuccess);
  delete _kp;
  _kp = NULL;

  kDebugInfo(1601, "-Arch::slotAddExited");
}


bool Arch::stderrIsError()
{
  return m_shellErrorData.find(QString("eror")) != -1;
}

#include "arch.moc"



