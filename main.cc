/*
    ark: A program for modifying archives via a GUI.

    Copyright (C)
    1997-1999 Robert Palmbos <palm9744@kettering.edu>
    1999 Francois-Xavier Duranceau <duranceau@kde.org>
    1999-2000: Emily Ezust  emilye@corel.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

// Note: This is a KUniqueApplication.
// To debug add --nofork to the command line.
// Be aware that newInstance() will not be called in this case.

#include <sys/param.h>

#include <klocale.h>
#include <kdebug.h>
#include <dcopclient.h>
#include <qmessagebox.h>
#include <kcmdlineargs.h>
#include "arkapp.h"
#include "arkwidget.h"

static const char *description = 
	I18N_NOOP("KDE Archiving tool");

static const char *version = "v0.0.1";

static KCmdLineOptions option[] =
{
   { "+[archive]", I18N_NOOP("Open 'archive'"), 0 },
   { 0, 0, 0 }
};

int main( int argc, char *argv[]  )
{
    KCmdLineArgs::init(argc, argv, "ark", description, version );
    KCmdLineArgs::addCmdLineOptions( option );


    if (!ArkApplication::start())
    {
       // Already running! 
       exit(0);
    }

    if (ArkApplication::getInstance()->isRestored())
    {
       kdebug(0, 1601, "In main: Restore...");
       RESTORE(ArkWidget);
    }
    kdebug(0, 1601, "Ready to exec...");
    return ArkApplication::getInstance()->exec();
}
