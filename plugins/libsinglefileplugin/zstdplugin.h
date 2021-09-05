/*
    SPDX-FileCopyrightText: 2021 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef ZSTDPLUGIN_H
#define ZSTDPLUGIN_H

#include "singlefileplugin.h"

class KERFUFFLE_EXPORT LibZstdInterface : public LibSingleFileInterface
{
    Q_OBJECT

public:
    LibZstdInterface(QObject *parent, const QVariantList &args);
    ~LibZstdInterface() override;
};

#endif // ZSTDPLUGIN_H
