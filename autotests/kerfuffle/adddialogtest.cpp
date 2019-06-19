/*
 * Copyright (c) 2016 Ragnar Thomsen <rthomsen6@gmail.com>
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

#include "adddialog.h"
#include "archiveformat.h"
#include "pluginmanager.h"

#include <KCollapsibleGroupBox>

#include <QCheckBox>
#include <QMimeDatabase>
#include <QTest>

using namespace Kerfuffle;

class AddDialogTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testBasicWidgets_data();
    void testBasicWidgets();

private:
    PluginManager m_pluginManager;
};

void AddDialogTest::initTestCase()
{
    // Avoid a runtime dependency on KLauncher.
    qputenv("KDE_FORK_SLAVES", "yes");
}

void AddDialogTest::testBasicWidgets_data()
{
    QTest::addColumn<QString>("mimeType");
    QTest::addColumn<bool>("supportsCompLevel");
    QTest::addColumn<int>("initialCompLevel");
    QTest::addColumn<int>("changeToCompLevel");

    QTest::newRow("tar") << QStringLiteral("application/x-tar") << false << -1 << -1;
    QTest::newRow("targzip") << QStringLiteral("application/x-compressed-tar") << true << 3 << 7;
    QTest::newRow("tarbzip") << QStringLiteral("application/x-bzip-compressed-tar") << true << 3 << 7;
    QTest::newRow("tarZ") << QStringLiteral("application/x-tarz") << false << -1 << -1;
    QTest::newRow("tarxz") << QStringLiteral("application/x-xz-compressed-tar") << true << 3 << 7;
    QTest::newRow("tarlzma") << QStringLiteral("application/x-lzma-compressed-tar") << true << 3 << 7;
    QTest::newRow("tarlzip") << QStringLiteral("application/x-lzip-compressed-tar") << true << 3 << 7;

    const auto writeMimeTypes = m_pluginManager.supportedWriteMimeTypes();

    if (writeMimeTypes.contains(QStringLiteral("application/zip"))) {
        QTest::newRow("zip") << QStringLiteral("application/zip") << true << 3 << 7;
    } else {
        qDebug() << "zip format not available, skipping test.";
    }

    if (writeMimeTypes.contains(QStringLiteral("application/x-7z-compressed"))) {
        QTest::newRow("7z") << QStringLiteral("application/x-7z-compressed") << true << 3 << 7;
    } else {
        qDebug() << "7z format not available, skipping test.";
    }

    if (writeMimeTypes.contains(QStringLiteral("application/vnd.rar"))) {
        QTest::newRow("rar") << QStringLiteral("application/vnd.rar") << true << 2 << 5;
    } else {
        qDebug() << "rar format not available, skipping test.";
    }

    if (writeMimeTypes.contains(QStringLiteral("application/x-lrzip-compressed-tar"))) {
        QTest::newRow("tarlrzip") << QStringLiteral("application/x-lrzip-compressed-tar") << true << 3 << 7;
    } else {
        qDebug() << "tar.lrzip format not available, skipping test.";
    }

    if (writeMimeTypes.contains(QStringLiteral("application/x-tzo"))) {
        QTest::newRow("tarlzop") << QStringLiteral("application/x-tzo") << true << 3 << 7;
    } else {
        qDebug() << "tar.lzo format not available, skipping test.";
    }
}

void AddDialogTest::testBasicWidgets()
{
    QFETCH(QString, mimeType);
    const QMimeType mime = QMimeDatabase().mimeTypeForName(mimeType);
    QFETCH(bool, supportsCompLevel);
    QFETCH(int, initialCompLevel);
    QFETCH(int, changeToCompLevel);

    AddDialog *dialog = new AddDialog(nullptr, QString(), QUrl(), mime);

    dialog->slotOpenOptions();

    auto collapsibleCompression = dialog->optionsDialog->findChild<KCollapsibleGroupBox*>(QStringLiteral("collapsibleCompression"));
    QVERIFY(collapsibleCompression);

    const KPluginMetaData metadata = PluginManager().preferredPluginFor(mime)->metaData();
    const ArchiveFormat archiveFormat = ArchiveFormat::fromMetadata(mime, metadata);
    QVERIFY(archiveFormat.isValid());

    if (archiveFormat.defaultCompressionLevel() > 0 && supportsCompLevel) {
        // Test that collapsiblegroupbox is enabled for mimetypes that support compression levels.
        QVERIFY(collapsibleCompression->isEnabled());

        auto compLevelSlider = dialog->optionsDialog->findChild<QSlider*>(QStringLiteral("compLevelSlider"));
        QVERIFY(compLevelSlider);

        // Test that min/max of slider are correct.
        QCOMPARE(compLevelSlider->minimum(), archiveFormat.minCompressionLevel());
        QCOMPARE(compLevelSlider->maximum(), archiveFormat.maxCompressionLevel());

        // Test that the slider is set to default compression level.
        QCOMPARE(compLevelSlider->value(), archiveFormat.defaultCompressionLevel());

        // Set the compression level slider.
        compLevelSlider->setValue(changeToCompLevel);
    } else {
        // Test that collapsiblegroupbox is disabled for mimetypes that don't support compression levels.
        QVERIFY(!collapsibleCompression->isEnabled());
    }

    dialog->optionsDialog->accept();
    dialog->accept();

    if (archiveFormat.defaultCompressionLevel() > 0 && supportsCompLevel) {
        // Test that the value set by slider is exported from AddDialog.
        QCOMPARE(dialog->compressionOptions().compressionLevel(), changeToCompLevel);
    }

    // Test that passing a compression level in ctor works.
    CompressionOptions opts;
    opts.setCompressionLevel(initialCompLevel);

    dialog = new AddDialog(nullptr, QString(), QUrl(), mime, opts);
    dialog->slotOpenOptions();

    if (archiveFormat.defaultCompressionLevel() > 0 && supportsCompLevel) {

        auto compLevelSlider = dialog->optionsDialog->findChild<QSlider*>(QStringLiteral("compLevelSlider"));
        QVERIFY(compLevelSlider);

        // Test that slider is set to the compression level supplied in ctor.
        QCOMPARE(compLevelSlider->value(), initialCompLevel);
    }
    dialog->optionsDialog->accept();
    dialog->accept();
}

QTEST_MAIN(AddDialogTest)

#include "adddialogtest.moc"
