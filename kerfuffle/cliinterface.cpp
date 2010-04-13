/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
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

#include <KProcess>
#include <KStandardDirs>
#include <KDebug>
#include <KLocale>

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QThread>
#include <QTimer>

namespace Kerfuffle
{
CliInterface::CliInterface(QObject *parent, const QVariantList & args)
        : ReadWriteArchiveInterface(parent, args),
        m_process(0)
{
    //because this interface uses the event loop
    setWaitForFinishedSignal(true);
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
    delete m_process;
    m_process = 0;
}

bool CliInterface::list()
{
    cacheParameterList();
    m_operationMode = List;

    if (!findProgramAndCreateProcess(m_param.value(ListProgram).toString())) {
        failOperation();
        return false;
    }

    QStringList args = m_param.value(ListArgs).toStringList();
    substituteListVariables(args);

    executeProcess(m_program, args);

    return true;
}

bool CliInterface::copyFiles(const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options)
{
    kDebug() ;
    cacheParameterList();

    m_operationMode = Copy;

    if (!findProgramAndCreateProcess(m_param.value(ExtractProgram).toString())) {
        failOperation();
        return false;
    }

    //start preparing the argument list
    QStringList args = m_param.value(ExtractArgs).toStringList();

    //now replace the various elements in the list
    for (int i = 0; i < args.size(); ++i) {
        QString argument = args.at(i);
        kDebug() << "Processing argument " << argument;

        if (argument == "$Archive") {
            args[i] = filename();
        }

        if (argument == "$PreservePathSwitch") {

            QStringList replacementFlags = m_param.value(PreservePathSwitch).toStringList();
            Q_ASSERT(replacementFlags.size() == 2);

            bool preservePaths = options.value("PreservePaths").toBool();
            QString theReplacement;
            if (preservePaths)
                theReplacement = replacementFlags.at(0);
            else
                theReplacement = replacementFlags.at(1);

            if (theReplacement.isEmpty()) {
                args.removeAt(i);
                --i; //decrement to compensate for the variable we removed
            } else
                //but in this case we don't have to decrement, we just
                //replace it
                args[i] = theReplacement;
        }

        if (argument == "$PasswordSwitch") {
            //if the PasswordSwitch argument has been added, we at least
            //assume that the format of the switch has been added as well
            Q_ASSERT(m_param.contains(PasswordSwitch));

            //we will decrement i afterwards
            args.removeAt(i);

            //if we get a hint about this being a password protected archive, ask about
            //the password in advance.
            if (options.value("PasswordProtectedHint").toBool() && password().isEmpty()) {
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
                    newArg.replace("$Password", pass);

                    //put it in the arg list
                    args.insert(i + j, newArg);
                    ++i;

                }
            }
            --i; //decrement to compensate for the variable we replaced
        }

        if (argument == "$RootNodeSwitch") {
            //if the RootNodeSwitch argument has been added, we at least
            //assume that the format of the switch has been added as well
            Q_ASSERT(m_param.contains(RootNodeSwitch));

            //we will decrement i afterwards
            args.removeAt(i);

            QString rootNode;
            if (options.contains("RootNode")) {
                rootNode = options.value("RootNode").toString();
                kDebug() << "Set root node " << rootNode;
            }

            if (!rootNode.isEmpty()) {
                QStringList theSwitch = m_param.value(RootNodeSwitch).toStringList();
                for (int j = 0; j < theSwitch.size(); ++j) {
                    //get the argument part
                    QString newArg = theSwitch.at(j);

                    //substitute the $Path
                    newArg.replace("$Path", rootNode);

                    //put it in the arg list
                    args.insert(i + j, newArg);
                    ++i;

                }
            }
            --i; //decrement to compensate for the variable we replaced
        }

        if (argument == "$Files") {
            args.removeAt(i);
            for (int j = 0; j < files.count(); ++j) {
                args.insert(i + j, files.at(j).toString());
                ++i;
            }
            --i;
        }
    }

    kDebug() << "Setting current dir to " << destinationDirectory;
    QDir::setCurrent(destinationDirectory);

    executeProcess(m_program, args);

    return true;
}

bool CliInterface::addFiles(const QStringList & files, const CompressionOptions& options)
{
    cacheParameterList();

    m_operationMode = Add;

    if (!findProgramAndCreateProcess(m_param.value(AddProgram).toString())) {
        failOperation();
        return false;
    }

    const QString globalWorkDir = options.value("GlobalWorkDir").toString();
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

        if (argument == "$Archive") {
            args[i] = filename();
        }

        if (argument == "$Files") {
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

    executeProcess(m_program, args);

    return true;
}

bool CliInterface::deleteFiles(const QList<QVariant> & files)
{
    cacheParameterList();
    m_operationMode = Delete;

    if (!findProgramAndCreateProcess(m_param.value(DeleteProgram).toString())) {
        failOperation();
        return false;
    }

    //start preparing the argument list
    QStringList args = m_param.value(DeleteArgs).toStringList();

    //now replace the various elements in the list
    for (int i = 0; i < args.size(); ++i) {
        QString argument = args.at(i);
        kDebug() << "Processing argument " << argument;

        if (argument == "$Archive") {
            args[i] = filename();
        }

        if (argument == "$Files") {
            args.removeAt(i);
            for (int j = 0; j < files.count(); ++j) {

                //QString relativeName = QDir::current().relativeFilePath(files.at(j));

                args.insert(i + j, files.at(j).toString());
                ++i;
            }
            --i;
        }
    }

    m_removedFiles = files;

    executeProcess(m_program, args);

    return true;
}

bool CliInterface::createProcess()
{
    kDebug();

    if (m_process) {
        delete m_process;
        m_process = 0;
    }

    m_process = new KProcess();
    m_stdOutData.clear();
    m_process->setOutputChannelMode(KProcess::MergedChannels);

    if (QMetaType::type("QProcess::ExitStatus") == 0)
        qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");

    connect(m_process, SIGNAL(started()), SLOT(started()), Qt::DirectConnection);
    connect(m_process, SIGNAL(readyReadStandardOutput()), SLOT(readStdout()), Qt::DirectConnection);
    connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(processFinished(int, QProcess::ExitStatus)), Qt::DirectConnection);

    return true;
}

bool CliInterface::executeProcess(const QString& path, const QStringList & args)
{
    kDebug() << "Executing " << path << args;
    Q_ASSERT(!path.isEmpty());

    m_process->setProgram(path, args);
    m_process->setNextOpenMode(QIODevice::ReadWrite | QIODevice::Unbuffered);
    m_process->start();

    return true;
}

void CliInterface::started()
{
    //m_state = 0;
    m_userCancelled = false;
}

void CliInterface::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)

    kDebug();

    //if the m_process pointer is gone, then there is nothing to worry
    //about here
    if (!m_process)
        return;

    if (m_operationMode == Delete) {
        foreach(const QVariant& v, m_removedFiles) {
            entryRemoved(v.toString());
        }
    }

    //handle all the remaining data in the process
    readStdout(true);

    progress(1.0);

    if (m_operationMode == Add) {
        list();
        return;
    }

    //and we're finished
    finished(true);
}

void CliInterface::failOperation()
{
    kDebug();

    if (m_process)
        m_process->terminate();

    finished(false);
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

    //for simplicity, we replace all carriage return characters to newlines
    dd.replace('\015', '\n');

    //same thing with backspaces.
    //TODO: whether this is a safe assumption or not needs to be
    //determined
    dd.replace('\010', '\n');

    m_stdOutData += dd;

    QList<QByteArray> lines = m_stdOutData.split('\n');

    //The reason for this check is that archivers often do not end
    //queries (such as file exists, wrong password) on a new line, but
    //freeze waiting for input. So we check for errors on the last line in
    //all cases.
    bool foundErrorMessage =
        (checkForErrorMessage(lines.last(), WrongPasswordPatterns) ||
         checkForErrorMessage(lines.last(), ExtractionFailedPatterns) ||
         checkForFileExistsMessage(lines.last()));

    if (foundErrorMessage)
        handleAll = true;

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

    foreach(const QByteArray& line, lines) {
        if (!line.isEmpty())
            handleLine(QString::fromLocal8Bit(line));
    }
}

void CliInterface::handleLine(const QString& line)
{
    if ((m_operationMode == Copy || m_operationMode == Add) && m_param.contains(CaptureProgress) && m_param.value(CaptureProgress).toBool()) {
        //read the percentage
        int pos = line.indexOf('%');
        if (pos != -1 && pos > 1) {
            int percentage = line.mid(pos - 2, 2).toInt();
            progress(float(percentage) / 100);
            return;
        }
    }

    if (m_operationMode == Copy) {
        if (checkForErrorMessage(line, WrongPasswordPatterns)) {
            kDebug() << "Wrong password!";
            error(i18n("Incorrect password."));
            setPassword(QString());
            failOperation();
            return;
        }

        if (checkForErrorMessage(line, ExtractionFailedPatterns)) {
            kDebug() << "Error in extraction!!";
            error(i18n("Extraction failed because of an unexpected error."));
            failOperation();
            return;
        }

        if (handleFileExistsMessage(line))
            return;
    }

    if (m_operationMode == List) {
        if (checkForErrorMessage(line, WrongPasswordPatterns)) {
            kDebug() << "Wrong password!";
            error(i18n("Incorrect password."));
            setPassword(QString());
            failOperation();
            return;
        }

        if (checkForErrorMessage(line, ExtractionFailedPatterns)) {
            kDebug() << "Error in extraction!!";
            error(i18n("Extraction failed because of an unexpected error."));
            failOperation();
            return;
        }

        if (handleFileExistsMessage(line))
            return;

        readListLine(line);
        return;
    }
}

bool CliInterface::findProgramAndCreateProcess(const QString& program)
{
    m_program = KStandardDirs::findExe(program);

    if (m_program.isEmpty()) {
        error(i18n("Failed to locate program '%1' in PATH.", program));
        return false;
    }

    if (!createProcess()) {
        error(i18n("Found program '%1', but failed to initialize the process.", program));
        return false;
    }

    return true;
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
    if (!checkForFileExistsMessage(line))
        return false;

    const QString filename = m_existsPattern.cap(1);

    Kerfuffle::OverwriteQuery query(QDir::current().path() + '/' + filename);
    query.setNoRenameMode(true);
    userQuery(&query);
    kDebug() << "Waiting response";
    query.waitForResponse();

    kDebug() << "Finished response";

    QString responseToProcess;
    const QStringList choices = m_param.value(FileExistsInput).toStringList();

    if (query.responseOverwrite())
        responseToProcess = choices.at(0);
    else if (query.responseSkip())
        responseToProcess = choices.at(1);
    else if (query.responseOverwriteAll())
        responseToProcess = choices.at(2);
    else if (query.responseAutoSkip())
        responseToProcess = choices.at(3);
    else if (query.responseCancelled())
        responseToProcess = choices.at(4);

    Q_ASSERT(!responseToProcess.isEmpty());

    responseToProcess += '\n';

    kDebug() << "Writing " << responseToProcess;

    m_process->write(responseToProcess.toLocal8Bit());

    return true;
}

bool CliInterface::checkForErrorMessage(const QString& line, int parameterIndex)
{
    static QHash<int, QList<QRegExp> > patternCache;
    QList<QRegExp> patterns;

    if (patternCache.contains(parameterIndex)) {
        patterns = patternCache.value(parameterIndex);
    } else {
        if (!m_param.contains(parameterIndex))
            return false;

        foreach(const QString& rawPattern, m_param.value(parameterIndex).toStringList()) {
            patterns << QRegExp(rawPattern);
        }
        patternCache[parameterIndex] = patterns;
    }

    foreach(const QRegExp& pattern, patterns) {
        if (pattern.indexIn(line) != -1) {
            return true;
        }
    }
    return false;
}

bool CliInterface::doKill()
{
    if (m_process) {
        m_process->terminate();

        if (!m_process->waitForFinished())
            m_process->kill();

        m_process->waitForFinished();

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

        if (parameter == "$Archive") {
            params[i] = filename();
        }
    }
}
}

#include "cliinterface.moc"
