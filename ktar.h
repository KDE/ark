/* This file is part of the KDE libraries
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#ifndef __ktar_h
#define __ktar_h

#include <sys/stat.h>
#include <sys/types.h>
#include <zlib.h>

#include <qdatetime.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qdict.h>

 // just to get different symbols from libkio for now
#define KTarGz KTarGz2
#define KTarBase KTarBase2
#define KTarDirectory KTarDirectory2
#define KTarFile KTarFile2
#define KTarEntry KTarEntry2

class KTarDirectory;
class KTarFile;

/**
 * @short generic class for reading/writing tar archives
 * @author David Faure <faure@kde.org>
 */
class KTarBase
{
protected:
  // Deprecated
  KTarBase();
  KTarBase(QIODevice * dev);
  virtual ~KTarBase();

public:
  /**
   * Opens the tar file/data for reading or writing.
   *
   * @param mode may be IO_ReadOnly or IO_WriteOnly
   *
   * @see #close
   */
  virtual bool open( int mode );

  /**
   * Closes the tar file/data.
   *
   * @see #open
   */
  virtual void close();

  /**
   * @return true if the file is opened
   */
  bool isOpened() const { return m_open; }


  /**
   * If a tar file is opened for writing then you can add new directories
   * using this function. KTar won't write one directory twice.
   */
  void writeDir( const QString& name, const QString& user, const QString& group );
  /**
   * If a tar file is opened for writing then you can add a new file
   * using this function. If the file name is for example "mydir/test1" then
   * the directory "mydir" is automatically appended first if that did not
   * happen yet.
   */
  void writeFile( const QString& name, const QString& user, const QString& group, uint size, const char* data );

  /**
   * If a tar file is opened for reading, then the contents
   * of the file can be accessed via this function.
   */
  const KTarDirectory* directory() const;

  QIODevice * device() const;
  void setDevice( QIODevice * dev );

protected:
  /**
   * Read @p len data into @p buffer - reimplemented
   * @return length read
   */
  virtual int read( char * buffer, int len ) = 0;

  /**
   * Write @p len data from @p buffer - reimplemented
   */
  virtual void write( const char * buffer, int len ) = 0;

  /**
   * @return the current position - reimplemented
   */
  virtual int position() = 0;

  /**
   * Ensure path exists, create otherwise.
   * This handles tar files missing directory entries, like mico-2.3.0.tar.gz :)
   * @internal
   */
  KTarDirectory * findOrCreate( const QString & path );

  /**
   * @internal
   * Fills @p buffer for writing a file as required by the tar format
   * Has to be called LAST, since it does the checksum
   * (normally, only the name has to be filled in before)
   * @param mode is expected to be 6 chars long, [uname and gname 31].
   */
  void fillBuffer( char * buffer, const char * mode, int size, char typeflag, const char * uname, const char * gname );

  class KTarBasePrivate;
  KTarBasePrivate * d;
  bool m_open;
  QStringList m_dirList;
  char m_mode;
};

/**
 * @short A class for reading/writing optionnally-gzipped tar balls.
 * Should be named KTar and be merged back with KTarBase - in KDE 3.0.
 * @author Torben Weis <weis@kde.org>
 */
class KTarGz : public KTarBase
{
public:
  /**
   * Creates an instance that operates on the given filename.
   *
   * @param filename has the format "/home/weis/myfile.tgz" or something
   *        like that.
   *
   * @see #open
   */
  KTarGz( const QString& filename );

  /**
   * Creates an instance that operates on the given device.
   * The device can be compressed (KFilterDev) or not (QFile, etc.).
   */
  KTarGz( QIODevice * dev );

  /**
   * If the tar ball is still opened, then it will be
   * closed automatically by the destructor.
   */
  virtual ~KTarGz();

  /**
   * The name of the tar file, as passed to the constructor
   * Null if you used the QIODevice constructor.
   */
  QString fileName() { return m_filename; }

private:
  /**
   * Read @p len data into @p buffer
   * @return length read
   */
  virtual int read( char * buffer, int len );

  /**
   * Write @p len data from @p buffer
   */
  virtual void write( const char * buffer, int len );

  /**
   * @return the current position
   */
  virtual int position();

  class KTarGzPrivate;
  KTarGzPrivate * d;
  QString m_filename;
};

/**
 * @short Base class for the tar-file's directory structure.
 *
 * @see KTarFile
 * @see KTarDirectory
 */
class KTarEntry
{
public:
  KTarEntry( KTarBase* tar, const QString& name, int access, int date,
             const QString& user, const QString& group,
             const QString &symlink );

  virtual ~KTarEntry() { }

  /**
   * Creation date of the file.
   */
  QDateTime datetime() const;
  int date() const { return m_date; }

  /**
   * Name of the file without path.
   */
  QString name() const { return m_name; }
  /**
   * The permissions and mode flags as returned by the stat() function
   * in st_mode.
   */
  mode_t permissions() const { return m_access; }
  /**
   * User who created the file.
   */
  QString user() const { return m_user; }
  /**
   * Group of the user who created the file.
   */
  QString group() const { return m_group; }

  /**
   * Symlink if there is one
   */
  QString symlink() const { return m_symlink; }

  /**
   * @return true if this entry is a file
   */
  virtual bool isFile() const { return false; }
  /**
   * @return true if this entry is a directory
   */
  virtual bool isDirectory() const { return false; }

protected:
  KTarBase* tar() const { return m_tar; }

private:
  QString m_name;
  int m_date;
  mode_t m_access;
  QString m_user;
  QString m_group;
  QString m_symlink;
  KTarBase* m_tar;
};

/**
 * @short A file in a tar-file's directory structure.
 *
 * @see KTar
 * @see KTarDirectory
 */
class KTarFile : public KTarEntry
{
public:
  KTarFile( KTarBase* tar, const QString& name, int access, int date,
            const QString& user, const QString& group, const QString &symlink,
            int pos, int size, const QByteArray& /* remove me */);

  virtual ~KTarFile() { }

  /**
   * Position of the data in the uncompressed tar file.
   */
  int position() const;
  /**
   * Size of the data.
   */
  int size() const;

  /**
   * @return the content of this file.
   */
  QByteArray data() const;

  /**
   * TODO : a method that returns a KLimitedIODevice (to be written)
   * on top of the underlying QIODevice, to be used in e.g. koffice
   */

  /**
   * @return true, since this entry is a file
   */
  virtual bool isFile() const { return true; }

private:
  int m_pos;
  int m_size;
  QByteArray m_data; //remove-me
};

/**
 * @short A directory in a tar-file's directory structure.
 *
 * @see KTar
 * @see KTarFile
 */
class KTarDirectory : public KTarEntry
{
public:
  KTarDirectory( KTarBase* tar, const QString& name, int access, int date,
                 const QString& user, const QString& group,
                 const QString& symlink);

  virtual ~KTarDirectory() { }

  /**
   * @return the names of all entries in this directory (filenames, no path).
   */
  QStringList entries() const;
  /**
   * @return a pointer to the entry in the directory.
   *
   * @param name may be "test1", "mydir/test3", "mydir/mysubdir/test3", etc.
   */
  KTarEntry* entry( QString name );
  const KTarEntry* entry( QString name ) const;

  /**
   * @internal
   * Adds a new entry to the directory.
   */
  void addEntry( KTarEntry* );

  /**
   * @return true, since this entry is a directory
   */
  virtual bool isDirectory() const { return true; }

private:
  QDict<KTarEntry> m_entries;
};

#endif
