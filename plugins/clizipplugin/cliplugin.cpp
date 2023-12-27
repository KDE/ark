/*
    SPDX-FileCopyrightText: 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2009-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "cliplugin.h"
#include "ark_debug.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDateTime>
#include <QDir>
#include <QRegularExpression>
#include <QTemporaryDir>

using namespace Kerfuffle;

K_PLUGIN_CLASS_WITH_JSON(CliPlugin, "kerfuffle_clizip.json")

CliPlugin::CliPlugin(QObject *parent, const QVariantList &args)
    : CliInterface(parent, args)
    , m_parseState(ParseStateHeader)
    , m_linesComment(0)
{
    qCDebug(ARK) << "Loaded cli_zip plugin";
    setupCliProperties();
}

CliPlugin::~CliPlugin()
{
}

void CliPlugin::resetParsing()
{
    m_parseState = ParseStateHeader;
    m_tempComment.clear();
    m_comment.clear();
}

// #208091: infozip applies special meanings to some characters, so we
//          need to escape them with backslashes.see match.c in
//          infozip's source code
QString CliPlugin::escapeFileName(const QString &fileName) const
{
    const QString escapedCharacters(QStringLiteral("[]*?^-\\!"));

    QString quoted;
    const int len = fileName.length();
    const QLatin1Char backslash('\\');
    quoted.reserve(len * 2);

    for (int i = 0; i < len; ++i) {
        if (escapedCharacters.contains(fileName.at(i))) {
            quoted.append(backslash);
        }

        quoted.append(fileName.at(i));
    }

    return quoted;
}

void CliPlugin::setupCliProperties()
{
    qCDebug(ARK) << "Setting up parameters...";

    m_cliProps->setProperty("captureProgress", false);

    m_cliProps->setProperty("addProgram", QStringLiteral("zip"));
    m_cliProps->setProperty("addSwitch", QStringList({QStringLiteral("-r")}));

    m_cliProps->setProperty("deleteProgram", QStringLiteral("zip"));
    m_cliProps->setProperty("deleteSwitch", QStringLiteral("-d"));

    m_cliProps->setProperty("extractProgram", QStringLiteral("unzip"));
    m_cliProps->setProperty("extractSwitchNoPreserve", QStringList{QStringLiteral("-j")});

    m_cliProps->setProperty("listProgram", QStringLiteral("zipinfo"));
    m_cliProps->setProperty("listSwitch", QStringList{QStringLiteral("-l"), QStringLiteral("-T"), QStringLiteral("-z")});

    m_cliProps->setProperty("testProgram", QStringLiteral("unzip"));
    m_cliProps->setProperty("testSwitch", QStringLiteral("-t"));

    m_cliProps->setProperty("passwordSwitch", QStringList{QStringLiteral("-P$Password")});

    m_cliProps->setProperty("compressionLevelSwitch", QStringLiteral("-$CompressionLevel"));
    m_cliProps->setProperty("compressionMethodSwitch",
                            QHash<QString, QVariant>{{QStringLiteral("application/zip"), QStringLiteral("-Z$CompressionMethod")},
                                                     {QStringLiteral("application/x-java-archive"), QStringLiteral("-Z$CompressionMethod")}});
    m_cliProps->setProperty("multiVolumeSwitch", QStringLiteral("-v$VolumeSizek"));

    m_cliProps->setProperty("testPassedPatterns", QStringList{QStringLiteral("^No errors detected in compressed data of ")});
    m_cliProps->setProperty("fileExistsFileNameRegExp",
                            QStringList{QStringLiteral("^replace (.+)\\? \\[y\\]es, \\[n\\]o, \\[A\\]ll, \\[N\\]one, \\[r\\]ename: $")});
    m_cliProps->setProperty("fileExistsInput",
                            QStringList{
                                QStringLiteral("y"), // Overwrite
                                QStringLiteral("n"), // Skip
                                QStringLiteral("A"), // Overwrite all
                                QStringLiteral("N"), // Autoskip
                            });
    m_cliProps->setProperty("extractionFailedPatterns", QStringList{QStringLiteral("unsupported compression method")});
}

bool CliPlugin::readListLine(const QString &line)
{
    static const QRegularExpression entryPattern(
        QStringLiteral("^(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\d{8}).(\\d{6})\\s+(.+)$"));

    // RegExp to identify the line preceding comments.
    const QRegularExpression commentPattern(QStringLiteral("^Archive:  .*$"));
    // RegExp to identify the line following comments.
    const QRegularExpression commentEndPattern(QStringLiteral("^Zip file size: .*$"));

    switch (m_parseState) {
    case ParseStateHeader:
        if (commentPattern.match(line).hasMatch()) {
            m_parseState = ParseStateComment;
        } else if (commentEndPattern.match(line).hasMatch()) {
            m_parseState = ParseStateEntry;
        }
        break;
    case ParseStateComment:
        if (commentEndPattern.match(line).hasMatch()) {
            m_parseState = ParseStateEntry;
            if (!m_tempComment.trimmed().isEmpty()) {
                m_comment = m_tempComment.trimmed();
                m_linesComment = m_comment.count(QLatin1Char('\n')) + 1;
                qCDebug(ARK) << "Found a comment with" << m_linesComment << "lines";
            }
        } else {
            m_tempComment.append(line + QLatin1Char('\n'));
        }
        break;
    case ParseStateEntry:
        QRegularExpressionMatch rxMatch = entryPattern.match(line);
        if (rxMatch.hasMatch()) {
            Archive::Entry *e = new Archive::Entry(this);
            e->setProperty("permissions", rxMatch.captured(1));

            // #280354: infozip may not show the right attributes for a given directory, so an entry
            //          ending with '/' is actually more reliable than 'd' bein in the attributes.
            e->setProperty("isDirectory", rxMatch.captured(10).endsWith(QLatin1Char('/')));

            e->setProperty("size", rxMatch.captured(4));
            QString status = rxMatch.captured(5);
            if (status[0].isUpper()) {
                e->setProperty("isPasswordProtected", true);
            }
            e->setProperty("compressedSize", rxMatch.captured(6).toInt());
            e->setProperty("method", rxMatch.captured(7));

            QString method = convertCompressionMethod(rxMatch.captured(7));
            Q_EMIT compressionMethodFound(method);

            const QDateTime ts(QDate::fromString(rxMatch.captured(8), QStringLiteral("yyyyMMdd")),
                               QTime::fromString(rxMatch.captured(9), QStringLiteral("hhmmss")));
            e->setProperty("timestamp", ts);

            e->setProperty("fullPath", rxMatch.captured(10));
            Q_EMIT entry(e);
        }
        break;
    }

    return true;
}

bool CliPlugin::readExtractLine(const QString &line)
{
    const QRegularExpression rxUnsupCompMethod(QStringLiteral("unsupported compression method (\\d+)"));
    const QRegularExpression rxUnsupEncMethod(QStringLiteral("need PK compat. v\\d\\.\\d \\(can do v\\d\\.\\d\\)"));
    const QRegularExpression rxBadCRC(QStringLiteral("bad CRC"));

    QRegularExpressionMatch unsupCompMethodMatch = rxUnsupCompMethod.match(line);
    if (unsupCompMethodMatch.hasMatch()) {
        Q_EMIT error(i18n("Extraction failed due to unsupported compression method (%1).", unsupCompMethodMatch.captured(1)));
        return false;
    }

    if (rxUnsupEncMethod.match(line).hasMatch()) {
        Q_EMIT error(i18n("Extraction failed due to unsupported encryption method."));
        return false;
    }

    if (rxBadCRC.match(line).hasMatch()) {
        Q_EMIT error(i18n("Extraction failed due to one or more corrupt files. Any extracted files may be damaged."));
        return false;
    }

    return true;
}

bool CliPlugin::moveFiles(const QVector<Archive::Entry *> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    qCDebug(ARK) << "Moving" << files.count() << "file(s) to destination:" << destination;

    m_oldWorkingDir = QDir::currentPath();
    m_tempWorkingDir.reset(new QTemporaryDir());
    m_tempAddDir.reset(new QTemporaryDir());
    QDir::setCurrent(m_tempWorkingDir->path());
    m_passedFiles = files;
    m_passedDestination = destination;
    m_passedOptions = options;

    m_subOperation = Extract;
    connect(this, &CliPlugin::finished, this, &CliPlugin::continueMoving);

    return extractFiles(files, QDir::currentPath(), ExtractionOptions());
}

int CliPlugin::moveRequiredSignals() const
{
    return 4;
}

void CliPlugin::continueMoving(bool result)
{
    if (!result) {
        finishMoving(false);
        return;
    }

    switch (m_subOperation) {
    case Extract:
        m_subOperation = Delete;
        if (!deleteFiles(m_passedFiles)) {
            finishMoving(false);
        }
        break;
    case Delete:
        m_subOperation = Add;
        if (!setMovingAddedFiles() || !addFiles(m_tempAddedFiles, m_passedDestination, m_passedOptions)) {
            finishMoving(false);
        }
        break;
    case Add:
        finishMoving(true);
        break;
    default:
        Q_ASSERT(false);
    }
}

bool CliPlugin::setMovingAddedFiles()
{
    m_passedFiles = entriesWithoutChildren(m_passedFiles);
    // If there are more files being moved than 1, we have destination as a destination folder,
    // otherwise it's new entry full path.
    if (m_passedFiles.count() > 1) {
        return setAddedFiles();
    }

    QDir::setCurrent(m_tempAddDir->path());
    const Archive::Entry *file = m_passedFiles.at(0);
    const QString oldPath = m_tempWorkingDir->path() + QLatin1Char('/') + file->fullPath(NoTrailingSlash);
    const QString newPath = m_tempAddDir->path() + QLatin1Char('/') + m_passedDestination->name();
    if (!QFile::rename(oldPath, newPath)) {
        return false;
    }
    m_tempAddedFiles << new Archive::Entry(nullptr, m_passedDestination->name());

    // We have to exclude file name from destination path in order to pass it to addFiles method.
    const QString destinationPath = m_passedDestination->fullPath();
    const int slashCount = destinationPath.count(QLatin1Char('/'));
    if (slashCount > 1 || (slashCount == 1 && !destinationPath.endsWith(QLatin1Char('/')))) {
        int destinationLength = destinationPath.length();
        bool iteratedChar = false;
        do {
            destinationLength--;
            if (destinationPath.at(destinationLength) != QLatin1Char('/')) {
                iteratedChar = true;
            }
        } while (destinationLength > 0 && !(iteratedChar && destinationPath.at(destinationLength) == QLatin1Char('/')));
        m_passedDestination->setProperty("fullPath", destinationPath.left(destinationLength + 1));
    } else {
        // ...unless the destination path is already a single folder, e.g. "dir/", or a file, e.g. "foo.txt".
        // In this case we're going to add to the root, so we just need to set a null destination.
        m_passedDestination = nullptr;
    }

    return true;
}

void CliPlugin::finishMoving(bool result)
{
    disconnect(this, &CliPlugin::finished, this, &CliPlugin::continueMoving);
    Q_EMIT progress(1.0);
    Q_EMIT finished(result);
    cleanUp();
}

QString CliPlugin::convertCompressionMethod(const QString &method)
{
    if (method == QLatin1String("stor")) {
        return QStringLiteral("Store");
    } else if (method.startsWith(QLatin1String("def"))) {
        return QStringLiteral("Deflate");
    } else if (method == QLatin1String("d64N")) {
        return QStringLiteral("Deflate64");
    } else if (method == QLatin1String("bzp2")) {
        return QStringLiteral("BZip2");
    } else if (method == QLatin1String("lzma")) {
        return QStringLiteral("LZMA");
    } else if (method == QLatin1String("ppmd")) {
        return QStringLiteral("PPMd");
    } else if (method == QLatin1String("u095")) {
        return QStringLiteral("XZ");
    } else if (method == QLatin1String("u099")) {
        Q_EMIT encryptionMethodFound(QStringLiteral("AES"));
        return i18nc("referred to compression method", "unknown");
    }
    return method;
}

bool CliPlugin::isPasswordPrompt(const QString &line)
{
    return line.endsWith(QLatin1String(" password: "));
}

bool CliPlugin::isWrongPasswordMsg(const QString &line)
{
    return line.endsWith(QLatin1String("incorrect password"));
}

bool CliPlugin::isCorruptArchiveMsg(const QString &line)
{
    return (line.contains(QLatin1String("End-of-central-directory signature not found"))
            || line.contains(QLatin1String("didn't find end-of-central-dir signature at end of central dir")));
}

bool CliPlugin::isDiskFullMsg(const QString &line)
{
    return (line.contains(QLatin1String("No space left on device")) || line.contains(QLatin1String("write error (disk full?)")));
}

bool CliPlugin::isFileExistsMsg(const QString &line)
{
    return (line.startsWith(QLatin1String("replace ")) && line.endsWith(QLatin1String("? [y]es, [n]o, [A]ll, [N]one, [r]ename: ")));
}

bool CliPlugin::isFileExistsFileName(const QString &line)
{
    return (line.startsWith(QLatin1String("replace ")) && line.endsWith(QLatin1String("? [y]es, [n]o, [A]ll, [N]one, [r]ename: ")));
}

#include "cliplugin.moc"
#include "moc_cliplugin.cpp"
