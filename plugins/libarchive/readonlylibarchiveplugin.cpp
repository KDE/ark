/*
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>
    SPDX-FileCopyrightText: 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2010 Raphael Kubo da Costa <rakuco@FreeBSD.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "readonlylibarchiveplugin.h"
#include "ark_debug.h"

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(ReadOnlyLibarchivePlugin, "kerfuffle_libarchive_readonly.json")

ReadOnlyLibarchivePlugin::ReadOnlyLibarchivePlugin(QObject *parent, const QVariantList &args)
    : LibarchivePlugin(parent, args)
{
    qCDebug(ARK) << "Loaded libarchive read-only plugin";
}

ReadOnlyLibarchivePlugin::~ReadOnlyLibarchivePlugin()
{
}

#include "moc_readonlylibarchiveplugin.cpp"
#include "readonlylibarchiveplugin.moc"
