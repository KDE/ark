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

#include "kerfuffle/jobs.h"

#include "jsonarchiveinterface.h"

#include <kglobal.h>
#include <qtest_kde.h>

#include <qsignalspy.h>

using Kerfuffle::ArchiveEntry;

class JobsTest : public QObject
{
    Q_OBJECT

protected Q_SLOTS:
    void initTestCase();
    void init();

    void slotNewEntry(const ArchiveEntry& entry);

private Q_SLOTS:
    void testEmitNewEntry();

private:
    QStringList m_entries;
};

QTEST_KDEMAIN_CORE(JobsTest)

void JobsTest::initTestCase()
{
    // Hackish way to make sure the i18n stuff
    // is called from the main thread
    KGlobal::locale();

    qRegisterMetaType<ArchiveEntry>("ArchiveEntry");
}

void JobsTest::init()
{
    m_entries.clear();
}

void JobsTest::testEmitNewEntry()
{
    QVariantList args;
    args.append(KDESRCDIR "data/archive001.json");

    JSONArchiveInterface *iface = new JSONArchiveInterface(this, args);

    QCOMPARE(iface->filename(),
             QLatin1String(KDESRCDIR "data/archive001.json"));
    QVERIFY(iface->open());

    Kerfuffle::ListJob *listJob = new Kerfuffle::ListJob(iface, this);
    QSignalSpy spy(listJob, SIGNAL(newEntry(const ArchiveEntry&)));
    connect(listJob,  SIGNAL(newEntry(const ArchiveEntry&)),
            SLOT(slotNewEntry(const ArchiveEntry&)));

    listJob->exec();

    QCOMPARE(spy.count(), 4);

    QStringList entries;
    entries.append(QLatin1String("a.txt"));
    entries.append(QLatin1String("aDir/"));
    entries.append(QLatin1String("aDir/b.txt"));
    entries.append(QLatin1String("c.txt"));
    QCOMPARE(entries, m_entries);

    iface->deleteLater();
}

void JobsTest::slotNewEntry(const ArchiveEntry& entry)
{
    m_entries.append(entry[Kerfuffle::FileName].toString());
}

#include "jobstest.moc"
