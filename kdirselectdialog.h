/*
	Copyright (C) 2001 Michael Jarrett <michaelj@corel.com>

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License version 2 as published by the Free Software Foundation.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.

	You should have received a copy of the GNU Library General Public License
	along with this library; see the file COPYING.LIB.  If not, write to
	the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*/

#ifndef KDIRSELECTDIALOG_H
#define KDIRSELECTDIALOG_H

class QWidget;
class QPushButton;
class QHBoxLayout;
class QVBoxLayout;
class KDialog;
class KURL;

class KDirSelect;

/**
* A pretty dialog for a KDirSelect control for selecting directories.
* @author Michael Jarrett <michaelj@corel.com>
* @see KFileDialog
*/
class KDirSelectDialog : public KDialog
{
	Q_OBJECT
public:
	KDirSelectDialog(KURL &rootURL, QWidget *parent = 0, const char *name = 0);
	~KDirSelectDialog();

	KURL getURL();

	static KURL selectDirectory(KURL root, QWidget *parent);

protected:
	// Layouts protected so that subclassing is easy
	QHBoxLayout *m_buttonLayout;
	QVBoxLayout *m_mainLayout;

private:
	KDirSelect *m_dirList;
	KURL m_currentURL;
};

#endif
