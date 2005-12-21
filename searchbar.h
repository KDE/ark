#ifndef SEARCHBAR_H
#define SEARCHBAR_H
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <klistviewsearchline.h>

#include <qwidget.h>

class KListView;
class KComboBox;
class KActionCollection;

class SearchBar: public KListViewSearchLine
{
	Q_OBJECT
	public:
		SearchBar( QWidget* parent, KActionCollection* aC, const char * name = 0 );
		~SearchBar();
};

#endif // SEARCHBAR_H
