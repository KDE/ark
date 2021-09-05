/*
    SPDX-FileCopyrightText: 2009 Raphael Kubo da Costa <rakuco@FreeBSD.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef XZPLUGIN_H
#define XZPLUGIN_H

#include "singlefileplugin.h"

class KERFUFFLE_EXPORT LibXzInterface : public LibSingleFileInterface
{
    Q_OBJECT

public:
    LibXzInterface(QObject *parent, const QVariantList & args);
    ~LibXzInterface() override;
};

#endif // XZPLUGIN_H
