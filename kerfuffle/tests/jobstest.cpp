/*
 * Copyright (c) 2010 Raphael Kubo da Costa <kubito@gmail.com>
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

#include "kerfuffle/archive.h"
#include "kerfuffle/jobs.h"

#include <qtest_kde.h>

#include <qsignalspy.h>

class JobsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testEmitNewEntry();
};

QTEST_KDEMAIN_CORE(JobsTest)

void JobsTest::testEmitNewEntry()
{
    Kerfuffle::Archive *archive;

    archive = Kerfuffle::factory(KDESRCDIR "data/simplearchive.tar.gz");
    if (!archive)
        QSKIP("There is no plugin to handle tar.gz files. Skipping test.", SkipSingle);

    Kerfuffle::ListJob *listJob = archive->list();
    QSignalSpy spy(listJob, SIGNAL(newEntry(const ArchiveEntry&)));

    listJob->start();

    QCOMPARE(spy.count(), 4);

    archive->deleteLater();
}

#include "jobstest.moc"
