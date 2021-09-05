/*
    SPDX-FileCopyrightText: 2009 Raphael Kubo da Costa <rakuco@FreeBSD.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef GZPLUGIN_H
#define GZPLUGIN_H

#include "singlefileplugin.h"

class KERFUFFLE_EXPORT LibGzipInterface : public LibSingleFileInterface
{
    Q_OBJECT

public:
    LibGzipInterface(QObject *parent, const QVariantList & args);
    ~LibGzipInterface() override;
};

#endif // GZPLUGIN_H
