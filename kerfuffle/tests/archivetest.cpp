#include <qtest_kde.h>

#include "kerfuffle/archive.h"

class ArchiveTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testFileName();
    void testIsPasswordProtected();
    void testOpenNonExistentFile();
};

QTEST_KDEMAIN_CORE(ArchiveTest)

void ArchiveTest::testFileName()
{
    Kerfuffle::Archive *archive = Kerfuffle::factory("/tmp/foo.tar.gz");

    if (!archive)
        QSKIP("There is no plugin to handle tar.gz files. Skipping test.", SkipSingle);

    QCOMPARE(archive->fileName(), QString("/tmp/foo.tar.gz"));

    archive->deleteLater();
}

void ArchiveTest::testIsPasswordProtected()
{
    Kerfuffle::Archive *archive = Kerfuffle::factory(KDESRCDIR "data/archivetest_encrypted.zip");

    if (!archive)
        QSKIP("There is no plugin to handle zip files. Skipping test.", SkipSingle);

    QVERIFY(archive->isPasswordProtected());

    archive->deleteLater();
}

void ArchiveTest::testOpenNonExistentFile()
{
    QSKIP("How should we deal with files that do not exist? Should factory() return NULL?", SkipSingle);
}

#include "archivebasetest.moc"
