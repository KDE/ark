/*
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef TESTHELPER_H
#define TESTHELPER_H

class KJob;

#include <QStringList>

namespace TestHelper
{
void startAndWaitForResult(KJob *job);

/**
 * @return List of format extensions (without the leading dot) to be used in tests.
 */
QStringList testFormats();
}

#endif // TESTHELPER_H
