/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal
 * <haraldhv atatatat stud.ntnu.no>
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
#ifndef _CLIINTERFACE_H_
#define _CLIINTERFACE_H_

#include "archiveinterface.h"
#include "kerfuffle_export.h"
#include <kptyprocess.h>
#include <QEventLoop>

namespace Kerfuffle
{

	enum CliInterfaceExtractOptions {
	};

	enum CliInterfaceParameters {
		
		///////////////[ COMMON ]/////////////
		
		/**
		 * Bool (default false)
		 * Will look for the %-sign in the stdout while working, in the form of
		 * (2%, 14%, 35%, etc etc), and report progress based upon this
		 */
		CaptureProgress = 0,

		///////////////[ LIST ]/////////////

		/**
		 * QString
		 * The name to the program that will handle listing of this
		 * archive (eg "rar"). Will be searched for in PATH
		 */
		ListProgram,
		/**
		 * QStringList
		 * The arguments that are passed to the program above for
		 * listing the archive. Special strings that will be
		 * substituted:
		 * $Archive - the path of the archive
		 */
		ListArgs,

		///////////////[ EXTRACT ]/////////////

		/**
		 * QString
		 * The name to the program that will handle extracting of this
		 * archive (eg "rar"). Will be searched for in PATH
		 */
		ExtractProgram,
		/**
		 * QStringList
		 * The arguments that are passed to the program above for
		 * extracting the archive. Special strings that will be
		 * substituted:
		 * $Archive - the path of the archive
		 * $Files - the files selected to be extracted, if any
		 * $PreservePathSwitch - the flag for extracting with full paths
		 * $RootNodeSwitch - the internal work dir in the archive (for example
		 * when the user has dragged a folder from the archive and wants it
		 * extracted relative to it)
		 */
		ExtractArgs,
		/**
		 * Bool (default false)
		 * When passing directories to the extract program, do not
		 * include trailing slashes
		 * e.g. if the user selected "foo/" and "foo/bar" in the gui, the
		 * paths "foo" and "foo/bar" will be sent to the program.
		 */
		NoTrailingSlashes,
		/**
		 * QStringList
		 * This should be a qstringlist with either two elements. The first
		 * string is what PreservePathSwitch in the ExtractArgs will be replaced
		 * with if PreservePath is True/enabled. The second is for the disabled
		 * case. An empty string means that the argument will not be used in
		 * that case.
		 * Example: for rar, "x" means extract with full paths, and "e" means
		 * extract without full paths. in this case we will use the stringlist
		 * ("x", "e"). Or, for another format that might use the switch
		 * "--extractFull" for preservePaths, and nothing otherwise: we use the
		 * stringlist ("--preservePaths", "")
		 */
		PreservePathSwitch,
		/**
		 * QStringList (default empty)
		 * The format of the root node switch. The variable $Path will be
		 * substituted for the path string.
		 * Example: ("--internalPath=$Path)
		 * or ("--path", "$Path")
		 */
		RootNodeSwitch,
		


		///////////////[ DELETE ]/////////////

		/**
		 * QString
		 * The name to the program that will handle deleting of elements in this
		 * archive format (eg "rar"). Will be searched for in PATH
		 */
		DeleteProgram,
		/**
		 * QStringList
		 * The arguments that are passed to the program above for
		 * deleting from the archive. Special strings that will be
		 * substituted:
		 * $Archive - the path of the archive
		 * $Files - the files selected to be deleted
		 */
		DeleteArgs,
		/**
		 * QString
		 * The name to the program that will handle adding in this
		 * archive format (eg "rar"). Will be searched for in PATH
		 */

		///////////////[ ADD ]/////////////

		AddProgram,
		/**
		 * QStringList
		 * The arguments that are passed to the program above for
		 * adding to the archive. Special strings that will be
		 * substituted:
		 * $Archive - the path of the archive
		 * $Files - the files selected to be added
		 */
		AddArgs
	};

	typedef QHash<int, QVariant> ParameterList;

	class KERFUFFLE_EXPORT CliInterface : public ReadWriteArchiveInterface
	{
		Q_OBJECT

		public:

			enum OperationMode  {
				List, Copy, Add, Delete
			};
			OperationMode m_mode;

			explicit CliInterface( const QString& filename, QObject *parent = 0);
			virtual ~CliInterface();

			virtual bool list();
			virtual bool copyFiles( const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options );
			virtual bool addFiles( const QStringList & files, const CompressionOptions& options );
			virtual bool deleteFiles( const QList<QVariant> & files );

			virtual ParameterList parameterList() const = 0;
			virtual bool readListLine(QString line) = 0;

		private:
			bool findProgramInPath(const QString& program);
			void substituteCopyVariables(QStringList& params, const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options);
			void substituteListVariables(QStringList& params);

			bool createProcess();
			bool executeProcess(const QString& path, const QStringList & args);
			void cacheParameterList();

			QStringList m_errorMessages;
			QByteArray m_stdOutData;
			QList<QVariant> m_archiveContents;
			bool m_userCancelled;

			KPtyProcess *m_process;
			QString m_program;
			QEventLoop *m_loop;
			ParameterList m_param;

		private slots:
			void started();
			void readStdout();
			void readFromStderr();
			void finished( int exitCode, QProcess::ExitStatus exitStatus );

	};

}

#endif /* _CLIINTERFACE_H_ */
