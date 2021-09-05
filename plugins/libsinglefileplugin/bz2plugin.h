/*
    SPDX-FileCopyrightText: 2009 Raphael Kubo da Costa <rakuco@FreeBSD.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef BZ2PLUGIN_H
#define BZ2PLUGIN_H

#include "singlefileplugin.h"

class KERFUFFLE_EXPORT LibBzip2Interface : public LibSingleFileInterface
{
    Q_OBJECT

public:
    LibBzip2Interface(QObject *parent, const QVariantList & args);
    ~LibBzip2Interface() override;
};

#endif // BZ2PLUGIN_H
