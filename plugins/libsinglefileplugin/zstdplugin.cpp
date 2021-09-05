/*
    SPDX-FileCopyrightText: 2021 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "zstdplugin.h"
#include "kerfuffle_export.h"

#include <QString>

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(LibZstdInterface, "kerfuffle_libzstd.json")

LibZstdInterface::LibZstdInterface(QObject *parent, const QVariantList & args)
        : LibSingleFileInterface(parent, args)
{
    m_mimeType = QStringLiteral( "application/zstd" );
    m_possibleExtensions.append(QStringLiteral( ".zst" ));
}

LibZstdInterface::~LibZstdInterface()
{
}

#include "zstdplugin.moc"
