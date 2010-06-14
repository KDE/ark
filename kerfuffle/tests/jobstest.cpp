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

#include <kdebug.h>
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
    void testExtractedFilesSize();
    void testIsPasswordProtected();
    void testIsSingleFolderArchive();
    void testListEntries();

private:
    JSONArchiveInterface *createArchiveInterface(const QString& filePath);

    QList<Kerfuffle::ArchiveEntry> m_entries;
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

JSONArchiveInterface *JobsTest::createArchiveInterface(const QString& filePath)
{
    QVariantList args;
    args.append(filePath);

    JSONArchiveInterface *iface = new JSONArchiveInterface(this, args);
    if (!iface->open()) {
        kDebug() << "Could not open" << filePath;
        return NULL;
    }

    return iface;
}

void JobsTest::testExtractedFilesSize()
{
    Kerfuffle::ListJob *listJob;

    JSONArchiveInterface *noSizeIface =
        createArchiveInterface(KDESRCDIR "data/archive001.json");
    JSONArchiveInterface *sizeIface =
        createArchiveInterface(KDESRCDIR "data/archive002.json");

    listJob = new Kerfuffle::ListJob(noSizeIface, this);
    listJob->exec();

    QCOMPARE(listJob->extractedFilesSize(), 0LL);

    listJob = new Kerfuffle::ListJob(sizeIface, this);
    listJob->exec();

    QCOMPARE(listJob->extractedFilesSize(), 45959LL);

    noSizeIface->deleteLater();
    sizeIface->deleteLater();
}

void JobsTest::testIsPasswordProtected()
{
    Kerfuffle::ListJob *listJob;

    JSONArchiveInterface *noPasswordIface =
        createArchiveInterface(KDESRCDIR "data/archive002.json");
    JSONArchiveInterface *passwordIface =
        createArchiveInterface(KDESRCDIR "data/archive-password.json");

    listJob = new Kerfuffle::ListJob(noPasswordIface, this);
    listJob->exec();

    QCOMPARE(listJob->isPasswordProtected(), false);

    listJob = new Kerfuffle::ListJob(passwordIface, this);
    listJob->exec();

    QCOMPARE(listJob->isPasswordProtected(), true);

    noPasswordIface->deleteLater();
    passwordIface->deleteLater();
}

void JobsTest::testIsSingleFolderArchive()
{
    JSONArchiveInterface *iface =
        createArchiveInterface(KDESRCDIR "data/archive001.json");

    Kerfuffle::ListJob *listJob = new Kerfuffle::ListJob(iface, this);
    listJob->exec();
    QCOMPARE(listJob->isSingleFolderArchive(), false);
    iface->deleteLater();
    listJob->deleteLater();

    iface = createArchiveInterface(KDESRCDIR "data/archive-singlefile.json");
    listJob = new Kerfuffle::ListJob(iface, this);
    listJob->exec();
    QCOMPARE(listJob->isSingleFolderArchive(), true);
    iface->deleteLater();
    listJob->deleteLater();

    iface = createArchiveInterface(KDESRCDIR "data/archive-onetopfolder.json");
    listJob = new Kerfuffle::ListJob(iface, this);
    listJob->exec();
    QCOMPARE(listJob->isSingleFolderArchive(), true);
    iface->deleteLater();
    listJob->deleteLater();

    iface = createArchiveInterface
            (KDESRCDIR "data/archive-multiplefolders.json");
    listJob = new Kerfuffle::ListJob(iface, this);
    listJob->exec();
    QCOMPARE(listJob->isSingleFolderArchive(), false);
    iface->deleteLater();
    listJob->deleteLater();

    iface = createArchiveInterface
            (KDESRCDIR "data/archive-nodir-manyfiles.json");
    listJob = new Kerfuffle::ListJob(iface, this);
    listJob->exec();
    QCOMPARE(listJob->isSingleFolderArchive(), false);
    iface->deleteLater();
    listJob->deleteLater();

    iface = createArchiveInterface
            (KDESRCDIR "data/archive-deepsinglehierarchy.json");
    listJob = new Kerfuffle::ListJob(iface, this);
    listJob->exec();
    QCOMPARE(listJob->isSingleFolderArchive(), true);
    iface->deleteLater();
    listJob->deleteLater();

    iface = createArchiveInterface
            (KDESRCDIR "data/archive-unorderedsinglefolder.json");
    listJob = new Kerfuffle::ListJob(iface, this);
    listJob->exec();
    QCOMPARE(listJob->isSingleFolderArchive(), true);
    iface->deleteLater();
    listJob->deleteLater();
}

void JobsTest::testListEntries()
{
    JSONArchiveInterface *iface =
        createArchiveInterface(KDESRCDIR "data/archive001.json");

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

    QCOMPARE(entries.count(), m_entries.count());

    for (int i = 0; i < entries.count(); ++i) {
        Kerfuffle::ArchiveEntry e(m_entries.at(i));

        QCOMPARE(entries[i], e[Kerfuffle::FileName].toString());
    }

    iface->deleteLater();
}

void JobsTest::slotNewEntry(const ArchiveEntry& entry)
{
    m_entries.append(entry);
}

#include "jobstest.moc"
