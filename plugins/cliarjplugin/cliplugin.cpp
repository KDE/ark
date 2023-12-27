/*
    SPDX-FileCopyrightText: 2022 Ilya Pominov <ipominov@astralinux.ru>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "cliplugin.h"
#include "ark_debug.h"

#include <QDateTime>
#include <QFileInfo>
#include <QStringView>

#include <KLocalizedString>
#include <KPluginFactory>

using namespace Kerfuffle;

K_PLUGIN_CLASS_WITH_JSON(CliPlugin, "kerfuffle_cliarj.json")

struct ArjFileEntry {
    enum EncryptedMethod {
        // https://sourceforge.net/p/arj/git/ci/master/tree/defines.h#l104
        EncryptedMethodArjOld = 0,
        EncryptedMethodArjStd = 1,
        EncryptedMethodGost256 = 2,
        EncryptedMethodGost256L = 3,
        EncryptedMethodGost40bit = 4,
        EncryptedMethodUnknown = 16
    };

    QString fileName() const
    {
        return QFileInfo(m_path).fileName();
    }

    bool isExecutable() const
    {
        return m_attributes.contains(QLatin1Char('x'));
    }

    int m_currentEntryNumber = 0;
    QString m_path;
    QStringList m_comments;
    bool m_commentsEnd = false;
    int m_version = 0;
    qulonglong m_origSize = 0;
    qulonglong m_compressedSize = 0;
    double m_ratio = 0.;
    QDateTime m_timeStamp;
    QString m_attributes;
    bool m_encrypted = false;
    EncryptedMethod m_encryptedMethod = EncryptedMethodUnknown;
};

CliPlugin::CliPlugin(QObject *parent, const QVariantList &args)
    : CliInterface(parent, args)
{
    qCDebug(ARK) << "Loaded cli_arj plugin";

    setupCliProperties();
}

CliPlugin::~CliPlugin() = default;

bool CliPlugin::addFiles(const QVector<Kerfuffle::Archive::Entry *> &files,
                         const Kerfuffle::Archive::Entry *destination,
                         const Kerfuffle::CompressionOptions &options,
                         uint numberOfEntriesToAdd)
{
    auto opt = options;
    if (opt.compressionMethod() == QStringLiteral("Standard")) {
        opt.setCompressionMethod(QString());
    } else if (opt.compressionMethod() == QStringLiteral("GOST 40-bit")) {
        opt.setCompressionMethod(QStringLiteral("!"));
    }

    return CliInterface::addFiles(files, destination, opt, numberOfEntriesToAdd);
}

bool CliPlugin::moveFiles(const QVector<Archive::Entry *> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    Q_UNUSED(options);

    m_operationMode = Move;

    QVector<Archive::Entry *> withoutChildren = entriesWithoutChildren(files);
    m_renamedFiles = files;
    setNewMovedFiles(files, destination, withoutChildren.count());

    QStringList args = cliProperties()->moveArgs(filename(), withoutChildren, nullptr, password());

    return runProcess(cliProperties()->property("moveProgram").toString(), args);
}

void CliPlugin::resetParsing()
{
    m_comment.clear();
    m_numberOfVolumes = 0;

    m_parseState = ParseStateTitle;
    m_remainingIgnoreLines = 0;
    m_headerComment.clear();
    m_currentParsedFile.reset(new ArjFileEntry());
    m_testPassed = true;
    m_renamedFiles.clear();
}

bool CliPlugin::readListLine(const QString &line)
{
    auto res = readLine(line);
    if (m_parseState == ParseStateEntryTotal && res) {
        m_comment = m_headerComment.join(QLatin1Char('\n'));
    }
    return res;
}

bool CliPlugin::readExtractLine(const QString &line)
{
    Q_UNUSED(line);

    return true;
}

bool CliPlugin::isFileExistsMsg(const QString &line)
{
    return line.contains(QStringLiteral("is same or newer, Overwrite?"));
}

bool CliPlugin::isFileExistsFileName(const QString &line)
{
    return line.contains(QStringLiteral("is same or newer, Overwrite?"));
}

bool CliPlugin::isNewMovedFileNamesMsg(const QString &line)
{
    return line.startsWith(QStringLiteral("Current filename:"));
}

bool CliPlugin::handleLine(const QString &line)
{
    if (line.contains(QStringLiteral("bad password"))) {
        qCWarning(ARK) << "Wrong password!";
        setPassword(QString());
        Q_EMIT error(i18nc("@info", "Extraction failed: Incorrect password"));
        return false;
    }

    if (m_operationMode == Test) {
        if (line.startsWith(QStringLiteral("Testing "))) {
            auto list = line.split(QStringLiteral("\b\b\b\b\b"));
            if (list.isEmpty() || !list.last().startsWith(QStringLiteral("OK"))) {
                m_testPassed = false;
            }
        }

        if (line.contains(QStringLiteral("file(s)")) && m_testPassed) {
            qCDebug(ARK) << "Test successful";
            Q_EMIT testSuccess();
        }

        return true;
    }

    return CliInterface::handleLine(line);
}

void CliPlugin::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_operationMode == Move && exitCode == 0 && exitStatus == QProcess::NormalExit) {
        const QStringList removedFullPaths = entryFullPaths(m_renamedFiles);
        for (const QString &fullPath : removedFullPaths) {
            Q_EMIT entryRemoved(fullPath);
        }
    }
    CliInterface::processFinished(exitCode, exitStatus);
}

void CliPlugin::setupCliProperties()
{
    qCDebug(ARK) << "Setting up parameters...";

    CliProperties *cliProps = cliProperties();
    cliProps->setProperty("captureProgress", true);

    cliProps->setProperty("addProgram", QStringLiteral("arj"));
    cliProps->setProperty("addSwitch", QStringList{QStringLiteral("a"), QStringLiteral("-r")});

    cliProps->setProperty("deleteProgram", QStringLiteral("arj"));
    cliProps->setProperty("deleteSwitch", QStringLiteral("d"));

    cliProps->setProperty("extractProgram", QStringLiteral("arj"));
    cliProps->setProperty("extractSwitch", QStringList{QStringLiteral("x"), QStringLiteral("-p1"), QStringLiteral("-jyc")});
    cliProps->setProperty("extractSwitchNoPreserve", QStringList{QStringLiteral("e")});

    cliProps->setProperty("listProgram", QStringLiteral("arj"));
    cliProps->setProperty("listSwitch", QStringLiteral("v"));

    cliProps->setProperty("moveProgram", QStringLiteral("arj"));
    cliProps->setProperty("moveSwitch", QStringLiteral("n"));

    cliProps->setProperty("testProgram", QStringLiteral("arj"));
    cliProps->setProperty("testSwitch", QStringLiteral("t"));

    cliProps->setProperty("passwordSwitch", QStringLiteral("-g$Password"));

    cliProps->setProperty("compressionMethodSwitch",
                          QHash<QString, QVariant>{{QStringLiteral("application/x-arj"), QStringLiteral("-m$CompressionMethod")},
                                                   {QStringLiteral("application/arj"), QStringLiteral("-m$CompressionMethod")}});
    cliProps->setProperty("encryptionMethodSwitch",
                          QHash<QString, QVariant>{{QStringLiteral("application/x-arj"), QStringLiteral("-hg$EncryptionMethod")},
                                                   {QStringLiteral("application/arj"), QStringLiteral("-hg$EncryptionMethod")}});
    cliProps->setProperty("multiVolumeSwitch", QStringLiteral("-v$VolumeSizek"));

    cliProps->setProperty("fileExistsFileNameRegExp", QStringList{QStringLiteral("^file \\./(.*)$"), QStringLiteral("^  Path:     \\./(.*)$")});

    cliProps->setProperty("commentSwitch", QStringList{QStringLiteral("c"), QStringLiteral("-z$CommentFile")});

    cliProps->setProperty("fileExistsInput",
                          QStringList{QStringLiteral("Yes"), // Overwrite
                                      QStringLiteral("No"), // Skip
                                      QStringLiteral("Always"), // Overwrite all
                                      QStringLiteral("Skip"), // Autoskip
                                      QStringLiteral("Quit")}); // Cancel
    cliProps->setProperty("multiVolumeSuffix", QStringList{QStringLiteral("$Suffix.001")});
}

void CliPlugin::ignoreLines(int lines, ParseState nextState)
{
    m_remainingIgnoreLines = lines;
    m_parseState = nextState;
}

bool CliPlugin::tryAddCurFileProperties(const QString &line)
{
    // 79 is fixed property line size
    if (line.size() != 79) {
        return false;
    }
    // Rev/Host OS    Original Compressed Ratio DateTime modified Attributes/GUA BPMGS
    // ------------ ---------- ---------- ----- ----------------- -------------- -----
    QStringList revHost = line.left(12).trimmed().split(QLatin1Char(' '));
    bool ok;
    m_currentParsedFile->m_version = revHost.first().toInt(&ok);
    if (!ok) {
        return false;
    }

    m_currentParsedFile->m_origSize = QStringView(line).mid(13, 10).toULongLong(&ok);
    if (!ok) {
        return false;
    }

    m_currentParsedFile->m_compressedSize = QStringView(line).mid(24, 10).toULongLong(&ok);
    if (!ok) {
        return false;
    }

    m_currentParsedFile->m_ratio = QStringView(line).mid(35, 5).toDouble(&ok);
    if (!ok) {
        return false;
    }

    m_currentParsedFile->m_timeStamp = QDateTime::fromString(line.mid(41, 17), QStringLiteral("yy-MM-dd hh:mm:ss"));
    if (!m_currentParsedFile->m_timeStamp.isValid()) {
        return false;
    }
    m_currentParsedFile->m_timeStamp = m_currentParsedFile->m_timeStamp.addYears(100);

    m_currentParsedFile->m_attributes = line.mid(59, 14);
    QChar garble = line.at(77);
    if (garble != QLatin1Char(' ')) {
        m_currentParsedFile->m_encrypted = true;
        m_currentParsedFile->m_encryptedMethod =
            garble.isDigit() ? static_cast<ArjFileEntry::EncryptedMethod>(garble.digitValue()) : ArjFileEntry::EncryptedMethodUnknown;
    }
    return true;
}

bool CliPlugin::tryAddCurFileComment(const QString &line)
{
    // There can be only one empty line
    if (m_currentParsedFile->m_commentsEnd) {
        return false;
    }

    if (line.isEmpty()) {
        m_currentParsedFile->m_commentsEnd = true;
        // If there is an empty line there should be comments
        return !m_currentParsedFile->m_comments.isEmpty();
    }

    // 25 is maximum comment lines count
    if (m_currentParsedFile->m_comments.size() == 25) {
        return false;
    }

    m_currentParsedFile->m_comments << line;
    return true;
}

void CliPlugin::sendCurFileEntry()
{
    Archive::Entry *e = new Archive::Entry(this);

    e->setProperty("fullPath", m_currentParsedFile->m_path);
    e->setProperty("name", m_currentParsedFile->fileName());
    e->setProperty("permissions", m_currentParsedFile->m_attributes);
    e->setProperty("size", m_currentParsedFile->m_origSize);
    e->setProperty("compressedSize", m_currentParsedFile->m_compressedSize);
    e->setProperty("ratio", QStringLiteral("%1").arg(m_currentParsedFile->m_ratio, 0, 'f', 3));
    e->setProperty("version", QStringLiteral("%1").arg(m_currentParsedFile->m_version));
    e->setProperty("timestamp", m_currentParsedFile->m_timeStamp);
    e->setProperty("isDirectory", false);
    e->setProperty("isExecutable", m_currentParsedFile->isExecutable());
    e->setProperty("isPasswordProtected", m_currentParsedFile->m_encrypted);
    if (m_currentParsedFile->m_encrypted) {
        const QMap<ArjFileEntry::EncryptedMethod, QString> methods = {{ArjFileEntry::EncryptedMethodArjOld, i18n("ARJ old")},
                                                                      {ArjFileEntry::EncryptedMethodArjStd, i18n("ARJ")},
                                                                      {ArjFileEntry::EncryptedMethodGost256, i18n("GOST256")},
                                                                      {ArjFileEntry::EncryptedMethodGost256L, i18n("GOST256L")},
                                                                      {ArjFileEntry::EncryptedMethodGost40bit, i18n("GOST 40-bit")}};
        e->setProperty("method", methods.value(m_currentParsedFile->m_encryptedMethod, i18n("unknown")));
    }

    Q_EMIT entry(e);
}

bool CliPlugin::readLine(const QString &line)
{
    // Ignore number of lines corresponding to m_remainingIgnoreLines.
    if (m_remainingIgnoreLines > 0) {
        --m_remainingIgnoreLines;
        return true;
    }

    switch (m_parseState) {
    case ParseStateTitle:
        if (line.startsWith(QStringLiteral("ARJ"))) {
            // Can be obtained ARJ version
            m_parseState = ParseStateProcessing;
            return true;
        }
        return false;

    case ParseStateProcessing:
        if (line.startsWith(QStringLiteral("Processing archive:"))) {
            // Can be obtained archive name
            m_parseState = ParseStateArchiveDateTime;
            return true;
        }
        return false;

    case ParseStateArchiveDateTime:
        if (line.startsWith(QStringLiteral("Archive created:"))) {
            // Can be obtained archive created and modified times
            m_parseState = ParseStateArchiveComments;
            return true;
        }
        return false;

    case ParseStateArchiveComments:
        if (line == QStringLiteral("Sequence/Pathname/Comment/Chapters")) {
            m_parseState = ParseStateEntryFileHeader;
            return true;
        }
        // 25 is maximum comment lines count
        if (m_headerComment.size() == 25) {
            return false;
        }
        m_headerComment << line;
        return true;

    case ParseStateEntryFileHeader:
        if (line.startsWith(QStringLiteral("Rev/Host"))) {
            ignoreLines(1, ParseStateEntryFileName);
            m_currentParsedFile.reset(new ArjFileEntry());
            return true;
        }
        return false;

    case ParseStateEntryFileName: {
        // Send previos file entry
        if (m_currentParsedFile->m_currentEntryNumber > 0) {
            sendCurFileEntry();
        }

        // End file entry
        if (line.startsWith(QStringLiteral("------------"))) {
            if (m_currentParsedFile->m_currentEntryNumber > 0) {
                m_parseState = ParseStateEntryTotal;
                return true;
            }
            return false;
        }

        // Next file entry
        QStringList fNameColumns = line.split(QStringLiteral(") "));
        if (fNameColumns.size() == 2) {
            bool bOk = false;
            int fileNo = fNameColumns.first().toInt(&bOk);
            if (bOk && (fileNo - m_currentParsedFile->m_currentEntryNumber) == 1) {
                m_currentParsedFile.reset(new ArjFileEntry());
                m_currentParsedFile->m_currentEntryNumber = fileNo;
                m_currentParsedFile->m_path = fNameColumns.last();
                m_parseState = ParseStateEntryFileProperty;
                return true;
            }
        }
        return false;
    }

    case ParseStateEntryFileProperty:
        if (tryAddCurFileProperties(line)) {
            m_parseState = ParseStateEntryFileDTA;
            return true;
        }
        // If property parsing fails, the line may be a comment
        return tryAddCurFileComment(line);

    case ParseStateEntryFileDTA:
        if (line.mid(35, 5) == QStringLiteral("DTA  ")) {
            // Can be obtained DTA timestamp
            m_parseState = ParseStateEntryFileDTC;
            return true;
        }
        return false;

    case ParseStateEntryFileDTC:
        if (line.mid(35, 5) == QStringLiteral("DTC  ")) {
            // Can be obtained DTC timestamp
            m_parseState = ParseStateEntryFileName;
            return true;
        }
        return false;

    case ParseStateEntryTotal:
        if (line.size() == 41) {
            // Can be obtained files count, and total sizes
            return true;
        }
        return false;
    }
    return false;
}

#include "cliplugin.moc"
#include "moc_cliplugin.cpp"
