/*

 ark -- archiver for the KDE project

 Copyright (C) 1997-1999 Rob Palmbos <palm9744@kettering.edu>
 Copyright (C) 1999 Francois-Xavier Duranceau <duranceau@kde.org>
 Copyright (C) 1999-2000 Corel Corporation (author: Emily Ezust <emilye@corel.com>)
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

/* The following class is the base class for all of the archive types.
 * In order for it to work properly with the K3Process, you have to
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

#include <QObject>
#include <QList>
#include <QPair>
#include <QRegExp>
#include <QVariant>

#include <KUrl>

class QByteArray;
class QStringList;
class K3Process;

class ArkWidget;

enum ArchType { UNKNOWN_FORMAT, ZIP_FORMAT, TAR_FORMAT, AA_FORMAT,
                LHA_FORMAT, RAR_FORMAT, ZOO_FORMAT, COMPRESSED_FORMAT,
                SEVENZIP_FORMAT, ACE_FORMAT };

enum EntryMetaDataType { FileName = 0, OriginalFileName = 1, Permissions = 2, Owner = 3,
                         Group = 4, Size = 5, CompressedSize = 6, Link = 7, Ratio = 8,
                         CRC = 9, Method = 10, Version = 11, Timestamp = 12, Custom = 1048576 };

typedef QHash<int, QVariant> ArchiveEntry;
typedef QList< QPair< QString, Qt::AlignmentFlag > > ColumnList;

/**
 * Pure virtual base class for archives - provides a framework as well as
 * useful common functionality.
 */
class Arch : public QObject
{
  Q_OBJECT

  public:
    Arch( ArkWidget *_viewer, const QString & _fileName );
    virtual ~Arch();

    virtual void open() = 0;
    virtual void create() = 0;
    virtual void remove( QStringList * ) = 0;

    virtual void addFile( const QStringList & ) = 0;
    virtual void addDir( const QString & ) = 0;

    virtual void extractFile( const QStringList &, const QString & _destDir ) = 0;

    virtual bool passwordRequired() { return false; }

    QString fileName() const { return m_filename; }

    enum EditProperties{ Add = 1, Delete = 2, Extract = 4,
                         View = 8, Integrity = 16 };

    bool isReadOnly() { return m_readOnly; }
    void setReadOnly( bool readOnly ) { m_readOnly = readOnly; }

    static Arch *archFactory( ArchType aType, ArkWidget *parent,
                              const QString &filename,
                              const QString &openAsMimeType = QString() );

  signals:
    void sigOpen( Arch * archive, bool success, const QString &filename, int );
    void sigCreate( Arch *, bool, const QString &, int );
    void sigDelete( bool );
    void sigExtract( bool );
    void sigAdd( bool );
    void newEntry( const ArchiveEntry& entry );

  protected:  // data
    QString m_filename;
    ArkWidget *m_gui;
    bool m_readOnly; // for readonly archives
};

// Columns
#define FILENAME_COLUMN    qMakePair( i18n(" Filename "),    Qt::AlignLeft  )
#define PERMISSION_COLUMN  qMakePair( i18n(" Permissions "), Qt::AlignLeft  )
#define OWNER_GROUP_COLUMN qMakePair( i18n(" Owner/Group "), Qt::AlignLeft  )
#define SIZE_COLUMN        qMakePair( i18n(" Size "),        Qt::AlignRight )
#define TIMESTAMP_COLUMN   qMakePair( i18n(" Timestamp "),   Qt::AlignRight )
#define LINK_COLUMN        qMakePair( i18n(" Link "),        Qt::AlignLeft  )
#define PACKED_COLUMN      qMakePair( i18n(" Size Now "),    Qt::AlignRight )
#define RATIO_COLUMN       qMakePair( i18n(" Ratio "),       Qt::AlignRight )
#define CRC_COLUMN         qMakePair( i18nc("Cyclic Redundancy Check"," CRC "), Qt::AlignRight )
#define METHOD_COLUMN      qMakePair( i18n(" Method "),  Qt::AlignLeft  )
#define VERSION_COLUMN     qMakePair( i18n(" Version "), Qt::AlignLeft  )
#define OWNER_COLUMN       qMakePair( i18n(" Owner "),   Qt::AlignLeft  )
#define GROUP_COLUMN       qMakePair( i18n(" Group "),   Qt::AlignLeft  )

#endif /* ARCH_H */
