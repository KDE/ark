/*
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "testhelper.h"

#include <KJob>

#include <QEventLoop>

void TestHelper::startAndWaitForResult(KJob *job)
{
    QEventLoop eventLoop;
    QObject::connect(job, &KJob::result, &eventLoop, &QEventLoop::quit);
    job->start();
    eventLoop.exec(); // krazy:exclude=crashy
}


QStringList TestHelper::testFormats()
{
    return {
        QStringLiteral("7z"),
        QStringLiteral("rar"),
        QStringLiteral("tar.bz2"),
        QStringLiteral("zip")
    };
}
