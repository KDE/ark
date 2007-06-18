#ifndef ARCHIVEINTERFACE_H
#define ARCHIVEINTERFACE_H

#include <QObject>
#include <QStringList>
#include <QString>

#include "arch.h"

class ArchiveObserver
{
	public:
		ArchiveObserver() {}
		virtual ~ArchiveObserver() {}

		virtual void onError( const QString & message, const QString & details ) = 0;
		virtual void onEntry( const ArchiveEntry & archiveEntry ) = 0;
		virtual void onProgress( double ) = 0;
};

class ReadOnlyArchiveInterface: public QObject
{
	Q_OBJECT
	public:
		ReadOnlyArchiveInterface( const QString & filename, QObject *parent = 0 );
		virtual ~ReadOnlyArchiveInterface();

		QString filename() const { return m_filename; }
		virtual bool isReadOnly() const { return true; }

		void registerObserver( ArchiveObserver *observer );
		void removeObserver( ArchiveObserver *observer );

		virtual bool open() { return true; }
		virtual bool list() = 0;
		virtual bool copyFiles( const QStringList & files, const QString & destinationDirectory ) = 0;

	protected:
		void error( const QString & message, const QString & details );
		void entry( const ArchiveEntry & archiveEntry );
		void progress( double );

	private:
		QList<ArchiveObserver*> m_observers;
		QString m_filename;
};
/*
class ReadWriteArchiveInterface: public ReadOnlyArchiveInterface
{
	Q_OBJECT
};
*/
#endif // ARCHIVEINTERFACE_H
