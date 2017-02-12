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
#ifndef CLIPROPERTIES_H
#define CLIPROPERTIES_H

#include "archiveinterface.h"
#include "kerfuffle_export.h"

#include <QRegularExpression>

namespace Kerfuffle
{

class KERFUFFLE_EXPORT CliProperties: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString addProgram MEMBER m_addProgram)
    Q_PROPERTY(QString deleteProgram MEMBER m_deleteProgram)
    Q_PROPERTY(QString extractProgram MEMBER m_extractProgram)
    Q_PROPERTY(QString listProgram MEMBER m_listProgram)
    Q_PROPERTY(QString moveProgram MEMBER m_moveProgram)
    Q_PROPERTY(QString testProgram MEMBER m_testProgram)

    Q_PROPERTY(QStringList addSwitch MEMBER m_addSwitch)
    Q_PROPERTY(QStringList commentSwitch MEMBER m_commentSwitch)
    Q_PROPERTY(QString deleteSwitch MEMBER m_deleteSwitch)
    Q_PROPERTY(QStringList extractSwitch MEMBER m_extractSwitch)
    Q_PROPERTY(QStringList extractSwitchNoPreserve MEMBER m_extractSwitchNoPreserve)
    Q_PROPERTY(QStringList listSwitch MEMBER m_listSwitch)
    Q_PROPERTY(QString moveSwitch MEMBER m_moveSwitch)
    Q_PROPERTY(QStringList testSwitch MEMBER m_testSwitch)

    Q_PROPERTY(QStringList passwordSwitch MEMBER m_passwordSwitch)
    Q_PROPERTY(QStringList passwordSwitchHeaderEnc MEMBER m_passwordSwitchHeaderEnc)
    Q_PROPERTY(QString compressionLevelSwitch MEMBER m_compressionLevelSwitch)
    Q_PROPERTY(QHash<QString,QVariant> compressionMethodSwitch MEMBER m_compressionMethodSwitch)
    Q_PROPERTY(QHash<QString,QVariant> encryptionMethodSwitch MEMBER m_encryptionMethodSwitch)
    Q_PROPERTY(QString multiVolumeSwitch MEMBER m_multiVolumeSwitch)

    Q_PROPERTY(QStringList passwordPromptPatterns MEMBER m_passwordPromptPatterns)
    Q_PROPERTY(QStringList wrongPasswordPatterns MEMBER m_wrongPasswordPatterns)
    Q_PROPERTY(QStringList testPassedPatterns MEMBER m_testPassedPatterns)
    Q_PROPERTY(QStringList fileExistsPatterns MEMBER m_fileExistsPatterns)
    Q_PROPERTY(QStringList fileExistsFileName MEMBER m_fileExistsFileName)
    Q_PROPERTY(QStringList corruptArchivePatterns MEMBER m_corruptArchivePatterns)
    Q_PROPERTY(QStringList diskFullPatterns MEMBER m_diskFullPatterns)

    Q_PROPERTY(QStringList fileExistsInput MEMBER m_fileExistsInput)
    Q_PROPERTY(QStringList multiVolumeSuffix MEMBER m_multiVolumeSuffix)

    Q_PROPERTY(bool captureProgress MEMBER m_captureProgress)

public:
    explicit CliProperties(QObject *parent, const KPluginMetaData &metaData, const QMimeType &archiveType);

    QStringList addArgs(const QString &archive,
                        const QStringList &files,
                        const QString &password,
                        bool headerEncryption,
                        int compressionLevel,
                        const QString &compressionMethod,
                        const QString &encryptionMethod,
                        ulong volumeSize);
    QStringList commentArgs(const QString &archive, const QString &commentfile);
    QStringList deleteArgs(const QString &archive, const QVector<Archive::Entry*> &files, const QString &password);
    QStringList extractArgs(const QString &archive, const QStringList &files, bool preservePaths, const QString &password);
    QStringList listArgs(const QString &archive, const QString &password);
    QStringList moveArgs(const QString &archive, const QVector<Archive::Entry *> &entries, Archive::Entry *destination, const QString &password);
    QStringList testArgs(const QString &archive, const QString &password);

    bool isPasswordPrompt(const QString &line);
    bool isWrongPasswordMsg(const QString &line);
    bool isTestPassedMsg(const QString &line);
    bool isfileExistsMsg(const QString &line);
    bool isFileExistsFileName(const QString &line);
    bool isCorruptArchiveMsg(const QString &line);
    bool isDiskFullMsg(const QString &line);

private:
    QStringList substituteCommentSwitch(const QString &commentfile) const;
    QStringList substitutePasswordSwitch(const QString &password, bool headerEnc = false) const;
    QString substituteCompressionLevelSwitch(int level) const;
    QString substituteCompressionMethodSwitch(const QString &method) const;
    QString substituteEncryptionMethodSwitch(const QString &method) const;
    QString substituteMultiVolumeSwitch(ulong volumeSize) const;

    QString m_addProgram;
    QString m_deleteProgram;
    QString m_extractProgram;
    QString m_listProgram;
    QString m_moveProgram;
    QString m_testProgram;

    QStringList m_addSwitch;
    QStringList m_commentSwitch;
    QString m_deleteSwitch;
    QStringList m_extractSwitch;
    QStringList m_extractSwitchNoPreserve;
    QStringList m_listSwitch;
    QString m_moveSwitch;
    QStringList m_testSwitch;

    QStringList m_passwordSwitch;
    QStringList m_passwordSwitchHeaderEnc;
    QString m_compressionLevelSwitch;
    QHash<QString,QVariant> m_compressionMethodSwitch;
    QHash<QString,QVariant> m_encryptionMethodSwitch;
    QString m_multiVolumeSwitch;

    QStringList m_passwordPromptPatterns;
    QStringList m_wrongPasswordPatterns;
    QStringList m_testPassedPatterns;
    QStringList m_fileExistsPatterns;
    QStringList m_fileExistsFileName;
    QStringList m_corruptArchivePatterns;
    QStringList m_diskFullPatterns;

    QStringList m_fileExistsInput;
    QStringList m_multiVolumeSuffix;

    bool m_captureProgress = false;

    QMimeType m_mimeType;
    KPluginMetaData m_metaData;
};
}

#endif /* CLIPROPERTIES_H */
