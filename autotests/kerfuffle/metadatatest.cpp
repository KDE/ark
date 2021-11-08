/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "kcoreaddons_version.h"
#include <KPluginMetaData>
#include <QTest>

class MetaDataTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void initTestCase();
    void testPluginLoading();
    void testPluginMetadata();

private:

    QVector<KPluginMetaData> m_plugins;
};

void MetaDataTest::initTestCase()
{
    m_plugins = KPluginMetaData::findPlugins(QStringLiteral("kerfuffle"));
}

// If a plugin has invalid JSON metadata (e.g. an extra comma somewhere)
// it won't occur in the list of available plugins.
void MetaDataTest::testPluginLoading()
{
    QCOMPARE(m_plugins.count() % PLUGINS_COUNT, 0);
}

void MetaDataTest::testPluginMetadata()
{
    for (const KPluginMetaData& metaData : std::as_const(m_plugins)) {
        QVERIFY(!metaData.mimeTypes().isEmpty());

        const QJsonObject json = metaData.rawData();
        QVERIFY(json.keys().contains(QLatin1String("X-KDE-Priority")));
        QVERIFY(json.keys().contains(QLatin1String("KPlugin")));

        if (json.keys().contains(QLatin1String("X-KDE-Kerfuffle-ReadOnlyExecutables"))) {
            QVERIFY(json[QStringLiteral("X-KDE-Kerfuffle-ReadOnlyExecutables")].isArray());
        }

        if (json.keys().contains(QLatin1String("X-KDE-Kerfuffle-ReadWriteExecutables"))) {
            QVERIFY(json[QStringLiteral("X-KDE-Kerfuffle-ReadWriteExecutables")].isArray());

            // If there is a list of read-write executables, the plugin has to be read-write.
            QVERIFY(json.keys().contains(QLatin1String("X-KDE-Kerfuffle-ReadWrite")));
            QVERIFY(json[QStringLiteral("X-KDE-Kerfuffle-ReadWrite")].toBool());
        }
    }
}

QTEST_GUILESS_MAIN(MetaDataTest)

#include "metadatatest.moc"
