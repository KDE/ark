/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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
      m_testResult(true)
{
    //because this interface uses the event loop
    setWaitForFinishedSignal(true);

    if (QMetaType::type("QProcess::ExitStatus") == 0) {
        qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
    }
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

bool CliInterface::list()
{
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
    kDebug();
    cacheParameterList();

    m_operationMode = Copy;

    //start preparing the argument list
    QStringList args = m_param.value(ExtractArgs).toStringList();

    //now replace the various elements in the list
    for (int i = 0; i < args.size(); ++i) {
        QString argument = args.at(i);
        kDebug() << "Processing argument " << argument;

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
                kDebug() << "Password hint enabled, querying user";

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
                kDebug() << "Set root node " << rootNode;
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

        if (argument == QLatin1String("$Files")) {
            args.removeAt(i);
            for (int j = 0; j < files.count(); ++j) {
                args.insert(i + j, escapeFileName(files.at(j).toString()));
                ++i;
            }
            --i;
        }
    }

    kDebug() << "Setting current dir to " << destinationDirectory;
    QDir::setCurrent(destinationDirectory);

    if (!runProcess(m_param.value(ExtractProgram).toStringList(), args)) {
        failOperation();
        return false;
    }

    return true;
}

bool CliInterface::addFiles(const QStringList & files, const CompressionOptions& options)
{
    cacheParameterList();

    m_operationMode = Add;

    const QString globalWorkDir = options.value(QLatin1String("GlobalWorkDir")).toString();
    const QDir workDir = globalWorkDir.isEmpty() ? QDir::current() : QDir(globalWorkDir);
    if (!globalWorkDir.isEmpty()) {
        kDebug() << "GlobalWorkDir is set, changing dir to " << globalWorkDir;
        QDir::setCurrent(globalWorkDir);
    }

    //start preparing the argument list
    QStringList args = m_param.value(AddArgs).toStringList();

    //now replace the various elements in the list
    for (int i = 0; i < args.size(); ++i) {
        const QString argument = args.at(i);
        kDebug() << "Processing argument " << argument;

        if (argument == QLatin1String("$CompressionLevelSwitch")) {
            QStringList compressionLevelSwitches = m_param.value(CompressionLevelSwitches).toStringList();
            QString theReplacement = compressionLevelSwitches.at(options.value(QLatin1String("CompressionLevel"), 2).toInt());

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
                kDebug() << "Password hint enabled, querying user";

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

        if (argument == QLatin1String("$EncryptHeaderSwitch")) {
            QString encryptHeaderSwitch = m_param.value(EncryptHeaderSwitch).toString();
            bool encryptHeader = options.value(QLatin1String("EncryptHeaderEnabled")).toBool();

            QString theReplacement;
            if (encryptHeader == true) {
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
            QString encryptionMethod = options.value(QLatin1String("EncryptionMethod")).toString();

            QString theReplacement;
            if (encryptionMethod == QLatin1String("AES256")) {
                theReplacement = encryptionMethodSwitches.at(0);
            }
            if (encryptionMethod == QLatin1String("ZipCrypto")) {
                theReplacement = encryptionMethodSwitches.at(1);
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
        kDebug() << "Processing argument " << argument;

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
    kDebug();
    cacheParameterList();

    m_testResult = true;
    m_operationMode = Test;

    //start preparing the argument list
    QStringList args = m_param.value(TestArgs).toStringList();

    //now replace the various elements in the list
    for (int i = 0; i < args.size(); ++i) {
        QString argument = args.at(i);
        kDebug() << "Processing argument " << argument;

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
                kDebug() << "Password hint enabled, querying user";

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

    kDebug() << "Executing" << programPath << arguments;

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
    connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(processFinished(int, QProcess::ExitStatus)), Qt::DirectConnection);

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

void CliInterface::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    kDebug() << exitCode << exitStatus;

    //if the m_process pointer is gone, then there is nothing to worry
    //about here
    if (!m_process) {
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

    //and we're finished
    emit finished(true);
}

void CliInterface::failOperation()
{
    // TODO: Would be good to unit test #304764/#304178.
    kDebug();
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
         checkForFileExistsMessage(QLatin1String(lines.last())));

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
        if (!line.isEmpty()) {
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

    if (m_operationMode == Copy) {
        if (checkForPasswordPromptMessage(line)) {
            kDebug() << "Found a password prompt";

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
            kDebug() << "Wrong password!";
            setPassword(QString());
            error(i18n("Incorrect password."));
            failOperation();
            return;
        }

        if (checkForErrorMessage(line, ExtractionFailedPatterns)) {
            kDebug() << "Error in extraction!!";
            emit error(i18n("Extraction failed because of an unexpected error."));
            failOperation();
            return;
        }

        if (handleFileExistsMessage(line)) {
            return;
        }
    }

    if (m_operationMode == List) {
        if (checkForPasswordPromptMessage(line)) {
            kDebug() << "Found a password prompt";

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
            kDebug() << "Wrong password!";
            emit error(i18n("Incorrect password."));
            failOperation();
            return;
        }

        if (checkForErrorMessage(line, ExtractionFailedPatterns)) {
            kDebug() << "Error in extraction!!";
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
            kDebug() << "Found a password prompt";

            Kerfuffle::PasswordNeededQuery query(filename());
            userQuery(&query);
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
            kDebug() << "Wrong password!";
            error(i18n("Incorrect password."));
            failOperation();
            return;
        }
    }
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
        kDebug() << "Detected file existing!! Filename " << m_existsPattern.cap(1);
        return true;
    }

    return false;
}

bool CliInterface::handleFileExistsMessage(const QString& line)
{
    if (!checkForFileExistsMessage(line)) {
        return false;
    }

    const QString filename = m_existsPattern.cap(1);

    Kerfuffle::OverwriteQuery query(QDir::current().path() + QLatin1Char('/') + filename);
    query.setNoRenameMode(true);
    emit userQuery(&query);
    kDebug() << "Waiting response";
    query.waitForResponse();

    kDebug() << "Finished response";

    QString responseToProcess;
    const QStringList choices = m_param.value(FileExistsInput).toStringList();

    if (query.responseOverwrite()) {
        responseToProcess = choices.at(0);
    } else if (query.responseSkip()) {
        responseToProcess = choices.at(1);
    } else if (query.responseOverwriteAll()) {
        responseToProcess = choices.at(2);
    } else if (query.responseAutoSkip()) {
        responseToProcess = choices.at(3);
    } else if (query.responseCancelled()) {
        if (choices.count() < 5) { // If the program has no way to cancel the extraction, we resort to killing it
            return doKill();
        }
        responseToProcess = choices.at(4);
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

    kDebug() << "Writing" << data << "to the process";

#ifdef Q_OS_WIN
    m_process->write(data);
#else
    m_process->pty()->write(data);
#endif
}

QString CliInterface::autoConvertEncoding( const QString & fileName )
{
    QByteArray result( fileName.toLatin1() );

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));

    KEncodingProber prober(KEncodingProber::CentralEuropean);
    KEncodingProber prober2(KEncodingProber::Universal);
    prober.feed(result);
    prober2.feed(result);
    QByteArray refinedEncoding;

    // fileName is probably in UTF-8 already.
    if (prober2.confidence() > 0.49) {
        refinedEncoding = prober2.encoding();
    } else {
        refinedEncoding = prober.encoding();
    }

    kDebug() << "KEncodingProber detected encodings: " << refinedEncoding << "for: " << fileName;

    // Workaround for CP850 support (which is frequently attributed to CP1251 by KEncodingProber instead)
    if (refinedEncoding == "windows-1251") {
        kDebug() << "Language: " << KGlobal::locale()->language();
        if ( KGlobal::locale()->language() == QLatin1String("de") ) {
            // In case the user's language is German we refine the detection of KEncodingProber
            // by assuming that in a german environment the usage of serbian / macedonian letters
            // and special characters is less likely to happen than the usage of umlauts

            kDebug() << "fileName" << fileName;
            kDebug() << "toLatin: " << fileName.toLatin1();
            // Check for case CP850 (Windows XP & Windows7)
            QString checkString = QTextCodec::codecForName("CP850")->toUnicode(fileName.toLatin1());
            kDebug() << "String converted to CP850: " << checkString;
            if ( checkString.contains(QLatin1String("ä")) || // Equals lower quotation mark in CP1251 - unlikely to be used in filenames
                 checkString.contains(QLatin1String("ö")) || // Equals quotation mark in CP1251 - unlikely to be used in filenames
                 checkString.contains(QLatin1String("Ö")) || // Equals TM symbol  - unlikely to be used in filenames
                 checkString.contains(QLatin1String("ü")) || // Overlaps with "Gje" in the Macedonian alphabet
                 checkString.contains(QLatin1String("Ä")) || // Overlaps with "Tshe" in the Serbian, Bosnian and Montenegrin alphabet
                 checkString.contains(QLatin1String("Ü")) || // Overlaps with "Lje" in the Serbian and Montenegrin alphabet
                 checkString.contains(QLatin1String("ß")) )  // Overlaps with "Be" in the cyrillic alphabet
            {
                refinedEncoding = "CP850";
                kDebug() << "RefinedEncoding: " << refinedEncoding;
            }
        }
    } else if (refinedEncoding == "IBM855") {
        refinedEncoding = "IBM 850";
        kDebug() << "Setting refinedEncoding to " << refinedEncoding;
    } else if (refinedEncoding == "IBM866") {
        refinedEncoding = "IBM 866";
        kDebug() << "Setting refinedEncoding to " << refinedEncoding;
    }

    QTextCodec * codec = QTextCodec::codecForName(refinedEncoding);

    if (!codec) {
        kDebug() << "codecForName returned null, using codec ISO-8859-1 instead";
        codec = QTextCodec::codecForName("ISO-8859-1");
    }

    QString refinedString = codec->toUnicode(fileName.toLatin1());

    return ( refinedString != fileName ) ? refinedString : fileName;
}

}

#include "cliinterface.moc"
