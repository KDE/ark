/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#include "bkplugin.h"
#include "kerfuffle/archivefactory.h"

#include <QFile>
#include <KDebug>

BKInterface::BKInterface( const QString & filename, QObject *parent )
	: ReadWriteArchiveInterface( filename, parent )
{
}

BKInterface::~BKInterface()
{
	bk_destroy_vol_info( &m_volInfo );
}

bool BKInterface::list()
{
	int rc;

	rc = bk_init_vol_info( &m_volInfo, true );
	if ( rc <= 0 ) return false;

	rc = bk_open_image( &m_volInfo, filename().toAscii().constData() );
	if ( rc <= 0 ) return false;

	rc = bk_read_vol_info( &m_volInfo );
	if ( rc <= 0 ) return false;

	if(m_volInfo.filenameTypes & FNTYPE_ROCKRIDGE)
		rc = bk_read_dir_tree( &m_volInfo, FNTYPE_ROCKRIDGE, true, 0 );
	else if(m_volInfo.filenameTypes & FNTYPE_JOLIET)
		rc = bk_read_dir_tree( &m_volInfo, FNTYPE_JOLIET, false, 0 );
	else
		rc = bk_read_dir_tree( &m_volInfo, FNTYPE_9660, false, 0 );
	if(rc <= 0) return false;

	return browse( BK_BASE_PTR( &( m_volInfo.dirTree ) ) );
}

bool BKInterface::copyFiles( const QList<QVariant> & files, const QString & destinationDirectory, bool preservePaths )
{
	foreach( const QVariant& file, files )
	{
		int rc;

		kDebug( 1601 ) << "Trying to extract " << file.toByteArray() ;
		rc = bk_extract( &m_volInfo, file.toByteArray(), QFile::encodeName( destinationDirectory ), true, 0 );
		if ( rc <= 0 )
		{
			error( QString( "Could not extract '%1'" ).arg( file.toString() ) );
			return false;
		}
	}
	return true;
}

bool BKInterface::browse( BkFileBase* base, const QString& prefix )
{
	QString name( base->name );
	QString fullpath = prefix.isEmpty()? name : prefix + '/' + name;
	if ( !name.isEmpty() )
	{
		ArchiveEntry e;
		e[ FileName ] = fullpath;
		e[ InternalID ] = '/'+fullpath;

		if ( IS_SYMLINK( base->posixFileMode ) )
		{
			e[ Link ] = QByteArray( BK_SYMLINK_PTR( base )->target );
		}
		if ( IS_REG_FILE( base->posixFileMode ) )
		{
			e[ Size ] = ( qulonglong ) BK_FILE_PTR( base )->size;
		}
		if ( IS_DIR( base->posixFileMode ) )
		{
			e[ IsDirectory ] = true;
		}

		entry( e );
	}

	if ( IS_DIR( base->posixFileMode ) )
	{
		BkFileBase *child = BK_DIR_PTR( base )->children;
		while ( child )
		{
			if ( !browse( child, fullpath ) )
			{
				return false;
			}
			child = child->next;
		}
	}

	return true;
}

bool BKInterface::addFiles( const QStringList & files )
{
  return false;
}

bool BKInterface::deleteFiles( const QList<QVariant> & files )
{
  return false;
}

KERFUFFLE_PLUGIN_FACTORY( BKInterface )

