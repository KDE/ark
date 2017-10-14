/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2016 Ragnar Thomsen <rthomsen6@gmail.com>
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

#include "cliproperties.h"
#include "ark_debug.h"
#include "archiveformat.h"
#include "pluginmanager.h"

namespace Kerfuffle
{

CliProperties::CliProperties(QObject *parent, const KPluginMetaData &metaData, const QMimeType &archiveType)
        : QObject(parent)
        , m_mimeType(archiveType)
        , m_metaData(metaData)
{
}

QStringList CliProperties::addArgs(const QString &archive, const QStringList &files, const QString &password, bool headerEncryption, int compressionLevel, const QString &compressionMethod, const QString &encryptionMethod, ulong volumeSize)
{
    if (!encryptionMethod.isEmpty()) {
        Q_ASSERT(!password.isEmpty());
    }

    QStringList args;
    foreach (const QString &s, m_addSwitch) {
        args << s;
    }
    if (!password.isEmpty()) {
        args << substitutePasswordSwitch(password, headerEncryption);
    }
    if (compressionLevel > -1) {
        args << substituteCompressionLevelSwitch(compressionLevel);
    }
    if (!compressionMethod.isEmpty()) {
        args << substituteCompressionMethodSwitch(compressionMethod);
    }
    if (!encryptionMethod.isEmpty()) {
        args << substituteEncryptionMethodSwitch(encryptionMethod);
    }
    if (volumeSize > 0) {
        args << substituteMultiVolumeSwitch(volumeSize);
    }
    args << archive;
    args << files;

    args.removeAll(QString());
    return args;
}

QStringList CliProperties::commentArgs(const QString &archive, const QString &commentfile)
{
    QStringList args;
    foreach (const QString &s, substituteCommentSwitch(commentfile)) {
        args << s;
    }
    args << archive;

    args.removeAll(QString());
    return args;
}

QStringList CliProperties::deleteArgs(const QString &archive, const QVector<Archive::Entry*> &files, const QString &password)
{
    QStringList args;
    args << m_deleteSwitch;
    if (!password.isEmpty()) {
        args << substitutePasswordSwitch(password);
    }
    args << archive;
    foreach (const Archive::Entry *e, files) {
        args << e->fullPath(NoTrailingSlash);
    }

    args.removeAll(QString());
    return args;
}

QStringList CliProperties::extractArgs(const QString &archive, const QStringList &files, bool preservePaths, const QString &password)
{
    QStringList args;

    if (preservePaths && !m_extractSwitch.isEmpty()) {
        args << m_extractSwitch;
    } else if (!preservePaths && !m_extractSwitchNoPreserve.isEmpty()) {
        args << m_extractSwitchNoPreserve;
    }

    if (!password.isEmpty()) {
        args << substitutePasswordSwitch(password);
    }
    args << archive;
    args << files;

    args.removeAll(QString());
    return args;
}

QStringList CliProperties::listArgs(const QString &archive, const QString &password)
{
    QStringList args;
    foreach (const QString &s, m_listSwitch) {
        args << s;
    }
    if (!password.isEmpty()) {
        args << substitutePasswordSwitch(password);
    }
    args << archive;

    args.removeAll(QString());
    return args;
}

QStringList CliProperties::moveArgs(const QString &archive, const QVector<Archive::Entry*> &entries, Archive::Entry *destination, const QString &password)
{
    QStringList args;
    args << m_moveSwitch;
    if (!password.isEmpty()) {
        args << substitutePasswordSwitch(password);
    }
    args << archive;
    if (entries.count() > 1) {
        foreach (const Archive::Entry *file, entries) {
            args << file->fullPath(NoTrailingSlash) << destination->fullPath() + file->name();
        }
    } else {
        args << entries.at(0)->fullPath(NoTrailingSlash) << destination->fullPath(NoTrailingSlash);
    }

    args.removeAll(QString());
    return args;
}

QStringList CliProperties::testArgs(const QString &archive, const QString &password)
{
    QStringList args;
    foreach (const QString &s, m_testSwitch) {
        args << s;
    }
    if (!password.isEmpty()) {
        args << substitutePasswordSwitch(password);
    }
    args << archive;

    args.removeAll(QString());
    return args;
}

QStringList CliProperties::substituteCommentSwitch(const QString &commentfile) const
{
    Q_ASSERT(!commentfile.isEmpty());

    Q_ASSERT(ArchiveFormat::fromMetadata(m_mimeType, m_metaData).supportsWriteComment());

    QStringList commentSwitches = m_commentSwitch;
    Q_ASSERT(!commentSwitches.isEmpty());

    QMutableListIterator<QString> i(commentSwitches);
    while (i.hasNext()) {
        i.next();
        i.value().replace(QLatin1String("$CommentFile"), commentfile);
    }

    return commentSwitches;
}

QStringList CliProperties::substitutePasswordSwitch(const QString &password, bool headerEnc) const
{
    if (password.isEmpty()) {
        return QStringList();
    }

    Archive::EncryptionType encryptionType = ArchiveFormat::fromMetadata(m_mimeType, m_metaData).encryptionType();
    Q_ASSERT(encryptionType != Archive::EncryptionType::Unencrypted);

    QStringList passwordSwitch;
    if (headerEnc) {
        passwordSwitch = m_passwordSwitchHeaderEnc;
    } else {
        passwordSwitch = m_passwordSwitch;
    }
    Q_ASSERT(!passwordSwitch.isEmpty());

    QMutableListIterator<QString> i(passwordSwitch);
    while (i.hasNext()) {
        i.next();
        i.value().replace(QLatin1String("$Password"), password);
    }

    return passwordSwitch;
}

QString CliProperties::substituteCompressionLevelSwitch(int level) const
{
    if (level < 0 || level > 9) {
        return QString();
    }

    Q_ASSERT(ArchiveFormat::fromMetadata(m_mimeType, m_metaData).maxCompressionLevel() != -1);

    QString compLevelSwitch = m_compressionLevelSwitch;
    Q_ASSERT(!compLevelSwitch.isEmpty());

    compLevelSwitch.replace(QLatin1String("$CompressionLevel"), QString::number(level));

    return compLevelSwitch;
}

QString CliProperties::substituteCompressionMethodSwitch(const QString &method) const
{   
    if (method.isEmpty()) {
        return QString();
    }

    Q_ASSERT(!ArchiveFormat::fromMetadata(m_mimeType, m_metaData).compressionMethods().isEmpty());

    QString compMethodSwitch = m_compressionMethodSwitch[m_mimeType.name()].toString();
    Q_ASSERT(!compMethodSwitch.isEmpty());

    QString cliMethod = ArchiveFormat::fromMetadata(m_mimeType, m_metaData).compressionMethods().value(method).toString();

    compMethodSwitch.replace(QLatin1String("$CompressionMethod"), cliMethod);

    return compMethodSwitch;
}

QString CliProperties::substituteEncryptionMethodSwitch(const QString &method) const
{
    if (method.isEmpty()) {
        return QString();
    }

    const ArchiveFormat format = ArchiveFormat::fromMetadata(m_mimeType, m_metaData);

    Q_ASSERT(!format.encryptionMethods().isEmpty());

    QString encMethodSwitch = m_encryptionMethodSwitch[m_mimeType.name()].toString();
    if (encMethodSwitch.isEmpty()) {
        return QString();
    }

    Q_ASSERT(format.encryptionMethods().contains(method));

    encMethodSwitch.replace(QLatin1String("$EncryptionMethod"), method);

    return encMethodSwitch;
}

QString CliProperties::substituteMultiVolumeSwitch(ulong volumeSize) const
{
    // The maximum value we allow in the QDoubleSpinBox is 1,000,000MB. Converted to
    // KB this is 1,024,000,000.
    if (volumeSize <= 0 || volumeSize > 1024000000) {
        return QString();
    }

    Q_ASSERT(ArchiveFormat::fromMetadata(m_mimeType, m_metaData).supportsMultiVolume());

    QString multiVolumeSwitch = m_multiVolumeSwitch;
    Q_ASSERT(!multiVolumeSwitch.isEmpty());

    multiVolumeSwitch.replace(QLatin1String("$VolumeSize"), QString::number(volumeSize));

    return multiVolumeSwitch;
}

bool CliProperties::isPasswordPrompt(const QString &line)
{
    foreach(const QString &rx, m_passwordPromptPatterns) {
        if (QRegularExpression(rx).match(line).hasMatch()) {
            return true;
        }
    }
    return false;
}

bool CliProperties::isWrongPasswordMsg(const QString &line)
{
    foreach(const QString &rx, m_wrongPasswordPatterns) {
        if (QRegularExpression(rx).match(line).hasMatch()) {
            return true;
        }
    }
    return false;
}

bool CliProperties::isTestPassedMsg(const QString &line)
{
    foreach(const QString &rx, m_testPassedPatterns) {
        if (QRegularExpression(rx).match(line).hasMatch()) {
            return true;
        }
    }
    return false;
}

bool CliProperties::isfileExistsMsg(const QString &line)
{
    foreach(const QString &rx, m_fileExistsPatterns) {
        if (QRegularExpression(rx).match(line).hasMatch()) {
            return true;
        }
    }
    return false;
}

bool CliProperties::isFileExistsFileName(const QString &line)
{
    foreach(const QString &rx, m_fileExistsFileName) {
        if (QRegularExpression(rx).match(line).hasMatch()) {
            return true;
        }
    }
    return false;
}

bool CliProperties::isCorruptArchiveMsg(const QString &line)
{
    foreach(const QString &rx, m_corruptArchivePatterns) {
        if (QRegularExpression(rx).match(line).hasMatch()) {
            return true;
        }
    }
    return false;
}

bool CliProperties::isDiskFullMsg(const QString &line)
{
    foreach(const QString &rx, m_diskFullPatterns) {
        if (QRegularExpression(rx).match(line).hasMatch()) {
            return true;
        }
    }
    return false;
}

}
