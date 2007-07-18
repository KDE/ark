/*

 ark -- archiver for the KDE project

 Copyright (C) 1997-1999 Rob Palmbos <palm9744@kettering.edu>
 Copyright (C) 1999 Francois-Xavier Duranceau <duranceau@kde.org>
 Copyright (C) 1999-2000 Corel Corporation (author: Emily Ezust <emilye@corel.com>)
 Copyright (C) 2001 Corel Corporation (author: Michael Jarrett <michaelj@corel.com>)
 Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>

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

#ifndef ARCH_H
#define ARCH_H

#include <QObject>
#include <QList>
#include <QStringList>
#include <QPair>
#include <QRegExp>
#include <QHash>
#include <QVariant>

#include <KUrl>

#include "kerfuffle_export.h"

class QStringList;

enum ArchType { UNKNOWN_FORMAT, ZIP_FORMAT, TAR_FORMAT, AA_FORMAT,
                LHA_FORMAT, RAR_FORMAT, ZOO_FORMAT, COMPRESSED_FORMAT,
                SEVENZIP_FORMAT, ACE_FORMAT };

enum EntryMetaDataType { FileName = 0, OriginalFileName = 1, Permissions = 2, Owner = 3,
                         Group = 4, Size = 5, CompressedSize = 6, Link = 7, Ratio = 8,
                         CRC = 9, Method = 10, Version = 11, Timestamp = 12, IsDirectory = 13, Custom = 1048576 };

typedef QHash<int, QVariant> ArchiveEntry;

/**
 * Pure virtual base class for archives - provides a framework as well as
 * useful common functionality.
 */
class KERFUFFLE_EXPORT Arch : public QObject
{
	Q_OBJECT

	protected:
		/* Creates an object representing an archive for _fileName */
		Arch( const QString & _fileName );

	public:
		virtual ~Arch();

		/* Starts an open() operation.
		 * This should list all archive entries ( by emmiting newEntry() ) and then emit
		 * opened() so that the GUI can know that the operation is over.
		 */
		virtual void open() = 0;
		virtual void create() = 0;
		virtual void remove( const QStringList & ) = 0;

		virtual void addFile( const QStringList & ) = 0;
		virtual void addDir( const QString & ) = 0;

		virtual void extractFile( const QVariant & fileName, const QString & destinationDir );
		virtual void extractFiles( const QList<QVariant> & fileList, const QString & destinationDir ) = 0;

		virtual bool passwordRequired() { return false; }

		QString fileName() const { return m_filename; }

		bool isReadOnly() { return m_readOnly; }
		static Arch *factory( const QString & filename, const QString & requestedMimeType = QString() );
		static QStringList supportedMimeTypes();

	signals:
		void opened( bool success );
		void sigCreate( Arch *, bool, const QString &, int );
		void sigDelete( bool );
		void sigExtract( bool );
		void sigAdd( bool );
		void newEntry( const ArchiveEntry& entry );
		void error( const QString& error, const QString& details );

	protected:
		void setReadOnly( bool readOnly ) { m_readOnly = readOnly; }

		QString m_filename;
		bool m_readOnly; // for readonly archives
};

#endif /* ARCH_H */
