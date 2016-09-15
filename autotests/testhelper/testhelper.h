/*
 * Copyright (c) 2016 Vladyslav Batyrenko <mvlabat@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TESTHELPER_H
#define TESTHELPER_H

#include "kerfuffle/jobs.h"
#include "kerfuffle/archiveentry.h"

#include <QTest>
#include <QEventLoop>
#include <QDirIterator>

using namespace Kerfuffle;

class TestHelper
{
public:

    static void startAndWaitForResult(KJob *job);
    static QList<Archive::Entry*> getEntryList(Archive *archive);
    static void verifyAddedEntriesWithDestination(const QList<Archive::Entry*> &argumentEntries, const Archive::Entry *destination, const QList<Archive::Entry*> &oldEntries, const QList<Archive::Entry*> &newEntries);
    static void verifyMovedEntriesWithDestination(const QList<Archive::Entry*> &argumentEntries, const Archive::Entry *destination, const QList<Archive::Entry*> &oldEntries, const QList<Archive::Entry*> &newEntries);
    static void verifyCopiedEntriesWithDestination(const QList<Archive::Entry*> &argumentEntries, const Archive::Entry *destination, const QList<Archive::Entry*> &oldEntries, const QList<Archive::Entry*> &newEntries);

private:
    TestHelper() {}

    static QStringList getExpectedNewEntryPaths(const QList<Archive::Entry*> &argumentEntries, const Archive::Entry *destination);
    static QStringList getExpectedMovedEntryPaths(const QList<Archive::Entry*> &entryList, const QList<Archive::Entry*> &argumentEntries, const Archive::Entry* destination);
    static QStringList getExpectedCopiedEntryPaths(const QList<Archive::Entry*> &entryList, const QList<Archive::Entry*> &argumentEntries, const Archive::Entry* destination);

    /**
     * Returns map of entries.
     *
     * It's useful when we need a sorted list of entries.
     */
    static QMap<QString, Archive::Entry*> getEntryMap(const QList<Archive::Entry*> &entries);

    static QEventLoop m_eventLoop;
};


#endif //TESTHELPER_H
