/*

 ark -- archiver for the KDE project

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
#ifndef OBSERVER_H
#define OBSERVER_H

#include "archive.h"

namespace Kerfuffle
{

	class ArchiveObserver
	{
		public:
			ArchiveObserver() {}
			virtual ~ArchiveObserver() {}

			virtual void onError( const QString & message, const QString & details ) = 0;
			virtual void onEntry( const ArchiveEntry & archiveEntry ) = 0;
			virtual void onProgress( double ) = 0;
			virtual void onEntryRemoved( const QString & path ) = 0;
	};

} // namespace Kerfuffle

#endif // OBSERVER_H
