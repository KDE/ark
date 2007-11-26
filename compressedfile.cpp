/*
    ark: A program for modifying archives via a GUI.

    Copyright (C)

    2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
    2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
    2003: Georg Robbers <Georg.Robbers@urz.uni-hd.de>

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

// C includes
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

// Qt includes
#include <qdir.h>

// KDE includes
#include <kdebug.h>
#include <klargefile.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <ktempdir.h>
#include <kprocess.h>
#include <kmimetype.h>
#include <kio/netaccess.h>
#include <kio/global.h>
#include <kfileitem.h>
#include <kapplication.h>
#include <kglobal.h>

// ark includes
#include "arkwidget.h"
#include "filelistview.h"
#include "compressedfile.h"

// encapsulates the idea of a compressed file

CompressedFile::CompressedFile( ArkWidget *_gui, const QString & _fileName, const QString & _openAsMimeType )
  : Arch( _gui, _fileName )
{
  m_tempDirectory = NULL;
  m_openAsMimeType = _openAsMimeType;
  kdDebug(1601) << "CompressedFile constructor" << endl;
  m_tempDirectory = new KTempDir( _gui->tmpDir()
                          + QString::fromLatin1( "compressed_file_temp" ) );
  m_tempDirectory->setAutoDelete( true );
  m_tmpdir = m_tempDirectory->name();
  initData();
  verifyCompressUtilityIsAvailable( m_archiver_program );
  verifyUncompressUtilityIsAvailable( m_unarchiver_program );

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
    m_unarchiver_program = QString::null;
    m_archiver_program = QString::null;

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
    if ( mimeType == "application/x-bzip2" )
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
        m_unarchiver_program = KGlobal::dirs()->findExe( "uncompress" ).isNull()? "gunzip" : "uncompress";
        m_archiver_program   = "compress";
        m_defaultExtensions  = ".Z";
    }

}

QString CompressedFile::extension()
{
  QStringList::Iterator it = m_defaultExtensions.begin();
  for( ; it != m_defaultExtensions.end(); ++it )
    if( m_filename.endsWith( *it ) )
        return QString::null;
  return m_defaultExtensions.first();
}

void CompressedFile::open()
{
  kdDebug(1601) << "+CompressedFile::open" << endl;
  setHeaders();

  // We copy the file into the temporary directory, uncompress it,
  // and when the uncompression is done, list it
  // (that code is in the slot slotOpenDone)

  m_tmpfile = m_gui->realURL().fileName();
  if ( m_tmpfile.isEmpty() )
    m_tmpfile = m_filename;
  m_tmpfile += extension();
  m_tmpfile = m_tmpdir + m_tmpfile;

  KURL src, target;
  src.setPath( m_filename );
  target.setPath( m_tmpfile );

  KIO::NetAccess::copy( src, target, m_gui );
  kdDebug(1601) << "Temp file name is " << target << endl;

  if ( !KIO::NetAccess::exists( target, true, NULL ) )
    return;

  KProcess *kp = m_currentProcess = new KProcess;
  kp->clearArguments();
  *kp << m_unarchiver_program << "-f" ;
  if ( m_unarchiver_program == "lzop")
  {
    *kp << "-d";
    // lzop hack, see comment in tar.cpp createTmp()
    kp->setUsePty( KProcess::Stdin, false );
  }
  // gunzip 1.3 seems not to like original names with directories in them
  // testcase: https://listman.redhat.com/pipermail/valhalla-list/2006-October.txt.gz
  /*if ( m_unarchiver_program == "gunzip" )
    *kp << "-N";
  */
  *kp << m_tmpfile;

  kdDebug(1601) << "Command is " << m_unarchiver_program << " " << m_tmpfile<< endl;

  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
  connect( kp, SIGNAL(processExited(KProcess*)), this,
	   SLOT(slotUncompressDone(KProcess*)));

  if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Could not start a subprocess.") );
      emit sigOpen(this, false, QString::null, 0 );
    }

  kdDebug(1601) << "-CompressedFile::open" << endl;
}

void CompressedFile::slotUncompressDone(KProcess *_kp)
{
  bool bSuccess = false;
  kdDebug(1601) << "normalExit = " << _kp->normalExit() << endl;
  if( _kp->normalExit() )
    kdDebug(1601) << "exitStatus = " << _kp->exitStatus() << endl;

  if( _kp->normalExit() && (_kp->exitStatus()==0) )
  {
    bSuccess = true;
  }

  delete _kp;
  _kp = m_currentProcess = 0;

  if ( !bSuccess )
  {
      emit sigOpen( this, false, QString::null, 0 );
      return;
  }

  QDir dir( m_tmpdir );
  QStringList lst( dir.entryList() );
  lst.remove( ".." );
  lst.remove( "." );
  KURL url;
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

  kdDebug(1601) << "+CompressedFile::addFile" << endl;

  Q_ASSERT(m_gui->getNumFilesInArchive() == 0);
  Q_ASSERT(urls.count() == 1);

  KURL url = KURL::fromPathOrURL(urls.first());
  Q_ASSERT(url.isLocalFile());

  QString file;
  file = url.path();

  KProcess proc;
  proc << "cp" << file << m_tmpdir;
  proc.start(KProcess::Block);

  m_tmpfile = file.right(file.length()
			 - file.findRev("/")-1);
  m_tmpfile = m_tmpdir + '/' + m_tmpfile;

  kdDebug(1601) << "Temp file name is " << m_tmpfile << endl;

  kdDebug(1601) << "File is " << file << endl;

  KProcess *kp = m_currentProcess = new KProcess;
  kp->clearArguments();

  // lzop hack, see comment in tar.cpp createTmp()
  if ( m_archiver_program == "lzop")
    kp->setUsePty( KProcess::Stdin, false );

  QString compressor = m_archiver_program;

  *kp << compressor << "-c" << file;

  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotAddInProgress(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
  connect( kp, SIGNAL(processExited(KProcess*)), this,
	   SLOT(slotAddDone(KProcess*)));

  int f_desc = KDE_open(QFile::encodeName(m_filename), O_CREAT | O_TRUNC | O_WRONLY, 0666);
  if (f_desc != -1)
      fd = fdopen( f_desc, "w" );
  else
      fd = NULL;

  if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Could not start a subprocess.") );
    }

  kdDebug(1601) << "-CompressedFile::addFile" << endl;
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

void CompressedFile::unarchFileInternal()
{
  if (m_destDir != m_tmpdir)
    {
      QString dest;
      if (m_destDir.isEmpty() || m_destDir.isNull())
      {
          kdError(1601) << "There was no extract directory given." << endl;
          return;
      }
      else
          dest=m_destDir;

      KProcess proc;
      proc << "cp" << m_tmpfile << dest;
      proc.start(KProcess::Block);
    }
  emit sigExtract(true);
}

void CompressedFile::remove(QStringList *)
{
  kdDebug(1601) << "+CompressedFile::remove" << endl;
  QFile::remove(m_tmpfile);

  // do not delete but truncate the compressed file in case someone
  // does a reload and finds it no longer exists!
  truncate(QFile::encodeName(m_filename), 0);

  m_tmpfile = "";
  emit sigDelete(true);
  kdDebug(1601) << "-CompressedFile::remove" << endl;
}



#include "compressedfile.moc"

