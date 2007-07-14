#include "bkplugin.h"
#include <kdebug.h>

BKInterface::BKInterface( const QString & filename, QObject *parent )
	: ReadOnlyArchiveInterface( filename, parent )
{
}

BKInterface::~BKInterface()
{
}

bool BKInterface::list()
{
	VolInfo volInfo;
	int rc;

	rc = bk_init_vol_info( &volInfo, true );
	if ( rc <= 0 ) return false;

	rc = bk_open_image( &volInfo, filename().toAscii().constData() );
	if ( rc <= 0 ) return false;

	rc = bk_read_vol_info( &volInfo );
	if ( rc <= 0 ) return false;

	if(volInfo.filenameTypes & FNTYPE_ROCKRIDGE)
		rc = bk_read_dir_tree( &volInfo, FNTYPE_ROCKRIDGE, true, 0 );
	else if(volInfo.filenameTypes & FNTYPE_JOLIET)
		rc = bk_read_dir_tree( &volInfo, FNTYPE_JOLIET, false, 0 );
	else
		rc = bk_read_dir_tree( &volInfo, FNTYPE_9660, false, 0 );
	if(rc <= 0) return false;


	kDebug( 1601 ) << k_funcinfo << "Let's browse!" << endl;
	bool result = browse( BK_BASE_PTR( &( volInfo.dirTree ) ) );

	bk_destroy_vol_info( &volInfo );

	return result;
}

bool BKInterface::copyFiles( const QList<QVariant> & files, const QString & destinationDirectory )
{
	error( "Not implemented yet" );
	return false;
}

bool BKInterface::browse( BkFileBase* base, const QString& prefix )
{
	QString name( base->name );
	QString fullpath = prefix.isEmpty()? name : prefix + '/' + name;
	if ( !name.isEmpty() )
	{
		ArchiveEntry e;
		e[ FileName ] = fullpath;
		e[ OriginalFileName ] = fullpath;

		kDebug( 1601 ) << k_funcinfo << "Browsing " << base->name << endl;
		kDebug( 1601 ) << k_funcinfo << "         which " << ( IS_DIR( base->posixFileMode )? "is " : "is not " ) << " a dir. " << endl;
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
			if ( !QString( base->name ).endsWith( '/' ) )
			{
				e[ FileName ] = e[ FileName ].toString() + '/';
			}
		}

		entry( e );
	}

	if ( IS_DIR( base->posixFileMode ) )
	{
		BkFileBase *child = BK_DIR_PTR( base )->children;
		kDebug( 1601 ) << k_funcinfo << "children is " << ( long long ) child << endl;
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

