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

/* The following class is the base class for all of the archive types.
 * In order for it to work properly with the KProcess, you have to
 * connect the ProcessExited signal appropriately before spawning
 * the core operations. Then the signal that the process exited can
 * be intercepted by the viewer (in ark, ArkWidget) and dealt with
 * appropriately. See LhaArch or ZipArch for a good model. Don't use
 * TarArch or CompressedFile as models - they're too complicated!
 *
 * Don't forget to set m_archiver_program and m_unarchiver_program
 * and add a call to
 *     verifyUtilityIsAvailable(m_archiver_program, m_unarchiver_program);
 * in the constructor of your class. It's OK to leave out the second argument.
 *
 * To add a new archive type:
 * 1. Create a new header file and a source code module
 * 2. Add an entry to the ArchType enum in arch.h.
 * 3. Add appropriate types to buildFormatInfo() in archiveformatinfo.cpp
 *    and archFactory() in arch.cpp
 */


#ifndef ARCH_H
#define ARCH_H

#include <qobject.h>
#include <qptrlist.h> // Some very annoying hackery in arkwidgetpart
#include <qregexp.h>
#include <qstring.h>
#include <kurl.h>
#include <qpair.h>

class QCString;
class QStringList;
class KProcess;

class FileListView;
class ArkWidget;

enum ArchType { UNKNOWN_FORMAT, ZIP_FORMAT, TAR_FORMAT, AA_FORMAT,
                LHA_FORMAT, RAR_FORMAT, ZOO_FORMAT, COMPRESSED_FORMAT,
                SEVENZIP_FORMAT, ACE_FORMAT };

typedef QValueList< QPair< QString, Qt::AlignmentFlags > > ColumnList;

/**
 * Pure virtual base class for archives - provides a framework as well as
 * useful common functionality.
 */
class Arch : public QObject
{
  Q_OBJECT

  protected:
    /**
     * A struct representing column data. This makes it possible to abstract
     * archive output, and save writing the same function for every archive
     * type. It is also much more robust than sscanf (which was breaking).
     */
    struct ArchColumns
    {
      int colRef; // Which column to load to in processLine
      QRegExp pattern;
      int maxLength;
      bool optional;

      ArchColumns( int col, QRegExp reg, int length = 64, bool opt = false );
    };

  public:
    Arch( ArkWidget *_viewer, const QString & _fileName );
    virtual ~Arch();

    virtual void open() = 0;
    virtual void create() = 0;
    virtual void remove( QStringList * ) = 0;

    virtual void addFile( const QStringList & ) = 0;
    virtual void addDir( const QString & ) = 0;

    // unarch the files in m_fileList or all files if m_fileList is empty.
    // if m_destDir is empty, abort with error.
    // m_viewFriendly forces certain options like directory junking required by view/edit
    virtual void unarchFileInternal() = 0;
    // returns true if a password is required
    virtual bool passwordRequired() { return false; }

    void unarchFile( QStringList *, const QString & _destDir,
                             bool viewFriendly = false );

    QString fileName() const { return m_filename; }

    enum EditProperties{ Add = 1, Delete = 2, Extract = 4,
                         View = 8, Integrity = 16 };

    // is the archive readonly?
    bool isReadOnly() { return m_bReadOnly; }
    void setReadOnly( bool bVal ) { m_bReadOnly = bVal; }

    bool isError() { return m_error; }
    void resetError() { m_error = false; }

    // check to see if the utility exists in the PATH of the user
    void verifyUtilityIsAvailable( const QString &,
                                   const QString & = QString::null );

    void verifyCompressUtilityIsAvailable( const QString &utility );

    void verifyUncompressUtilityIsAvailable( const QString &utility );

    bool archUtilityIsAvailable() { return m_bArchUtilityIsAvailable; }

    bool unarchUtilityIsAvailable() { return m_bUnarchUtilityIsAvailable; }

    QString getArchUtility() { return m_archiver_program; }

    QString getUnarchUtility() { return m_unarchiver_program; }

    void appendShellOutputData( const char * data ) { m_lastShellOutput.append( QString::fromLocal8Bit( data ) ); }
    void clearShellOutput() { m_lastShellOutput.truncate( 0 ); }
    const QString& getLastShellOutput() const { return m_lastShellOutput; }

    static Arch *archFactory( ArchType aType, ArkWidget *parent,
                              const QString &filename,
                              const QString &openAsMimeType = QString::null );

  protected slots:
    void slotOpenExited( KProcess* );
    void slotExtractExited( KProcess* );
    void slotDeleteExited( KProcess* );
    void slotAddExited( KProcess* );

    void slotReceivedOutput( KProcess *, char*, int );

    virtual bool processLine( const QCString &line );
    virtual void slotReceivedTOC( KProcess *, char *, int );

  signals:
    void sigOpen( Arch * archive, bool success, const QString &filename, int );
    void sigCreate( Arch *, bool, const QString &, int );
    void sigDelete( bool );
    void sigExtract( bool );
    void sigAdd( bool );
    void headers( const ColumnList& columns );

  protected:  // data
    QString m_filename;
    QString m_lastShellOutput;
    QCString m_buffer;
    ArkWidget *m_gui;
    bool m_bReadOnly; // for readonly archives
    bool m_error;

    // lets tar delete unsuccessfully before adding without confusing the user
    bool m_bNotifyWhenDeleteFails;

    // set to whether if the compressing utility is in the user's PATH
    bool m_bArchUtilityIsAvailable;

    // set to whether if the uncompressing utility is in the user's PATH
    bool m_bUnarchUtilityIsAvailable;

    QString m_archiver_program;
    QString m_unarchiver_program;

    // Archive parsing information
    QCString m_headerString;
    bool m_header_removed, m_finished;
    QPtrList<ArchColumns> m_archCols;
    int m_numCols, m_dateCol, m_fixYear, m_fixMonth, m_fixDay, m_fixTime;
    int m_repairYear, m_repairMonth, m_repairTime;
    KProcess *m_currentProcess;
    QStringList *m_fileList;
    QString m_destDir;
    bool m_viewFriendly;
    QCString m_password;
};

// Columns
// don't forget to change common_texts.cpp if you change something here
#define FILENAME_COLUMN    qMakePair( i18n(" Filename "),    Qt::AlignLeft  )
#define PERMISSION_COLUMN  qMakePair( i18n(" Permissions "), Qt::AlignLeft  )
#define OWNER_GROUP_COLUMN qMakePair( i18n(" Owner/Group "), Qt::AlignLeft  )
#define SIZE_COLUMN        qMakePair( i18n(" Size "),        Qt::AlignRight )
#define TIMESTAMP_COLUMN   qMakePair( i18n(" Timestamp "),   Qt::AlignRight )
#define LINK_COLUMN        qMakePair( i18n(" Link "),        Qt::AlignLeft  )
#define PACKED_COLUMN      qMakePair( i18n(" Size Now "),    Qt::AlignRight )
#define RATIO_COLUMN       qMakePair( i18n(" Ratio "),       Qt::AlignRight )
#define CRC_COLUMN         qMakePair( i18n("Cyclic Redundancy Check"," CRC "), Qt::AlignLeft )
#define METHOD_COLUMN      qMakePair( i18n(" Method "),  Qt::AlignLeft  )
#define VERSION_COLUMN     qMakePair( i18n(" Version "), Qt::AlignLeft  )
#define OWNER_COLUMN       qMakePair( i18n(" Owner "),   Qt::AlignLeft  )
#define GROUP_COLUMN       qMakePair( i18n(" Group "),   Qt::AlignLeft  )

#endif /* ARCH_H */
