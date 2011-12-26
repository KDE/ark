/*
 * Copyright (c) 2010-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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

#include <qeventloop.h>
#include <qsignalspy.h>

using Kerfuffle::ArchiveEntry;

class JobsTest : public QObject
{
    Q_OBJECT

public:
    JobsTest();

protected Q_SLOTS:
    void init();

    void slotNewEntry(const ArchiveEntry& entry);

private Q_SLOTS:
    // ListJob-related tests
    void testExtractedFilesSize();
    void testIsPasswordProtected();
    void testIsSingleFolderArchive();
    void testListEntries();

    // ExtractJob-related tests
    void testExtractJobAccessors();

    // DeleteJob-related tests
    void testRemoveEntry();

    // AddJob-related tests
    void testAddEntry();

private:
    JSONArchiveInterface *createArchiveInterface(const QString& filePath);
    QList<Kerfuffle::ArchiveEntry> listEntries(JSONArchiveInterface *iface);
    void startAndWaitForResult(KJob *job);

    QList<Kerfuffle::ArchiveEntry> m_entries;
    QEventLoop m_eventLoop;
};

QTEST_KDEMAIN_CORE(JobsTest)

JobsTest::JobsTest()
    : QObject(0)
    , m_eventLoop(this)
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

void JobsTest::startAndWaitForResult(KJob *job)
{
    connect(job, SIGNAL(result(KJob*)), &m_eventLoop, SLOT(quit()));
    job->start();
    m_eventLoop.exec();
}

void JobsTest::testExtractedFilesSize()
{
    Kerfuffle::ListJob *listJob;

    JSONArchiveInterface *noSizeIface =
        createArchiveInterface(QLatin1String(KDESRCDIR "data/archive001.json"));
    JSONArchiveInterface *sizeIface =
        createArchiveInterface(QLatin1String(KDESRCDIR "data/archive002.json"));

    listJob = new Kerfuffle::ListJob(noSizeIface, this);
    listJob->setAutoDelete(false);
    startAndWaitForResult(listJob);

    QCOMPARE(listJob->extractedFilesSize(), 0LL);

    listJob = new Kerfuffle::ListJob(sizeIface, this);
    listJob->setAutoDelete(false);
    startAndWaitForResult(listJob);

    QCOMPARE(listJob->extractedFilesSize(), 45959LL);

    noSizeIface->deleteLater();
    sizeIface->deleteLater();
}

void JobsTest::testIsPasswordProtected()
{
    Kerfuffle::ListJob *listJob;

    JSONArchiveInterface *noPasswordIface =
        createArchiveInterface(QLatin1String(KDESRCDIR "data/archive002.json"));
    JSONArchiveInterface *passwordIface =
        createArchiveInterface(QLatin1String(KDESRCDIR "data/archive-password.json"));

    listJob = new Kerfuffle::ListJob(noPasswordIface, this);
    listJob->setAutoDelete(false);
    startAndWaitForResult(listJob);

    QVERIFY(!listJob->isPasswordProtected());

    listJob = new Kerfuffle::ListJob(passwordIface, this);
    listJob->setAutoDelete(false);
    startAndWaitForResult(listJob);

    QVERIFY(listJob->isPasswordProtected());

    noPasswordIface->deleteLater();
    passwordIface->deleteLater();
}

void JobsTest::testIsSingleFolderArchive()
{
    JSONArchiveInterface *iface =
        createArchiveInterface(QLatin1String(KDESRCDIR "data/archive001.json"));

    Kerfuffle::ListJob *listJob = new Kerfuffle::ListJob(iface, this);
    listJob->setAutoDelete(false);
    startAndWaitForResult(listJob);
    QVERIFY(!listJob->isSingleFolderArchive());
    QCOMPARE(listJob->subfolderName(), QString());
    iface->deleteLater();

    iface = createArchiveInterface(QLatin1String(KDESRCDIR "data/archive-singlefile.json"));
    listJob = new Kerfuffle::ListJob(iface, this);
    listJob->setAutoDelete(false);
    startAndWaitForResult(listJob);
    QVERIFY(listJob->isSingleFolderArchive());
    QCOMPARE(listJob->subfolderName(), QLatin1String("a.txt"));
    iface->deleteLater();

    iface = createArchiveInterface(QLatin1String(KDESRCDIR "data/archive-onetopfolder.json"));
    listJob = new Kerfuffle::ListJob(iface, this);
    listJob->setAutoDelete(false);
    startAndWaitForResult(listJob);
    QVERIFY(listJob->isSingleFolderArchive());
    QCOMPARE(listJob->subfolderName(), QLatin1String("aDir"));
    iface->deleteLater();

    iface = createArchiveInterface
            (QLatin1String(KDESRCDIR "data/archive-multiplefolders.json"));
    listJob = new Kerfuffle::ListJob(iface, this);
    listJob->setAutoDelete(false);
    startAndWaitForResult(listJob);
    QVERIFY(!listJob->isSingleFolderArchive());
    QCOMPARE(listJob->subfolderName(), QString());
    iface->deleteLater();

    iface = createArchiveInterface
            (QLatin1String(KDESRCDIR "data/archive-nodir-manyfiles.json"));
    listJob = new Kerfuffle::ListJob(iface, this);
    listJob->setAutoDelete(false);
    startAndWaitForResult(listJob);
    QVERIFY(!listJob->isSingleFolderArchive());
    QCOMPARE(listJob->subfolderName(), QString());
    iface->deleteLater();

    iface = createArchiveInterface
            (QLatin1String(KDESRCDIR "data/archive-deepsinglehierarchy.json"));
    listJob = new Kerfuffle::ListJob(iface, this);
    listJob->setAutoDelete(false);
    startAndWaitForResult(listJob);
    QVERIFY(listJob->isSingleFolderArchive());
    QCOMPARE(listJob->subfolderName(), QLatin1String("aDir"));
    iface->deleteLater();

    iface = createArchiveInterface
            (QLatin1String(KDESRCDIR "data/archive-unorderedsinglefolder.json"));
    listJob = new Kerfuffle::ListJob(iface, this);
    listJob->setAutoDelete(false);
    startAndWaitForResult(listJob);
    QVERIFY(listJob->isSingleFolderArchive());
    QCOMPARE(listJob->subfolderName(), QLatin1String("aDir"));
    iface->deleteLater();
}

void JobsTest::testListEntries()
{
    JSONArchiveInterface *iface =
        createArchiveInterface(QLatin1String(KDESRCDIR "data/archive001.json"));

    QList<Kerfuffle::ArchiveEntry> archiveEntries(listEntries(iface));

    QStringList entries;
    entries.append(QLatin1String("a.txt"));
    entries.append(QLatin1String("aDir/"));
    entries.append(QLatin1String("aDir/b.txt"));
    entries.append(QLatin1String("c.txt"));

    QCOMPARE(entries.count(), archiveEntries.count());

    for (int i = 0; i < entries.count(); ++i) {
        Kerfuffle::ArchiveEntry e(archiveEntries.at(i));

        QCOMPARE(entries[i], e[Kerfuffle::FileName].toString());
    }

    iface->deleteLater();
}

void JobsTest::slotNewEntry(const ArchiveEntry& entry)
{
    m_entries.append(entry);
}

QList<Kerfuffle::ArchiveEntry> JobsTest::listEntries(JSONArchiveInterface *iface)
{
    m_entries.clear();

    Kerfuffle::ListJob *listJob = new Kerfuffle::ListJob(iface, this);
    connect(listJob, SIGNAL(newEntry(ArchiveEntry)),
            SLOT(slotNewEntry(ArchiveEntry)));

    startAndWaitForResult(listJob);

    return m_entries;
}

void JobsTest::testExtractJobAccessors()
{
    JSONArchiveInterface *iface = createArchiveInterface(QLatin1String(KDESRCDIR "data/archive001.json"));
    Kerfuffle::ExtractJob *job =
        new Kerfuffle::ExtractJob(QVariantList(), QLatin1String("/tmp/some-dir"),
                                  Kerfuffle::ExtractionOptions(), iface, this);
    Kerfuffle::ExtractionOptions defaultOptions;
    defaultOptions[QLatin1String("PreservePaths")] = false;

    QCOMPARE(job->destinationDirectory(), QLatin1String("/tmp/some-dir"));
    QCOMPARE(job->extractionOptions(), defaultOptions);

    job->setAutoDelete(false);
    startAndWaitForResult(job);

    QCOMPARE(job->destinationDirectory(), QLatin1String("/tmp/some-dir"));
    QCOMPARE(job->extractionOptions(), defaultOptions);

    Kerfuffle::ExtractionOptions options;
    options[QLatin1String("PreservePaths")] = true;
    options[QLatin1String("foo")] = QLatin1String("bar");
    options[QLatin1String("pi")] = 3.14f;

    job = new Kerfuffle::ExtractJob(QVariantList(), QLatin1String("/root"),
                                    options, iface, this);

    QCOMPARE(job->destinationDirectory(), QLatin1String("/root"));
    QCOMPARE(job->extractionOptions(), options);

    job->setAutoDelete(false);
    startAndWaitForResult(job);

    QCOMPARE(job->destinationDirectory(), QLatin1String("/root"));
    QCOMPARE(job->extractionOptions(), options);
}

void JobsTest::testRemoveEntry()
{
    QVariantList filesToDelete;
    JSONArchiveInterface *iface;
    Kerfuffle::DeleteJob *deleteJob;
    QStringList expectedEntries;

    filesToDelete.append(QLatin1String("c.txt"));
    iface = createArchiveInterface(QLatin1String(KDESRCDIR "data/archive001.json"));
    deleteJob = new Kerfuffle::DeleteJob(filesToDelete, iface, this);
    startAndWaitForResult(deleteJob);
    QList<Kerfuffle::ArchiveEntry> archiveEntries(listEntries(iface));
    expectedEntries.append(QLatin1String("a.txt"));
    expectedEntries.append(QLatin1String("aDir/"));
    expectedEntries.append(QLatin1String("aDir/b.txt"));
    QCOMPARE(archiveEntries.count(), expectedEntries.count());
    for (int i = 0; i < expectedEntries.count(); ++i) {
        const Kerfuffle::ArchiveEntry e(archiveEntries.at(i));
        QCOMPARE(expectedEntries[i], e[Kerfuffle::FileName].toString());
    }
    iface->deleteLater();

    // TODO: test for errors
}

void JobsTest::testAddEntry()
{
    JSONArchiveInterface *iface = createArchiveInterface(QLatin1String(KDESRCDIR "data/archive001.json"));

    QList<Kerfuffle::ArchiveEntry> archiveEntries = listEntries(iface);
    QCOMPARE(archiveEntries.count(), 4);

    QStringList newEntries = QStringList() << QLatin1String("foo");

    Kerfuffle::AddJob *addJob =
        new Kerfuffle::AddJob(newEntries, Kerfuffle::CompressionOptions(), iface, this);
    startAndWaitForResult(addJob);

    archiveEntries = listEntries(iface);
    QCOMPARE(archiveEntries.count(), 5);

    addJob = new Kerfuffle::AddJob(newEntries, Kerfuffle::CompressionOptions(), iface, this);
    startAndWaitForResult(addJob);

    archiveEntries = listEntries(iface);
    QCOMPARE(archiveEntries.count(), 5);

    newEntries = QStringList() << QLatin1String("bar") << QLatin1String("aDir/test.txt");

    addJob = new Kerfuffle::AddJob(newEntries, Kerfuffle::CompressionOptions(), iface, this);
    startAndWaitForResult(addJob);

    archiveEntries = listEntries(iface);
    QCOMPARE(archiveEntries.count(), 7);

    iface->deleteLater();
}

#include "jobstest.moc"
