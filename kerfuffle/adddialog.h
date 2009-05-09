/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv atatatat stud.ntnu.no>
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
#ifndef _ADDDIALOG_H_
#define _ADDDIALOG_H_

#include "kerfuffle_export.h"

#include <KFileDialog>

namespace Kerfuffle
{

	class KERFUFFLE_EXPORT AddDialog : public KFileDialog
	{
		public:
			AddDialog(const QStringList& itemsToAdd,
					const KUrl & 	startDir, 
					const QString & 	filter, 
					QWidget * 	parent, 
					QWidget * 	widget = 0	 
					);


		private:
			class AddDialogUI *m_ui;
			void setupIconList(const QStringList& itemsToAdd);

	};

}
#endif 
