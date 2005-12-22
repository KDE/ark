/*
 ark -- archiver for the KDE project

 Copyright (C) 2003: Georg Robbers <georg.robbers@urz.uni-hd.de>

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

#ifndef ARKFACTORY_H
#define ARKFACTORY_H
#include <kparts/factory.h>

class ArkFactory : public KParts::Factory
{
public:
    ArkFactory() : KParts::Factory() {}
    virtual ~ArkFactory();
    virtual KParts::Part *createPartObject(
        QWidget *parentWidget = 0,const char *widgetName = 0,
        QObject *parent = 0, const char *name = 0,
        const char *classname = "KParts::Part",
        const QStringList &args = QStringList() );
    static KInstance* instance();
   private:
    static KInstance* s_instance;
    static KAboutData* s_about;
    static int instanceNumber;
};

#endif
