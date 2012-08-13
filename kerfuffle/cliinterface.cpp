/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (C) 2012 basysKom GmbH <info@basyskom.com>
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

#include "cliinterface.h"
#include "queries.h"

#ifdef Q_OS_WIN
# include <KProcess>
#else
# include <KPtyDevice>
# include <KPtyProcess>
#endif

#include <KStandardDirs>
#include <KDebug>
#include <KLocale>
#include <kencodingprober.h>
#include <KUrl>

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QProcess>
#include <QThread>
#include <QTimer>
#include <QTextCodec>

namespace Kerfuffle
{
CliInterface::CliInterface(QObject *parent, const QVariantList & args)
    : ReadWriteArchiveInterface(parent, args),
      m_process(0),
      m_testResult(true),
      m_fixFileNameEncoding(false)
{
    //because this interface uses the event loop
    setWaitForFinishedSignal(true);

    if (QMetaType::type("QProcess::ExitStatus") == 0) {
        qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
    }

    connect(QApplication::instance(), SIGNAL(lastWindowClosed()), this, SLOT(killAllProcesses()));
}

void CliInterface::cacheParameterList()
{
    m_param = parameterList();
    Q_ASSERT(m_param.contains(ExtractProgram));
    Q_ASSERT(m_param.contains(ListProgram));
    Q_ASSERT(m_param.contains(PreservePathSwitch));
    Q_ASSERT(m_param.contains(FileExistsExpression));
    Q_ASSERT(m_param.contains(FileExistsInput));
}

CliInterface::~CliInterface()
{
    Q_ASSERT(!m_process);
}

bool CliInterface::supportsParameter(CliInterfaceParameters param)
{
    if (m_param.isEmpty())
        cacheParameterList();

    bool hasParam = false;

    if (m_param.contains(param)) {
        QVariant var = m_param.value(param);
        switch (var.type()) {
        case QVariant::StringList:
            hasParam = !var.toStringList().isEmpty();
            break;

        case QVariant::String:
            hasParam = !var.toString().isEmpty();
            break;

        case QVariant::Bool:
            hasParam = var.toBool();
            break;

        default:
            break;
        }
    }

    return hasParam;
}

bool CliInterface::supportsOption(const Kerfuffle::SupportedOptions option, const QString & mimeType)
{
    switch (option) {
    case CompressionLevel:
        return supportsParameter(CompressionLevelSwitches);
        break;
    case Testing:
        return supportsParameter(TestProgram);
        break;
    case FixFileNameEncoding:
        // for now only libarchive plugin supports this.
        return false;
        break;
    case MultiPart:
        return supportsParameter(MultiPartSwitch);
        break;
    case MultiThreading:
        return supportsParameter(MultiThreadingSwitch);
        break;
    case EncryptionMethod:
        return supportsParameter(EncryptionMethodSwitches);
        break;
    case EncryptHeader:
        return supportsParameter(EncryptHeaderSwitch);
        break;
    case Password:
        return supportsParameter(PasswordSwitch);
        break;
    case PreservePath:
        return supportsParameter(PreservePathSwitch);
        break;
    case RootNode:
        return supportsParameter(RootNodeSwitch);
        break;
    case Rename:
        return supportsParameter(SupportsRename);
        break;
    case IOWrite:
        if (mimeType == QLatin1String("application/split7z") || mimeType == QLatin1String("application/splitzip")) {
            return false;
        } else {
            return true;
        }
        break;
    }
    return false;
}

bool CliInterface::list()
{
    resetReadState();
    cacheParameterList();
    m_operationMode = List;

    QStringList args = m_param.value(ListArgs).toStringList();
    substituteListVariables(args);

    if (!runProcess(m_param.value(ListProgram).toStringList(), args)) {
        failOperation();
        return false;
    }

    return true;
}

bool CliInterface::copyFiles(const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options)
{
    kDebug(1601);
    cacheParameterList();

    m_operationMode = Copy;
    m_options = options;

    //start preparing the argument list
    QStringList args = m_param.value(ExtractArgs).toStringList();

    //now replace the various elements in the list
    for (int i = 0; i < args.size(); ++i) {
        QString argument = args.at(i);
        kDebug(1601) << "Processing argument " << argument;

        if (argument == QLatin1String("$Archive")) {
            args[i] = filename();
        }

        if (argument == QLatin1String("$MultiThreadingSwitch")) {
            QString multiThreadingSwitch = m_param.value(MultiThreadingSwitch).toString();
            bool multiThreading = options.value(QLatin1String("MultiThreadingEnabled")).toBool();

            QString theReplacement;
            if (multiThreading == true) {
                theReplacement = multiThreadingSwitch;
            }

            if (theReplacement.isEmpty()) {
                args.removeAt(i);
                --i; //decrement to compensate for the variable we removed
            } else {
                //but in this case we don't have to decrement, we just
                //replace it
                args[i] = theReplacement;
            }
        }

        if (argument == QLatin1String("$PreservePathSwitch")) {
            QStringList replacementFlags = m_param.value(PreservePathSwitch).toStringList();
            Q_ASSERT(replacementFlags.size() == 2);

            bool preservePaths = options.value(QLatin1String("PreservePaths")).toBool();
            QString theReplacement;
            if (preservePaths) {
                theReplacement = replacementFlags.at(0);
            } else {
                theReplacement = replacementFlags.at(1);
            }

            if (theReplacement.isEmpty()) {
                args.removeAt(i);
                --i; //decrement to compensate for the variable we removed
            } else {
                //but in this case we don't have to decrement, we just
                //replace it
                args[i] = theReplacement;
            }
        }

        if (argument == QLatin1String("$PasswordSwitch")) {
            //if the PasswordSwitch argument has been added, we at least
            //assume that the format of the switch has been added as well
            Q_ASSERT(m_param.contains(PasswordSwitch));

            //we will decrement i afterwards
            args.removeAt(i);

            //if we get a hint about this being a password protected archive, ask about
            //the password in advance.
            if ((options.value(QLatin1String("PasswordProtectedHint")).toBool()) &&
                    (password().isEmpty())) {
                kDebug(1601) << "Password hint enabled, querying user";

                Kerfuffle::PasswordNeededQuery query(filename());
                emit userQuery(&query);
                query.waitForResponse();

                if (query.responseCancelled()) {
                    failOperation();
                    return false;
                }
                setPassword(query.password());
            }

            QString pass = password();

            if (!pass.isEmpty()) {
                QStringList theSwitch = m_param.value(PasswordSwitch).toStringList();
                for (int j = 0; j < theSwitch.size(); ++j) {
                    //get the argument part
                    QString newArg = theSwitch.at(j);

                    //substitute the $Path
                    newArg.replace(QLatin1String("$Password"), pass);

                    //put it in the arg list
                    args.insert(i + j, newArg);
                    ++i;

                }
            }
            --i; //decrement to compensate for the variable we replaced
        }

        if (argument == QLatin1String("$RootNodeSwitch")) {
            //if the RootNodeSwitch argument has been added, we at least
            //assume that the format of the switch has been added as well
            Q_ASSERT(m_param.contains(RootNodeSwitch));

            //we will decrement i afterwards
            args.removeAt(i);

            QString rootNode;
            if (options.contains(QLatin1String("RootNode"))) {
                rootNode = options.value(QLatin1String("RootNode")).toString();
                kDebug(1601) << "Set root node " << rootNode;
            }

            if (!rootNode.isEmpty()) {
                QStringList theSwitch = m_param.value(RootNodeSwitch).toStringList();
                for (int j = 0; j < theSwitch.size(); ++j) {
                    //get the argument part
                    QString newArg = theSwitch.at(j);

                    //substitute the $Path
                    newArg.replace(QLatin1String("$Path"), rootNode);

                    //put it in the arg list
                    args.insert(i + j, newArg);
                    ++i;

                }
            }
            --i; //decrement to compensate for the variable we replaced
        }

        if (argument == QLatin1String("$FileExistsSwitch")) {
            //if the FileExistsSwitch argument has been added, we at least
            //assume that the format of the switch has been added as well
            Q_ASSERT(m_param.contains(FileExistsSwitch));

            int conflictOption = -1;
            if (options.contains(QLatin1String("ConflictsHandling"))) {
                conflictOption = options.value(QLatin1String("ConflictsHandling")).toInt();
            }

            QString theReplacement;
            if (conflictOption > -1) {
                theReplacement = m_param.value(FileExistsSwitch).toStringList().at(conflictOption);
            }

            if (theReplacement.isEmpty()) {
                args.removeAt(i);
                --i; //decrement to compensate for the variable we removed
            } else {
                //but in this case we don't have to decrement, we just
                //replace it
                args[i] = theReplacement;
            }
        }

        if (argument == QLatin1String("$Files")) {
            args.removeAt(i);
            for (int j = 0; j < files.count(); ++j) {
                args.insert(i + j, escapeFileName(files.at(j).toString()));
                ++i;
            }
            --i;
        }
    }

    kDebug(1601) << "Setting current dir to " << destinationDirectory;
    QDir::setCurrent(destinationDirectory);

    m_fixFileNameEncoding = options.value(QLatin1String("FixFileNameEncoding")).toBool();
    m_destinationDirectory = destinationDirectory;

    if (!runProcess(m_param.value(ExtractProgram).toStringList(), args)) {
        failOperation();
        return false;
    }

    return true;
}

void CliInterface::fixFileNameEncoding(const QString & destinationDirectory)
{
    if (destinationDirectory.isEmpty()) {
        return;
    }

    QDir destDir(destinationDirectory);
    destDir.setFilter(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);

    QStringList list = destDir.entryList();
    for (int i = 0; i < list.size(); ++i) {
        QString encodingCorrectedString = autoConvertEncoding(list.at(i));

        if (list.at(i) != encodingCorrectedString) {
            if (!QFile::rename(list.at(i), encodingCorrectedString)) {
                kWarning() << "Renaming" << list.at(i) << "to" << encodingCorrectedString << "failed";
            }
        }
    }
}

bool CliInterface::addFiles(const QStringList & files, const CompressionOptions& options)
{
    cacheParameterList();

    m_operationMode = Add;
    m_options = options;

    const QString globalWorkDir = options.value(QLatin1String("GlobalWorkDir")).toString();
    const QDir workDir = globalWorkDir.isEmpty() ? QDir::current() : QDir(globalWorkDir);
    if (!globalWorkDir.isEmpty()) {
        kDebug(1601) << "GlobalWorkDir is set, changing dir to " << globalWorkDir;
        QDir::setCurrent(globalWorkDir);
    }

    //start preparing the argument list
    QStringList args = m_param.value(AddArgs).toStringList();

    //now replace the various elements in the list
    for (int i = 0; i < args.size(); ++i) {
        const QString argument = args.at(i);
        kDebug(1601) << "Processing argument " << argument;

        if (argument == QLatin1String("$CompressionLevelSwitch")) {
            QStringList compressionLevelSwitches = m_param.value(CompressionLevelSwitches).toStringList();
            QString theReplacement = compressionLevelSwitches.at(options.value(QLatin1String("CompressionLevel"), 4).toInt());

            if (theReplacement.isEmpty()) {
                args.removeAt(i);
                --i; //decrement to compensate for the variable we removed
            } else {
                //but in this case we don't have to decrement, we just
                //replace it
                args[i] = theReplacement;
            }
        }

        if (argument == QLatin1String("$MultiThreadingSwitch")) {
            QString multiThreadingSwitch = m_param.value(MultiThreadingSwitch).toString();
            bool multiThreading = options.value(QLatin1String("MultiThreadingEnabled")).toBool();

            QString theReplacement;
            if (multiThreading == true) {
                theReplacement = multiThreadingSwitch;
            }

            if (theReplacement.isEmpty()) {
                args.removeAt(i);
                --i; //decrement to compensate for the variable we removed
            } else {
                //but in this case we don't have to decrement, we just
                //replace it
                args[i] = theReplacement;
            }
        }

        if (argument == QLatin1String("$TemporaryDirectorySwitch")) {
            QString theReplacement = m_param.value(TemporaryDirectorySwitch).toString();
            theReplacement.replace(QLatin1String("$DirectoryPath"), KGlobal::dirs()->findDirs("tmp", QLatin1String(""))[0]); //krazy:exclude=doublequote_chars
            args[i] = theReplacement;
        }

        if (argument == QLatin1String("$MultiPartSwitch")) {
            QString multiPartSwitch = m_param.value(MultiPartSwitch).toString();
            int multiPartSize = options.value(QLatin1String("MultiPartSize")).toInt();

            QString theReplacement;
            if (multiPartSize != 0) {
                theReplacement = multiPartSwitch;
                theReplacement.replace(QLatin1String("$MultiPartSize"), QString::number(multiPartSize));
            }

            if (theReplacement.isEmpty()) {
                args.removeAt(i);
                --i; //decrement to compensate for the variable we removed
            } else {
                //but in this case we don't have to decrement, we just
                //replace it
                args[i] = theReplacement;
            }
        }

        if (argument == QLatin1String("$PasswordSwitch")) {
            //if the PasswordSwitch argument has been added, we at least
            //assume that the format of the switch has been added as well
            Q_ASSERT(m_param.contains(PasswordSwitch));

            //we will decrement i afterwards
            args.removeAt(i);

            //if we get a hint about this being a password protected archive, ask about
            //the password in advance.
            if ((options.value(QLatin1String("PasswordProtectedHint")).toBool()) &&
                    (password().isEmpty())) {
                kDebug(1601) << "Password hint enabled, querying user";

                Kerfuffle::PasswordNeededQuery query(filename(), Kerfuffle::PasswordNeededQuery::AskNewPassword);
                userQuery(&query);
                query.waitForResponse();

                if (query.responseCancelled()) {
                    failOperation();
                    return false;
                }
                setPassword(query.password());
            }

            QString pass = password();

            if (!pass.isEmpty()) {
                QStringList theSwitch = m_param.value(PasswordSwitch).toStringList();
                for (int j = 0; j < theSwitch.size(); ++j) {
                    //get the argument part
                    QString newArg = theSwitch.at(j);

                    //substitute the $Path
                    newArg.replace(QLatin1String("$Password"), pass);

                    //put it in the arg list
                    args.insert(i + j, newArg);
                    ++i;

                }
            }
            --i; //decrement to compensate for the variable we replaced
        }

        if (argument == QLatin1String("$EncryptHeaderSwitch")) {
            QString encryptHeaderSwitch = m_param.value(EncryptHeaderSwitch).toString();
            bool encryptHeader = options.value(QLatin1String("EncryptHeaderEnabled")).toBool();

            QString theReplacement;

            if (encryptHeader && encryptHeaderSwitch.contains(QLatin1String("$Password"))) {
                // password should have been set at this point (see above) but check anyway
                if (password().isEmpty()) {
                    kDebug(1601) << "Password hint enabled, querying user";

                    Kerfuffle::PasswordNeededQuery query(filename(), Kerfuffle::PasswordNeededQuery::AskNewPassword);
                    userQuery(&query);
                    query.waitForResponse();

                    if (query.responseCancelled()) {
                        failOperation();
                        return false;
                    }
                    setPassword(query.password());
                }

                QString pass = password();
                if (!pass.isEmpty()) {
                    encryptHeaderSwitch.replace(QLatin1String("$Password"), pass);
                }

                theReplacement = encryptHeaderSwitch;

            } else if (encryptHeader && !encryptHeaderSwitch.contains(QLatin1String("$Password"))) {
                theReplacement = encryptHeaderSwitch;
            }

            if (theReplacement.isEmpty()) {
                args.removeAt(i);
                --i; //decrement to compensate for the variable we removed
            } else {
                //but in this case we don't have to decrement, we just
                //replace it
                args[i] = theReplacement;
            }
        }

        if (argument == QLatin1String("$EncryptionMethodSwitches")) {
            QStringList encryptionMethodSwitches = m_param.value(EncryptionMethodSwitches).toStringList();
            // 0 == AES256 , 1 == Zipcrypto
            QString theReplacement = encryptionMethodSwitches.at(options.value(QLatin1String("EncryptionMethod"), 0).toInt());

            if (theReplacement.isEmpty()) {
                args.removeAt(i);
                --i; //decrement to compensate for the variable we removed
            } else {
                //but in this case we don't have to decrement, we just
                //replace it
                args[i] = theReplacement;
            }
        }

        if (argument == QLatin1String("$Archive")) {
            args[i] = filename();
        }

        if (argument == QLatin1String("$Files")) {
            args.removeAt(i);
            for (int j = 0; j < files.count(); ++j) {
                // #191821: workDir must be used instead of QDir::current()
                //          so that symlinks aren't resolved automatically
                // TODO: this kind of call should be moved upwards in the
                //       class hierarchy to avoid code duplication
                const QString relativeName =
                    workDir.relativeFilePath(files.at(j));

                args.insert(i + j, relativeName);
                ++i;
            }
            --i;
        }
    }

    kDebug(1601) << args;
    if (!runProcess(m_param.value(AddProgram).toStringList(), args)) {
        failOperation();
        return false;
    }

    return true;
}

bool CliInterface::deleteFiles(const QList<QVariant> & files)
{
    cacheParameterList();
    m_operationMode = Delete;

    //start preparing the argument list
    QStringList args = m_param.value(DeleteArgs).toStringList();

    //now replace the various elements in the list
    for (int i = 0; i < args.size(); ++i) {
        QString argument = args.at(i);
        kDebug(1601) << "Processing argument " << argument;

        if (argument == QLatin1String("$Archive")) {
            args[i] = filename();
        } else if (argument == QLatin1String("$Files")) {
            args.removeAt(i);
            for (int j = 0; j < files.count(); ++j) {
                args.insert(i + j, escapeFileName(files.at(j).toString()));
                ++i;
            }
            --i;
        }
    }

    m_removedFiles = files;

    if (!runProcess(m_param.value(DeleteProgram).toStringList(), args)) {
        failOperation();
        return false;
    }

    return true;
}

bool CliInterface::testFiles(const QList<QVariant> & files, TestOptions options)
{
    Q_UNUSED(options)
    kDebug(1601);
    cacheParameterList();

    m_testResult = true;
    m_operationMode = Test;
    m_options = options;

    //start preparing the argument list
    QStringList args = m_param.value(TestArgs).toStringList();

    //now replace the various elements in the list
    for (int i = 0; i < args.size(); ++i) {
        QString argument = args.at(i);
        kDebug(1601) << "Processing argument " << argument;

        if (argument == QLatin1String("$Archive")) {
            args[i] = filename();
        }

        if (argument == QLatin1String("$PasswordSwitch")) {
            //if the PasswordSwitch argument has been added, we at least
            //assume that the format of the switch has been added as well
            Q_ASSERT(m_param.contains(PasswordSwitch));

            //we will decrement i afterwards
            args.removeAt(i);

            //if we get a hint about this being a password protected archive, ask about
            //the password in advance.
            if ((options.value(QLatin1String("PasswordProtectedHint")).toBool()) &&
                    (password().isEmpty())) {
                kDebug(1601) << "Password hint enabled, querying user";

                Kerfuffle::PasswordNeededQuery query(filename());
                userQuery(&query);
                query.waitForResponse();

                if (query.responseCancelled()) {
                    failOperation();
                    return false;
                }
                setPassword(query.password());
            }

            QString pass = password();

            if (!pass.isEmpty()) {
                QStringList theSwitch = m_param.value(PasswordSwitch).toStringList();
                for (int j = 0; j < theSwitch.size(); ++j) {
                    //get the argument part
                    QString newArg = theSwitch.at(j);

                    //substitute the $Path
                    newArg.replace(QLatin1String("$Password"), pass);

                    //put it in the arg list
                    args.insert(i + j, newArg);
                    ++i;

                }
            }
            --i; //decrement to compensate for the variable we replaced
        }

        if (argument == QLatin1String("$Files")) {
            args.removeAt(i);
            for (int j = 0; j < files.count(); ++j) {
                args.insert(i + j, escapeFileName(files.at(j).toString()));
                ++i;
            }
            --i;
        }
    }

    if (!runProcess(m_param.value(TestProgram).toStringList(), args)) {
        failOperation();
        return false;
    }

    return m_testResult;
}

bool CliInterface::runProcess(const QStringList& programNames, const QStringList& arguments)
{
    QString programPath;
    for (int i = 0; i < programNames.count(); i++) {
        programPath = KStandardDirs::findExe(programNames.at(i));
        if (!programPath.isEmpty())
            break;
    }
    if (programPath.isEmpty()) {
        const QString names = programNames.join(QLatin1String(", "));
        emit error(i18ncp("@info", "Failed to locate program <filename>%2</filename> on disk.",
                                   "Failed to locate programs <filename>%2</filename> on disk.", programNames.count(), names));
        return false;
    }

    kDebug(1601) << "Executing" << programPath << arguments;

    if (m_process) {
        m_process->waitForFinished();
        delete m_process;
    }

#ifdef Q_OS_WIN
    m_process = new KProcess;
#else
    m_process = new KPtyProcess;
    m_process->setPtyChannels(KPtyProcess::StdinChannel);
#endif

    m_process->setOutputChannelMode(KProcess::MergedChannels);
    m_process->setNextOpenMode(QIODevice::ReadWrite | QIODevice::Unbuffered | QIODevice::Text);
    m_process->setProgram(programPath, arguments);

    connect(m_process, SIGNAL(readyReadStandardOutput()), SLOT(readStdout()), Qt::DirectConnection);
    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(processFinished(int,QProcess::ExitStatus)), Qt::DirectConnection);

    m_stdOutData.clear();

    m_process->start();

#ifdef Q_OS_WIN
    bool ret = m_process->waitForFinished(-1);
#else
    QEventLoop loop;
    bool ret = (loop.exec(QEventLoop::WaitForMoreEvents | QEventLoop::ExcludeUserInputEvents) == 0);
#endif

    Q_ASSERT(!m_process);

    return ret;
}

void CliInterface::killAllProcesses()
{
    if (!qApp->quitOnLastWindowClosed()) {
        return;
    }

    kDebug(1601) << "Killing process";
    doKill();
    finished(true);
}

void CliInterface::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)

    kDebug(1601);

    //if the m_process pointer is gone, then there is nothing to worry
    //about here
    if (!m_process) {
        if (m_operationMode == Copy && m_fixFileNameEncoding) {
            fixFileNameEncoding(m_destinationDirectory);
            m_destinationDirectory.clear();
        }
        return;
    }

    if (m_operationMode == Delete) {
        foreach(const QVariant & v, m_removedFiles) {
            entryRemoved(v.toString());
        }
    }

    //handle all the remaining data in the process
    readStdout(true);

    delete m_process;
    m_process = 0;

    emit progress(1.0);

    if (m_operationMode == Add) {
        list();
        return;
    }

    // if readStdout above needs to handle password request then we need to wait for it
    // before we can call fixFileNameEncoding().
    if (m_operationMode == Copy && m_fixFileNameEncoding) {
        fixFileNameEncoding(m_destinationDirectory);
        m_destinationDirectory.clear();
    }

    //and we're finished
    emit finished(true);
}

void CliInterface::failOperation()
{
    // TODO: Would be good to unit test #304764/#304178.
    kDebug(1601);
    doKill();
}

void CliInterface::readStdout(bool handleAll)
{
    //when hacking this function, please remember the following:
    //- standard output comes in unpredictable chunks, this is why
    //you can never know if the last part of the output is a complete line or not
    //- console applications are not really consistent about what
    //characters they send out (newline, backspace, carriage return,
    //etc), so keep in mind that this function is supposed to handle
    //all those special cases and be the lowest common denominator

    Q_ASSERT(m_process);

    if (!m_process->bytesAvailable()) {
        //if process has no more data, we can just bail out
        return;
    }

    //if the process is still not finished (m_process is appearantly not
    //set to NULL if here), then the operation should definitely not be in
    //the main thread as this would freeze everything. assert this.
    Q_ASSERT(QThread::currentThread() != QApplication::instance()->thread());

    QByteArray dd = m_process->readAllStandardOutput();
    m_stdOutData += dd;

    QList<QByteArray> lines = m_stdOutData.split('\n');

    //The reason for this check is that archivers often do not end
    //queries (such as file exists, wrong password) on a new line, but
    //freeze waiting for input. So we check for errors on the last line in
    //all cases.
    // TODO: QLatin1String() might not be the best choice here.
    //       The call to handleLine() at the end of the method uses
    //       QString::fromLocal8Bit(), for example.
    // TODO: The same check methods are called in handleLine(), this
    //       is suboptimal.
    bool foundErrorMessage =
        (checkForErrorMessage(QLatin1String(lines.last()), WrongPasswordPatterns) ||
         checkForErrorMessage(QLatin1String(lines.last()), ExtractionFailedPatterns) ||
         checkForPasswordPromptMessage(QLatin1String(lines.last())) ||
         checkForFileExistsMessage(QLatin1String(lines.last())) ||
         checkForRenameFileMessage(QLatin1String(lines.last())) ||
         checkForUseCurrentPasswordMessage(QLatin1String(lines.last())));

    if (foundErrorMessage) {
        handleAll = true;
    }

    //this is complex, here's an explanation:
    //if there is no newline, then there is no guaranteed full line to
    //handle in the output. The exception is that it is supposed to handle
    //all the data, OR if there's been an error message found in the
    //partial data.
    if (lines.size() == 1 && !handleAll) {
        return;
    }

    if (handleAll) {
        m_stdOutData.clear();
    } else {
        //because the last line might be incomplete we leave it for now
        //note, this last line may be an empty string if the stdoutdata ends
        //with a newline
        m_stdOutData = lines.takeLast();
    }

    foreach(const QByteArray & line, lines) {
        // sometimes 7z does not list all file's metadata. We need to
        // pass the empty line to mark that the file's metadata has ended
        // and whatever we have got so far must be added to the archive
        // folder structure.
        if (!line.isEmpty() || m_operationMode == List) {
            handleLine(QString::fromLocal8Bit(line));
        }
    }
}

void CliInterface::handleLine(const QString& line)
{
    // TODO: This should be implemented by each plugin; the way progress is
    //       shown by each CLI application is subject to a lot of variation.
    if ((m_operationMode == Copy || m_operationMode == Add || m_operationMode == Test) && m_param.contains(CaptureProgress) && m_param.value(CaptureProgress).toBool()) {
        //read the percentage
        int pos = line.indexOf(QLatin1Char('%'));
        if (pos != -1 && pos > 1) {
            int percentage = line.mid(pos - 2, 2).toInt();
            emit progress(float(percentage) / 100);
            return;
        }
    }

    if (m_operationMode == Add) {
        if (checkForErrorMessage(line, AddFailedPatterns)) {
            setPassword(QString());
            error(i18nc("@info Adding files to an archive failed for some reason",
                        "Adding files failed with the following message:\n%1", line));
            failOperation();
            return;
        }

        if (handleFileExistsMessage(line)) {
            return;
        }

        if (handleRenameFileMessage(line)) {
            return;
        }
    }

    if (m_operationMode == Copy || m_operationMode == Delete) {
        if (checkForPasswordPromptMessage(line)) {
            kDebug(1601) << "Found a password prompt";

            Kerfuffle::PasswordNeededQuery query(filename());
            emit userQuery(&query);
            query.waitForResponse();

            if (query.responseCancelled()) {
                failOperation();
                return;
            }

            setPassword(query.password());

            const QString response(password() + QLatin1Char('\n'));
            writeToProcess(response.toLocal8Bit());

            return;
        }

        if (checkForErrorMessage(line, WrongPasswordPatterns)) {
            kDebug(1601) << "Wrong password!";
            setPassword(QString());
            error(i18n("Incorrect password."));
            failOperation();
            return;
        }

        if (checkForErrorMessage(line, ExtractionFailedPatterns)) {
            kDebug(1601) << "Error in extraction!!";
            emit error(i18n("Extraction failed because of an unexpected error."));
            failOperation();
            return;
        }

        if (handleFileExistsMessage(line)) {
            return;
        }

        if (handleRenameFileMessage(line)) {
            return;
        }
    }

    if (m_operationMode == List) {
        if (checkForPasswordPromptMessage(line)) {
            kDebug(1601) << "Found a password prompt";

            Kerfuffle::PasswordNeededQuery query(filename());
            emit userQuery(&query);
            query.waitForResponse();

            if (query.responseCancelled()) {
                failOperation();
                return;
            }

            setPassword(query.password());

            const QString response(password() + QLatin1Char('\n'));
            writeToProcess(response.toLocal8Bit());

            return;
        }

        if (checkForErrorMessage(line, WrongPasswordPatterns)) {
            kDebug(1601) << "Wrong password!";
            emit error(i18n("Incorrect password."));
            failOperation();
            return;
        }

        if (checkForErrorMessage(line, ExtractionFailedPatterns)) {
            kDebug(1601) << "Error in extraction!!";
            emit error(i18n("Extraction failed because of an unexpected error."));
            failOperation();
            return;
        }

        if (handleFileExistsMessage(line)) {
            return;
        }

        readListLine(line);
        return;
    }

    if (m_operationMode == Test) {

        if (checkForPasswordPromptMessage(line)) {
            kDebug(1601) << "Found a password prompt";

            Kerfuffle::PasswordNeededQuery query(filename());
            userQuery(&query);
            query.waitForResponse();

            if (query.responseCancelled()) {
                error(QString(), QLatin1String("cancelled"));
                failOperation();
                return;
            }

            setPassword(query.password());

            const QString response(password() + QLatin1Char('\n'));
            writeToProcess(response.toLocal8Bit());

            return;
        }

        if (checkForUseCurrentPasswordMessage(line)) {
            if (!m_param.contains(UseCurrentPasswordInput)) {
                error(i18n("Unexpected error: couldn't find input strings."));
                failOperation();
                return;
            }

            Kerfuffle::UseCurrentPasswordQuery query(filename());
            userQuery(&query);
            query.waitForResponse();

            QString response;
            const QStringList choices = m_param.value(UseCurrentPasswordInput).toStringList();

            if (query.responseYes()) {
                response = choices.at(0); // Yes
            } else if (query.responseYesAll()) {
                response = choices.at(2); // Yes, all
            } else {
                response = choices.at(1); // No
            }

            response += QLatin1Char('\n');
            writeToProcess(response.toLocal8Bit());
            return;
        }

        if (checkForErrorMessage(line, TestFailedPatterns) && checkForErrorMessage(line, WrongPasswordPatterns)) {
            m_testResult = false;
            error(i18n("Integrity check failed: Either the archive is broken or the password is incorrect."));
            failOperation();
            return;
        }

        if (checkForErrorMessage(line, TestFailedPatterns)) {
            m_testResult = false;
            error(i18n("Integrity check failed: The archive is broken."));
            failOperation();
            return;
        }

        if (checkForErrorMessage(line, WrongPasswordPatterns)) {
            kDebug(1601) << "Wrong password!";
            error(i18n("Incorrect password."));
            failOperation();
            return;
        }
    }
}

bool CliInterface::checkForUseCurrentPasswordMessage(const QString &line)
{
    if (!m_param.contains(UseCurrentPasswordPattern)) {
        return false;
    }

    if (m_useCurrentPasswordPattern.isEmpty()) {
        m_useCurrentPasswordPattern.setPattern(m_param.value(UseCurrentPasswordPattern).toString());
    }

    if (m_useCurrentPasswordPattern.indexIn(line) != -1) {
        kDebug(1601) << "Detected message whether to use current password again " << m_useCurrentPasswordPattern.cap(0);
        return true;
    }

    return false;
}

bool CliInterface::checkForRenameFileMessage(const QString &line)
{
    if (!m_param.contains(FileExistsNewNamePattern)) {
        return false;
    }

    if (m_renameFilePattern.isEmpty()) {
        m_renameFilePattern.setPattern(m_param.value(FileExistsNewNamePattern).toString());
    }

    if (m_renameFilePattern.indexIn(line) != -1) {
        kDebug(1601) << "Detected file rename message " << m_renameFilePattern.cap(1);
        return true;
    }

    return false;
}

bool CliInterface::handleRenameFileMessage(const QString &line)
{
    if (!checkForRenameFileMessage(line)) {
        return false;
    }

    QVariant var = property("NewFileName");
    if (!var.isValid() || var.toString().isEmpty()) {
        error(i18n("Extraction failed: couldn't set new file name."));
        failOperation();
        return false;
    }

    QString response = var.toString();
    response += QLatin1Char('\n');
    writeToProcess(response.toLocal8Bit());

    setProperty("NewFileName", QVariant()); // a property is cleaned by setting an invalid QVariant

    return true;
}

bool CliInterface::checkForPasswordPromptMessage(const QString& line)
{
    const QString passwordPromptPattern(m_param.value(PasswordPromptPattern).toString());

    if (passwordPromptPattern.isEmpty())
        return false;

    if (m_passwordPromptPattern.isEmpty()) {
        m_passwordPromptPattern.setPattern(m_param.value(PasswordPromptPattern).toString());
    }

    if (m_passwordPromptPattern.indexIn(line) != -1) {
        return true;
    }

    return false;
}

bool CliInterface::checkForFileExistsMessage(const QString& line)
{
    if (m_existsPattern.isEmpty()) {
        m_existsPattern.setPattern(m_param.value(FileExistsExpression).toString());
    }
    if (m_existsPattern.indexIn(line) != -1) {
        kDebug(1601) << "Detected file existing!! Filename " << m_existsPattern.cap(1);
        return true;
    }

    saveLastLine(line);

    return false;
}

QString CliInterface::findFileExistsName()
{
    QString filename = m_existsPattern.cap(1);

    if (filename.isEmpty()) {
        filename = fileExistsName();
        kDebug(1601) << "Detected file existing!! Filename " << filename;
    }
    return filename;
}

bool CliInterface::handleFileExistsMessage(const QString& line)
{
    if (!checkForFileExistsMessage(line)) {
        return false;
    }

    kDebug(1601) << line;
    const QString filename = findFileExistsName();
    QString responseToProcess;
    const QStringList choices = m_param.value(FileExistsInput).toStringList();

    int conflictOption = m_options.value(QLatin1String("ConflictsHandling"), (int)Kerfuffle::AlwaysAsk).toInt();

    if (conflictOption == Kerfuffle::AlwaysAsk) {
        kDebug(1601) << "No auto rename";
        Kerfuffle::OverwriteQuery query(QDir::current().path() + QLatin1Char('/') + filename);
        if (!m_param.contains(SupportsRename)) {
            query.setNoRenameMode(true);
        } else {
            query.setAutoRenameMode(true);
        }
        userQuery(&query);
        kDebug(1601) << "Waiting response";
        query.waitForResponse();
        kDebug(1601) << "Finished response";

        if (query.responseOverwrite()) {
            responseToProcess = choices.at(0);
        } else if (query.responseSkip()) {
            responseToProcess = choices.at(1);
        } else if (query.responseOverwriteAll()) {
            m_options[QLatin1String("ConflictsHandling")] = (int)Kerfuffle::OverwriteAll;
            responseToProcess = choices.at(2);
        } else if (query.responseAutoSkip()) {
            m_options[QLatin1String("ConflictsHandling")] = (int)Kerfuffle::SkipAll;
            responseToProcess = choices.at(3);
        } else if (query.responseCancelled()) {
            if (choices.count() < 5) { // If the program has no way to cancel the extraction, we resort to killing it
                return doKill();
            }
            responseToProcess = choices.at(4);
        } else if (query.responseRename() && choices.count() > 5) {
            setProperty("NewFileName", QVariant(query.newFilename()));
            responseToProcess = choices.at(5);
        } else if (query.responseAutoRenameAll() && choices.count() > 5) {
            m_options[QLatin1String("ConflictsHandling")] = (int)Kerfuffle::RenameAll;
            setProperty("NewFileName", QVariant(query.newFilename()));
            responseToProcess = choices.at(5);
        }
    } else if (conflictOption == Kerfuffle::OverwriteAll)  {
        responseToProcess = choices.at(2);
    } else if (conflictOption == Kerfuffle::SkipAll)  {
        responseToProcess = choices.at(3);
    } else if (conflictOption == Kerfuffle::RenameAll)  {
        QString newName = Kerfuffle::suggestNameForFile(KUrl(QDir::current().path()), filename);
        setProperty("NewFileName", QVariant(newName));
        responseToProcess = choices.at(5);
    }

    Q_ASSERT(!responseToProcess.isEmpty());

    responseToProcess += QLatin1Char('\n');

    writeToProcess(responseToProcess.toLocal8Bit());

    return true;
}

bool CliInterface::checkForErrorMessage(const QString& line, int parameterIndex)
{
    if (!m_param.contains(parameterIndex)) {
        return false;
    }

    foreach(const QString & rawPattern, m_param.value(parameterIndex).toStringList()) {
        if (QRegExp(rawPattern).indexIn(line) != -1) {
            return true;
        }
    }
    return false;
}

bool CliInterface::doKill()
{
    if (m_process) {
        // Give some time for the application to finish gracefully
        if (!m_process->waitForFinished(5)) {
            m_process->kill();
        }

        return true;
    }

    return false;
}

bool CliInterface::doSuspend()
{
    return false;
}

bool CliInterface::doResume()
{
    return false;
}

void CliInterface::substituteListVariables(QStringList& params)
{
    for (int i = 0; i < params.size(); ++i) {
        const QString parameter = params.at(i);

        if (parameter == QLatin1String("$Archive")) {
            params[i] = filename();
        }
    }
}

QString CliInterface::escapeFileName(const QString& fileName) const
{
    return fileName;
}

void CliInterface::writeToProcess(const QByteArray& data)
{
    Q_ASSERT(m_process);
    Q_ASSERT(!data.isNull());

#ifdef Q_OS_WIN
    m_process->write(data);
#else
    m_process->pty()->write(data);
#endif
}

QString CliInterface::autoConvertEncoding(const QString & fileName)
{
    QByteArray result(fileName.toLatin1());

    QTextCodec * currentCodecForLocale = QTextCodec::codecForLocale();
    QTextCodec * currentCodecForCStrings = QTextCodec::codecForCStrings();

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));

    KEncodingProber prober(KEncodingProber::Universal);
    prober.feed(result);
    QByteArray refinedEncoding;

    // fileName is probably in UTF-8 already.
    if (prober.confidence() > 0.49) {
        refinedEncoding = prober.encoding();
    } else {
        prober.setProberType(KEncodingProber::CentralEuropean);
        prober.feed(result);
        refinedEncoding = prober.encoding();
    }

    kDebug(1601) << "KEncodingProber detected encoding: " << refinedEncoding << "( confidence: " << prober.confidence() << ") for: " << fileName;

    if (refinedEncoding == "UTF-8") {
        QTextCodec::setCodecForLocale(currentCodecForLocale);
        QTextCodec::setCodecForCStrings(currentCodecForCStrings);
        return fileName;
    }

    // Workaround for CP850 support (which is frequently attributed to CP1251 by KEncodingProber instead)
    if (refinedEncoding == "windows-1251") {
        kDebug() << "Language: " << KGlobal::locale()->language();
        if (KGlobal::locale()->language() == QLatin1String("de")) {
            // In case the user's language is German we refine the detection of KEncodingProber
            // by assuming that in a german environment the usage of serbian / macedonian letters
            // and special characters is less likely to happen than the usage of umlauts

            kDebug(1601) << "fileName" << fileName;
            kDebug(1601) << "toLatin: " << fileName.toLatin1();
            // Check for case CP850 (Windows XP & Windows7)
            QString checkString = QTextCodec::codecForName("CP850")->toUnicode(fileName.toLatin1());
            kDebug(1601) << "String converted to CP850: " << checkString;
            if (checkString.contains(QLatin1String("ä")) ||  // Equals lower quotation mark in CP1251 - unlikely to be used in filenames
                    checkString.contains(QLatin1String("ö")) || // Equals quotation mark in CP1251 - unlikely to be used in filenames
                    checkString.contains(QLatin1String("Ö")) || // Equals TM symbol  - unlikely to be used in filenames
                    checkString.contains(QLatin1String("ü")) || // Overlaps with "Gje" in the Macedonian alphabet
                    checkString.contains(QLatin1String("Ä")) || // Overlaps with "Tshe" in the Serbian, Bosnian and Montenegrin alphabet
                    checkString.contains(QLatin1String("Ü")) || // Overlaps with "Lje" in the Serbian and Montenegrin alphabet
                    checkString.contains(QLatin1String("ß"))) { // Overlaps with "Be" in the cyrillic alphabet
                refinedEncoding = "CP850";
                kDebug(1601) << "RefinedEncoding: " << refinedEncoding;
            }
        }
    } else if (refinedEncoding == "IBM855") {
        refinedEncoding = "IBM 850";
        kDebug(1601) << "Setting refinedEncoding to " << refinedEncoding;
    } else if (refinedEncoding == "IBM866") {
        refinedEncoding = "IBM 866";
        kDebug(1601) << "Setting refinedEncoding to " << refinedEncoding;
    }

    QTextCodec * codec = QTextCodec::codecForName(refinedEncoding);

    if (!codec) {
        kDebug(1601) << "codecForName returned null, using codec ISO 8859-1 instead";
        codec = QTextCodec::codecForName("ISO 8859-1");
    }

    QString refinedString = codec->toUnicode(fileName.toLatin1());

    QTextCodec::setCodecForLocale(currentCodecForLocale);
    QTextCodec::setCodecForCStrings(currentCodecForCStrings);

    return (refinedString != fileName) ? refinedString : fileName;
}

}

#include "cliinterface.moc"
