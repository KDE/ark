/* CompressedFile implementation 
*/
/*
    ark: A program for modifying archives via a GUI.

    Copyright (C)

    2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

// Qt includes
#include <qdir.h>
#include <qstringlist.h>

// KDE includes
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

// ark includes
#include "compressedfile.h"

// the generic viewer to which to send header and column info.
#include "viewer.h"

CompressedFile::CompressedFile( ArkSettings *_settings, Viewer *_gui,
		  const QString & _fileName )
  : Arch(_settings, _gui, _fileName )
{
  kDebugInfo(1601, "CompressedFile constructor");
  m_tmpdir.sprintf("/tmp/ark.%d", getpid());
}

void CompressedFile::setHeaders()
{
  kDebugInfo( 1601, "+CompressedFile::setHeaders");
  QStringList list;

  list.append(i18n(" Filename "));
  list.append(i18n(" Permissions ") );
  list.append(i18n(" Owner ") );
  list.append(i18n(" Group ") );
  list.append(i18n(" Size ") );

  // which columns to align right
  int *alignRightCols = new int[1];
  alignRightCols[0] = 3;
  
  m_gui->setHeaders(&list, alignRightCols, 1);
  delete [] alignRightCols;

  kDebugInfo( 1601, "-CompressedFile::setHeaders");
}

QString CompressedFile::getUnCompressor()
{
  // see extension of m_filename to determine.
  QString ret;

  if (m_filename.right(3) == ".gz")
    ret = "gunzip";
  else if (m_filename.right(3) == ".bz")
    ret = "bunzip";
  else if (m_filename.right(4) == ".bz2")
    ret = "bunzip2";
  else if (m_filename.right(4) == ".lzo")
    ret = "lzop";
  else if (m_filename.right(2) == ".Z")
    ret ="uncompress";
  return ret;
}

QString CompressedFile::getCompressor()
{
  QString ret;
  if (m_filename.right(3) == ".gz")
    ret = "gzip";
  else if (m_filename.right(3) == ".bz")
    ret = "bzip";
  else if (m_filename.right(4) == ".bz2")
    ret = "bzip2";
  else if (m_filename.right(4) == ".lzo")
    ret = "lzop";
  else if (m_filename.right(2) == ".Z")
    ret = "compress";
  return ret;
}

void CompressedFile::open()
{
  kDebugInfo( 1601, "+CompressedFile::open");
  setHeaders();

  // We copy the file into the temporary directory, uncompress it,
  // and when the uncompression is done, obtain an ls -l of it
  // (that code is in the slot slotOpenDone)

  QString command;
  command = "cp '" + m_filename + "' " + m_tmpdir;
  system((const char *)command);

  m_tmpfile = m_filename.right(m_filename.length()
			       - m_filename.findRev("/")-1);
  m_tmpfile = m_tmpdir + "/" + m_tmpfile;

  kDebugInfo(1601, "Temp file name is %s", (const char *)m_tmpfile);

  KProcess *kp = new KProcess;
  QString uncompressor = getUnCompressor();
  kp->clearArguments();
  *kp << uncompressor.local8Bit() << "-f" ;
  if (uncompressor == "lzop")
    *kp << "-d";
  *kp << m_tmpfile.local8Bit();

  kDebugInfo(1601, "Command is %s %s", (const char *)uncompressor,
	     (const char *)m_tmpfile.local8Bit());

  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
  connect( kp, SIGNAL(processExited(KProcess*)), this,
	   SLOT(slotUncompressDone(KProcess*)));

  if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Couldn't start a subprocess.") );
      emit sigOpen(this, false, QString::null, 0 );
    }

  kDebugInfo( 1601, "-CompressedFile::open");
}

void CompressedFile::slotUncompressDone(KProcess *_kp)
{
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
    KMessageBox::sorry( (QWidget *)0, i18n("Open failed"), i18n("Error") );

  if (bSuccess)
    {
      // now do the ls -l. Just a simple system() call
      m_tmpfile = m_tmpfile.left(m_tmpfile.findRev("."));
      kDebugInfo(1601, "Temp file is %s", (const char *)m_tmpfile);
      chdir((const char *)m_tmpdir);
      QString command = "ls -l " +
	m_tmpfile.right(m_tmpfile.length() - 1 - m_tmpfile.findRev("/"));
      
      char line[4096];
      char columns[7][80];
      char filename[4096];
      
      FILE *readHandle = popen((const char *)command, "r");
      fscanf(readHandle, "%[-A-Za-z:0-9_. ]", line);
      sscanf(line, "%[-drwxst] %[0-9] %[0-9.a-zA-Z_] %[0-9.a-zA-Z_] %[0-9] %12[A-Za-z0-9: ]%1[ ]%[^\n]", columns[0], columns[5],
	     columns[1], columns[2], columns[3],
	     columns[4], columns[6], filename);
      
      kDebugInfo(1601, "%s\t%s\t%s\t%s\t%s\t%s\n",
	      columns[0], columns[1], columns[2], columns[3],
	      columns[4], filename);


      QStringList list;
      kDebugInfo(1601, "Filename is %s", (const char *)filename);
      list.append(QString::fromLocal8Bit(filename));
      for (int i=0; i<4; i++)
	{
	  list.append(QString::fromLocal8Bit(columns[i]));
	}
      m_gui->add(&list); // send to GUI
    }
  delete _kp;
  _kp = 0;
  emit sigOpen( this, bSuccess, m_filename,
		Arch::Extract | Arch::Delete | Arch::Add | Arch::View );
}

void CompressedFile::create()
{
  emit sigCreate(this, true, m_filename,
		 Arch::Extract | Arch::Delete | Arch::Add 
		  | Arch::View);
}

void CompressedFile::addFile( QStringList *urls )
{
  // only used for adding ONE file to an EMPTY gzip file, i.e., one that
  // has just been created

  kDebugInfo( 1601, "+CompressedFile::addFile");

  ASSERT(m_gui->getNumFilesInArchive() == 0);
  ASSERT(urls->count() == 1);

  QString file = urls->first();
  if (file.left(5) == "file:")
    file = file.right(file.length() - 5);

  QString command;
  command = "cp '" + file + "' " + m_tmpdir;
  system((const char *)command);

  m_tmpfile = file.right(file.length()
			 - file.findRev("/")-1);
  m_tmpfile = m_tmpdir + "/" + m_tmpfile;

  kDebugInfo(1601, "Temp file name is %s", (const char *)m_tmpfile);

  kDebugInfo(1601, "File is %s", (const char *)file);

  KProcess *kp = new KProcess;
  kp->clearArguments();
  QString compressor = getCompressor();

  *kp << compressor << "-c" << file.local8Bit();

  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotAddInProgress(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
  connect( kp, SIGNAL(processExited(KProcess*)), this,
	   SLOT(slotAddDone(KProcess*)));

  fd = fopen( m_filename.local8Bit(), "w" );

  if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Couldn't start a subprocess.") );
    }

  kDebugInfo( 1601, "-CompressedFile::addFile");
}

void CompressedFile::slotAddInProgress(KProcess*, char* _buffer, int _bufflen)
{
  // we're trying to capture the output of a command like this
  //    gzip -c myfile
  // and feed the output to the compressed file
  int size;
  size = fwrite(_buffer, 1, _bufflen, fd);
  if (size != _bufflen)
    {
      KMessageBox::error(0, i18n("Trouble writing to the archive..."));
      exit(99);
    }
}

void CompressedFile::slotAddDone(KProcess *_kp)
{
  fclose(fd);
  slotAddExited(_kp);
}

void CompressedFile::unarchFile(QStringList *_fileList, const QString & _destDir)
{
  if (_destDir != m_tmpdir)
    {
      QString command;
      command.sprintf("cp %s %s", (const char *)m_tmpfile, 
		      (const char *)_destDir);
      system((const char *)command);
    }
  emit sigExtract(true);    
}

void CompressedFile::remove(QStringList *list)
{
  kDebugInfo( 1601, "+CompressedFile::remove");
  unlink(m_tmpfile);

  // delete the compressed file but then create it empty in case someone
  // does a reload and finds it no longer exists!
  unlink(m_filename);
  QString command = "touch '" + m_filename + "'";
  system((const char *)command);  

  m_tmpfile = "";
  emit sigDelete(true);
  kDebugInfo( 1601, "-CompressedFile::remove");
}



#include "compressedfile.moc"

