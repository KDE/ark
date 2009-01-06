/*
 * 7zipplugin -- plugin for KDE ark
 *
 * Copyright (C) 2008 Chen Yew Ming <fusion82@gmail.com>
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
#ifndef _7ZIPPLUGIN_H
#define _7ZIPPLUGIN_H

#include "kerfuffle/archiveinterface.h"
#include <QProcess>
class QByteArray;
class KProcess;
class KPtyProcess;
class QEventLoop;

using namespace Kerfuffle;


class p7zipInterface: public ReadWriteArchiveInterface
{
	Q_OBJECT
	public:
		explicit p7zipInterface( const QString & filename, QObject *parent = 0 );
		~p7zipInterface();

		bool list();
		bool copyFiles( const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options );

		bool addFiles( const QStringList & files, const CompressionOptions& options );
		bool deleteFiles( const QList<QVariant> & files );

	private:
		void listProcessLine(int& state, const QString& line);
		void writeToProcess( const QByteArray &data );
		bool create7zipProcess();
		bool execute7zipProcess(const QStringList & args);
		bool handlePasswordPrompt(QByteArray &message);
		QString m_exepath;
		ArchiveEntry m_currentArchiveEntry;
		QByteArray m_stdOutData;
		QByteArray m_stdErrData;
		QEventLoop *m_loop;
		int m_state;
		QStringList m_errorMessages;
		QList<QVariant> m_archiveContents;
		
		unsigned int m_totalFilesCount;
		unsigned int m_progressFilesCount;
		bool m_userCancelled;
		

	#if defined(Q_OS_WIN)
		KProcess *m_process;
	#else
		KPtyProcess *m_process;
	#endif

	private slots:
		void started();
		void listReadStdout();
		void copyReadStdout();
		void addReadStdout();
		void deleteReadStdout();
		void readFromStderr();
		void finished( int exitCode, QProcess::ExitStatus exitStatus );
};

#endif // SEVENZIPPLUGIN_H
