/*
    ark: A program for modifying archives via a GUI.

    Copyright (C) 2000 Corel Corporation (author: Emily Ezust <emilye@corel.com>)
    Copyright (C) 2001 Corel Corporation (author: Michael Jarrett <michaelj@corel.com>)
    Copyright (C) 2003 Georg Robbers <Georg.Robbers@urz.uni-hd.de>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

// ark includes
#include "compressedfile.h"
#include "arkwidget.h"
#include "filelistview.h"

// C includes
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

// Qt includes
#include <QDir>

// KDE includes
#include <KDebug>
#include <kde_file.h>
#include <KLocale>
#include <KMessageBox>
#include <KStandardDirs>
#include <KTempDir>
#include <K3Process>
#include <KMimeType>
#include <kio/netaccess.h>
#include <kio/global.h>
#include <KFileItem>
#include <KApplication>

// encapsulates the idea of a compressed file

CompressedFile::CompressedFile( ArkWidget *_gui, const QString & _fileName, const QString & _openAsMimeType )
  : Arch( _gui, _fileName )
{
  m_tempDirectory = NULL;
  m_openAsMimeType = _openAsMimeType;
  kDebug(1601) << "CompressedFile constructor" << endl;
  m_tempDirectory = new KTempDir( _gui->tmpDir()
                          + QString::fromLatin1( "compressed_file_temp" ) );
  m_tmpdir = m_tempDirectory->name();
  initData();
  verifyUtilityIsAvailable(m_archiver_program, m_unarchiver_program);

  if (!QFile::exists(_fileName))
  {
    KMessageBox::information(0,
              i18n("You are creating a simple compressed archive which contains only one input file.\n"
                  "When uncompressed, the file name will be based on the name of the archive file.\n"
                  "If you add more files you will be prompted to convert it to a real archive."),
              i18n("Simple Compressed Archive"), "CreatingCompressedArchive");
  }
}

CompressedFile::~CompressedFile()
{
    if ( m_tempDirectory )
        delete m_tempDirectory;
}

void CompressedFile::setHeaders()
{
  ColumnList list;
  list.append(FILENAME_COLUMN);
  list.append(PERMISSION_COLUMN);
  list.append(OWNER_COLUMN);
  list.append(GROUP_COLUMN);
  list.append(SIZE_COLUMN);

  emit headers(list);
}

void CompressedFile::initData()
{
    m_unarchiver_program = QString();
    m_archiver_program = QString();

    QString mimeType;
    if ( !m_openAsMimeType.isNull() )
        mimeType = m_openAsMimeType;
    else
        mimeType = KMimeType::findByPath( m_filename )->name();

    if ( mimeType == "application/x-gzip" )
    {
        m_unarchiver_program = "gunzip";
        m_archiver_program = "gzip";
        m_defaultExtensions << ".gz" << "-gz" << ".z" << "-z" << "_z" << ".Z";
    }
    if ( mimeType == "application/x-bzip" )
    {
        m_unarchiver_program = "bunzip";
        m_archiver_program = "bzip";
        m_defaultExtensions << ".bz";
    }
    if ( mimeType == "application/x-bzip" )
    {
        m_unarchiver_program = "bunzip2";
        m_archiver_program = "bzip2";
        m_defaultExtensions << ".bz2" << ".bz";
    }
    if ( mimeType == "application/x-lzop" )
    { m_unarchiver_program = "lzop";
        m_archiver_program = "lzop";
        m_defaultExtensions << ".lzo";
    }
    if ( mimeType == "application/x-compress" )
    {
        m_unarchiver_program = "uncompress";
        m_archiver_program = "compress";
        m_defaultExtensions = QStringList( ".Z" );
    }

}

QString CompressedFile::extension()
{
  QStringList::Iterator it = m_defaultExtensions.begin();
  for( ; it != m_defaultExtensions.end(); ++it )
    if( m_filename.endsWith( *it ) )
        return QString();
  return m_defaultExtensions.first();
}

void CompressedFile::open()
{
  kDebug(1601) << "+CompressedFile::open" << endl;
  setHeaders();

  // We copy the file into the temporary directory, uncompress it,
  // and when the uncompression is done, list it
  // (that code is in the slot slotOpenDone)

  m_tmpfile = m_gui->realURL().fileName();
  if ( m_tmpfile.isEmpty() )
    m_tmpfile = m_filename;
  m_tmpfile += extension();
  m_tmpfile = m_tmpdir + m_tmpfile;

  KUrl src, target;
  src.setPath( m_filename );
  target.setPath( m_tmpfile );
  KIO::NetAccess::file_copy( m_filename, m_tmpfile, m_gui );

  kDebug(1601) << "Temp file name is " << m_tmpfile << endl;

  K3Process *kp = m_currentProcess = new K3Process;
  kp->clearArguments();
  *kp << m_unarchiver_program << "-f" ;
  if ( m_unarchiver_program == "lzop")
  {
    *kp << "-d";
    // lzop hack, see comment in tar.cpp createTmp()
    kp->setUsePty( K3Process::Stdin, false );
  }
  // gunzip 1.3 seems not to like original names with directories in them
  // testcase: https://listman.redhat.com/pipermail/valhalla-list/2006-October.txt.gz
  /*if ( m_unarchiver_program == "gunzip" )
    *kp << "-N";
  */
  *kp << m_tmpfile;

  kDebug(1601) << "Command is " << m_unarchiver_program << " " << m_tmpfile<< endl;

  connect( kp, SIGNAL(receivedStdout(K3Process*, char*, int)),
	   this, SLOT(slotReceivedOutput(K3Process*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(K3Process*, char*, int)),
	   this, SLOT(slotReceivedOutput(K3Process*, char*, int)));
  connect( kp, SIGNAL(processExited(K3Process*)), this,
	   SLOT(slotUncompressDone(K3Process*)));

  if (kp->start(K3Process::NotifyOnExit, K3Process::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Could not start a subprocess.") );
      emit sigOpen(this, false, QString(), 0 );
    }

  kDebug(1601) << "-CompressedFile::open" << endl;
}

void CompressedFile::slotUncompressDone(K3Process *_kp)
{
  bool bSuccess = false;
  kDebug(1601) << "normalExit = " << _kp->normalExit() << endl;
  if( _kp->normalExit() )
    kDebug(1601) << "exitStatus = " << _kp->exitStatus() << endl;

  if( _kp->normalExit() && (_kp->exitStatus()==0) )
  {
    bSuccess = true;
  }

  delete _kp;
  _kp = m_currentProcess = 0;

  if ( !bSuccess )
  {
      emit sigOpen( this, false, QString(), 0 );
      return;
  }

  QDir dir( m_tmpdir );
  QStringList lst( dir.entryList() );
  lst.removeAll( ".." );
  lst.removeAll( "." );
  KUrl url;
  url.setPath( m_tmpdir + lst.first() );
  m_tmpfile = url.path();
  KIO::UDSEntry udsInfo;
  KIO::NetAccess::stat( url, udsInfo, m_gui );
  KFileItem fileItem( udsInfo, url );
  QStringList list;
  list << fileItem.name();
  list << fileItem.permissionsString();
  list << fileItem.user();
  list << fileItem.group();
  list << KIO::number( fileItem.size() );
  m_gui->fileList()->addItem(list); // send to GUI

  emit sigOpen( this, bSuccess, m_filename,
                Arch::Extract | Arch::Delete | Arch::Add | Arch::View );
}

void CompressedFile::create()
{
  emit sigCreate(this, true, m_filename,
		 Arch::Extract | Arch::Delete | Arch::Add
		  | Arch::View);
}

void CompressedFile::addFile( const QStringList &urls )
{
  // only used for adding ONE file to an EMPTY gzip file, i.e., one that
  // has just been created

  kDebug(1601) << "+CompressedFile::addFile" << endl;

  Q_ASSERT(m_gui->getNumFilesInArchive() == 0);
  Q_ASSERT(urls.count() == 1);

  KUrl url(urls.first());
  Q_ASSERT(url.isLocalFile());

  QString file;
  file = url.path();

  K3Process proc;
  proc << "cp" << file << m_tmpdir;
  proc.start(K3Process::Block);

  m_tmpfile = file.right(file.length()
			 - file.lastIndexOf('/')-1);
  m_tmpfile = m_tmpdir + '/' + m_tmpfile;

  kDebug(1601) << "Temp file name is " << m_tmpfile << endl;

  kDebug(1601) << "File is " << file << endl;

  K3Process *kp = m_currentProcess = new K3Process;
  kp->clearArguments();

  // lzop hack, see comment in tar.cpp createTmp()
  if ( m_archiver_program == "lzop")
    kp->setUsePty( K3Process::Stdin, false );

  QString compressor = m_archiver_program;

  *kp << compressor << "-c" << file;

  connect( kp, SIGNAL(receivedStdout(K3Process*, char*, int)),
	   this, SLOT(slotAddInProgress(K3Process*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(K3Process*, char*, int)),
	   this, SLOT(slotReceivedOutput(K3Process*, char*, int)));
  connect( kp, SIGNAL(processExited(K3Process*)), this,
	   SLOT(slotAddDone(K3Process*)));

  int f_desc = KDE_open(QFile::encodeName(m_filename), O_CREAT | O_TRUNC | O_WRONLY, 0666);
  if (f_desc != -1)
      fd = fdopen( f_desc, "w" );
  else
      fd = NULL;

  if (kp->start(K3Process::NotifyOnExit, K3Process::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Could not start a subprocess.") );
    }

  kDebug(1601) << "-CompressedFile::addFile" << endl;
}

void CompressedFile::slotAddInProgress(K3Process*, char* _buffer, int _bufflen)
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

void CompressedFile::slotAddDone(K3Process *_kp)
{
  fclose(fd);
  slotAddExited(_kp);
}

void CompressedFile::unarchFileInternal()
{
  if (m_destDir != m_tmpdir)
    {
      QString dest;
      if (m_destDir.isEmpty() || m_destDir.isNull())
      {
          kError(1601) << "There was no extract directory given." << endl;
          return;
      }
      else
          dest=m_destDir;

      K3Process proc;
      proc << "cp" << m_tmpfile << dest;
      proc.start(K3Process::Block);
    }
  emit sigExtract(true);
}

void CompressedFile::remove(QStringList *)
{
  kDebug(1601) << "+CompressedFile::remove" << endl;
  QFile::remove(m_tmpfile);

  // delete the compressed file but then create it empty in case someone
  // does a reload and finds it no longer exists!
  QFile::remove(m_filename);

  ::close(::open(QFile::encodeName(m_filename), O_WRONLY | O_CREAT | O_EXCL));

  m_tmpfile = "";
  emit sigDelete(true);
  kDebug(1601) << "-CompressedFile::remove" << endl;
}



#include "compressedfile.moc"

