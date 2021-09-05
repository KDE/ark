/*
    SPDX-FileCopyrightText: 2009 Raphael Kubo da Costa <rakuco@FreeBSD.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "gzplugin.h"
#include "kerfuffle_export.h"

#include <QString>

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(LibGzipInterface, "kerfuffle_libgz.json")

LibGzipInterface::LibGzipInterface(QObject *parent, const QVariantList & args)
        : LibSingleFileInterface(parent, args)
{
    m_mimeType = QStringLiteral( "application/x-gzip" );
    m_possibleExtensions.append(QStringLiteral( ".gz" ));
}

LibGzipInterface::~LibGzipInterface()
{
}

#include "gzplugin.moc"
