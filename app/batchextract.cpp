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

#include "batchextract.h"
#include <KPluginLoader>
#include <KPluginFactory>
#include <KMessageBox>
#include <KLocale>
#include "part/interface.h"

BatchExtract::BatchExtract(QWidget *parent)
	: KDialog(parent),
	m_part(NULL),
	arkInterface(NULL)
{

}

bool BatchExtract::loadPart()
{
	KPluginFactory *factory = KPluginLoader("libarkpart").factory();
	if(factory) {
		m_part = static_cast<KParts::ReadWritePart*>( factory->create<KParts::ReadWritePart>(NULL) );
	}
	if ( !factory || !m_part )
	{
		KMessageBox::error( this, i18n( "Unable to find Ark's KPart component, please check your installation." ) );
		return false;
	}
	m_part->setObjectName( "ArkPart" );

	arkInterface = qobject_cast<Interface*>(m_part);

	if (!arkInterface)
	{
		KMessageBox::error( this, i18n( "Unable to find Ark's KPart component, please check your installation." ) );
		return false;
	}

	return true;
}

BatchExtract::~BatchExtract()
{
	delete m_part;
	m_part = 0;
}
