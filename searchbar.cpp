/*
 * ark: A program for modifying archives via a GUI.
 *
 * Copyright (C) 2004, Henrique Pinto <henrique.pinto@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "searchbar.h"

#include <klistviewsearchline.h>
#include <klistview.h>
#include <kcombobox.h>
#include <klocale.h>
#include <kaction.h>
#include <kactioncollection.h>

#include <qlabel.h>
#include <qapplication.h>
#include <qvaluelist.h>

SearchBar::SearchBar( QWidget* parent, KActionCollection* aC, const char * name )
	: KListViewSearchLine( parent, 0, name )
{
	KAction *resetSearch = new KAction( i18n( "Reset Search" ), QApplication::reverseLayout() ? "clear_left" : "locationbar_erase", 0, this, SLOT( slotResetSearch() ), aC, "reset_search" );

	resetSearch->plug( parent );
	resetSearch->setWhatsThis( i18n( "Reset Search\n"
	                                 "Resets the search bar, so that all archive entries are shown again." ) );
}

SearchBar::~SearchBar()
{
}

void SearchBar::slotResetSearch()
{
	clear();
}

#include "searchbar.moc"
