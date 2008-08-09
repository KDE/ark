/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv (at@at) stud.ntnu.no>
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

#include "queries.h"
#include <QMessageBox>

#include <KLocale>

#include <kio/renamedialog.h>

namespace Kerfuffle
{

	Query::Query()
	{
		m_responseMutex.lock();
	}

	QVariant Query::response()
	{
		return m_data.value("response");
	}

	void Query::waitForResponse()
	{
		m_responseMutex.lock();
		m_responseMutex.unlock();
	}

	void Query::setResponse(QVariant response)
	{
		m_data["response"] = response;
		m_responseMutex.unlock();
	}

	//---

	OverwriteQuery::OverwriteQuery( QString filename)
	{
		m_data["filename"] = filename;
	}

	void OverwriteQuery::execute()
	{
		KIO::RenameDialog dialog(
				NULL,
				i18n("File already exists"), 
				KUrl(m_data.value("filename").toString()),
				KUrl(m_data.value("filename").toString()),
				(KIO::RenameDialog_Mode)(KIO::M_OVERWRITE | KIO::M_MULTI));
		dialog.exec();

		setResponse(dialog.result());
	}

	bool OverwriteQuery::responseCancelled()
	{
		return m_data.value("response").toInt() == KIO::R_CANCEL;
	}
	bool OverwriteQuery::responseOverwriteAll()
	{
		return m_data.value("response").toInt() == KIO::R_OVERWRITE_ALL;
	}
	bool OverwriteQuery::responseOverwrite()
	{
		return m_data.value("response").toInt() == KIO::R_OVERWRITE;
	}


}
