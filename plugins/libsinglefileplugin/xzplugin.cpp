/*
    SPDX-FileCopyrightText: 2009 Raphael Kubo da Costa <rakuco@FreeBSD.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "xzplugin.h"
#include "kerfuffle_export.h"

#include <QString>

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(LibXzInterface, "kerfuffle_libxz.json")

LibXzInterface::LibXzInterface(QObject *parent, const QVariantList & args)
        : LibSingleFileInterface(parent, args)
{
    m_mimeType = QStringLiteral( "application/x-lzma" );
    m_possibleExtensions.append(QStringLiteral( ".lzma" ));
    m_possibleExtensions.append(QStringLiteral( ".xz" ));
}

LibXzInterface::~LibXzInterface()
{
}

#include "xzplugin.moc"
