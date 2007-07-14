#ifndef BKPLUGIN_H
#define BKPLUGIN_H

#include "archiveinterface.h"
extern "C"
{
#include "bk/bk.h"
}

class BKInterface: public ReadOnlyArchiveInterface
{
	Q_OBJECT
	public:
		BKInterface( const QString & filename, QObject *parent = 0 );
		~BKInterface();

		bool list();
		bool copyFiles( const QList<QVariant> & files, const QString & destinationDirectory );

	private:
		bool browse( BkFileBase* base, const QString& prefix = QString() );
};

#endif // BKPLUGIN_H
