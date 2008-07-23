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
#ifndef INTERFACE_H
#define INTERFACE_H

#include <QStringList>
#include <QtPlugin>
#include <kjobtrackerinterface.h>

class Interface
{
	public:
		virtual ~Interface() {}

		virtual QStringList supportedMimeTypes() const = 0;
		virtual QStringList supportedWriteMimeTypes() const = 0;

		virtual bool extract(QVariantMap arguments, KJobTrackerInterface *tracker = 0) = 0;
		virtual bool showExtractionDialog(QVariantMap& arguments) = 0;

		virtual bool isBusy() const = 0;
};

Q_DECLARE_INTERFACE( Interface, "org.kde.kerfuffle.partinterface/0.43" )

#endif // INTERFACE_H
