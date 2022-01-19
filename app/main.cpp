/*
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>
    SPDX-FileCopyrightText: 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2015-2017 Ragnar Thomsen <rthomsen6@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ark_version.h"
#include "ark_debug.h"
#include "mainwindow.h"
#include "batchextract.h"
#include "addtoarchive.h"
#include "pluginmanager.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QFileOpenEvent>

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <Kdelibs4ConfigMigrator>
#endif
#include <KLocalizedString>

#include <iostream>

using Kerfuffle::AddToArchive;

class OpenFileEventHandler : public QObject
{
    Q_OBJECT
public:
    OpenFileEventHandler(QApplication *parent, MainWindow *w)
        : QObject(parent)
        , m_window(w)
    {
        parent->installEventFilter(this);
    }

    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (event->type() == QEvent::FileOpen) {
            QFileOpenEvent *openEvent = static_cast<QFileOpenEvent*>(event);
            qCDebug(ARK) << "File open event:" << openEvent->url() << "for window" << m_window;
            m_window->openUrl(openEvent->url());
            return true;
        }
        return QObject::eventFilter(obj, event);
    }
private:
    MainWindow *m_window;
};

int main(int argc, char **argv)
{
    /**
     * enable high dpi support
     */
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts); // Required for the webengine part.
    QApplication application(argc, argv);

    KCrash::initialize();

    // Debug output can be turned on here:
    //QLoggingCategory::setFilterRules(QStringLiteral("ark.debug = true"));
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    Kdelibs4ConfigMigrator migrate(QStringLiteral("ark"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("arkrc"));
    migrate.setUiFiles(QStringList() << QStringLiteral("arkuirc"));
    migrate.migrate();
#endif

    KLocalizedString::setApplicationDomain("ark");

    KAboutData aboutData(QStringLiteral("ark"),
                         i18n("Ark"),
                         QStringLiteral(ARK_VERSION_STRING),
                         i18n("KDE Archiving tool"),
                         KAboutLicense::GPL,
                         i18n("(c) 1997-2019, The Ark Developers"),
                         QString(),
                         QStringLiteral("https://utils.kde.org/projects/ark")
    );

    aboutData.setOrganizationDomain("kde.org");

    aboutData.addAuthor(i18n("Elvis Angelaccio"),
                        i18n("Maintainer"),
                        QStringLiteral("elvis.angelaccio@kde.org"));
    aboutData.addAuthor(i18n("Ragnar Thomsen"),
                        i18n("Maintainer, KF5 port"),
                        QStringLiteral("rthomsen6@gmail.com"));
    aboutData.addAuthor(i18n("Raphael Kubo da Costa"),
                        i18n("Former Maintainer"),
                        QStringLiteral("rakuco@FreeBSD.org"));
    aboutData.addAuthor(i18n("Harald Hvaal"),
                        i18n("Former Maintainer"),
                        QStringLiteral("haraldhv@stud.ntnu.no"));
    aboutData.addAuthor(i18n("Henrique Pinto"),
                        i18n("Former Maintainer"),
                        QStringLiteral("henrique.pinto@kdemail.net"));
    aboutData.addAuthor(i18n("Helio Chissini de Castro"),
                        i18n("Former maintainer"),
                        QStringLiteral("helio@kde.org"));
    aboutData.addAuthor(i18n("Georg Robbers"),
                        QString(),
                        QStringLiteral("Georg.Robbers@urz.uni-hd.de"));
    aboutData.addAuthor(i18n("Roberto Selbach Teixeira"),
                        QString(),
                        QStringLiteral("maragato@kde.org"));
    aboutData.addAuthor(i18n("Francois-Xavier Duranceau"),
                        QString(),
                        QStringLiteral("duranceau@kde.org"));
    aboutData.addAuthor(i18n("Emily Ezust (Corel Corporation)"),
                        QString(),
                        QStringLiteral("emilye@corel.com"));
    aboutData.addAuthor(i18n("Michael Jarrett (Corel Corporation)"),
                        QString(),
                        QStringLiteral("michaelj@corel.com"));
    aboutData.addAuthor(i18n("Robert Palmbos"),
                        QString(),
                        QStringLiteral("palm9744@kettering.edu"));

    aboutData.addCredit(i18n("Vladyslav Batyrenko"),
                        i18n("Advanced editing functionalities"),
                        QString(),
                        QStringLiteral("https://mvlabat.github.io/ark-gsoc-2016/"));
    aboutData.addCredit(i18n("Bryce Corkins"),
                        i18n("Icons"),
                        QStringLiteral("dbryce@attglobal.net"));
    aboutData.addCredit(i18n("Liam Smit"),
                        i18n("Ideas, help with the icons"),
                        QStringLiteral("smitty@absamail.co.za"));
    aboutData.addCredit(i18n("Andrew Smith"),
                        i18n("bkisofs code"),
                        QString(),
                        QStringLiteral("http://littlesvr.ca/misc/contactandrew.php"));

    KAboutData::setApplicationData(aboutData);
    application.setWindowIcon(QIcon::fromTheme(QStringLiteral("ark"), application.windowIcon()));

    QCommandLineParser parser;

    // Url to open.
    parser.addPositionalArgument(QStringLiteral("[urls]"), i18n("URLs to open."));

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("d") << QStringLiteral("dialog"),
                                        i18n("Show a dialog for specifying the options for the operation (extract/add)")));

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("o") << QStringLiteral("destination"),
                                        i18n("Destination folder to extract to. Defaults to current path if not specified."),
                                        QStringLiteral("directory")));

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("O") << QStringLiteral("opendestination"),
                                        i18n("Open destination folder after extraction.")));

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("c") << QStringLiteral("add"),
                                        i18n("Query the user for an archive filename and add specified files to it. Quit when finished.")));

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("t") << QStringLiteral("add-to"),
                                        i18n("Add the specified files to 'filename'. Create archive if it does not exist. Quit when finished."),
                                        QStringLiteral("filename")));

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("p") << QStringLiteral("changetofirstpath"),
                                        i18n("Change the current dir to the first entry and add all other entries relative to this one.")));

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("f") << QStringLiteral("autofilename"),
                                        i18n("Automatically choose a filename, with the selected suffix (for example rar, tar.gz, zip or any other supported types)"),
                                        QStringLiteral("suffix")));

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("b") << QStringLiteral("batch"),
                                        i18n("Use the batch interface instead of the usual dialog. This option is implied if more than one url is specified.")));

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("e") << QStringLiteral("autodestination"),
                                        i18n("The destination argument will be set to the path of the first file supplied.")));

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("a") << QStringLiteral("autosubfolder"),
                                        i18n("Archive contents will be read, and if detected to not be a single folder or a single file archive, a subfolder with the name of the archive will be created.")));

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("m") << QStringLiteral("mimetypes"),
                                        i18n("List supported MIME types.")));

    aboutData.setupCommandLine(&parser);

    // Do the command line parsing.
    parser.process(application);

    // Handle standard options.
    aboutData.processCommandLine(&parser);

    // This is needed to prevent Dolphin from freezing when opening an archive.
    KDBusService dbusService(KDBusService::Multiple | KDBusService::NoExitOnFailure);

    // Session restoring.
    if (application.isSessionRestored()) {
        if (!KMainWindow::canBeRestored(1)) {
            return -1;
        }

        MainWindow* window = new MainWindow;
        window->restore(1);
        if (!window->loadPart()) {
            delete window;
            return -1;
        }
    } else { // New ark window (no restored session).

        // Open any given URLs.
        const QStringList urls = parser.positionalArguments();

        if (parser.isSet(QStringLiteral("add")) ||
            parser.isSet(QStringLiteral("add-to"))) {

            if (urls.isEmpty()) {
                std::cout << "Missing arguments: urls." << std::endl;
                parser.showHelp(-1);
            }

            AddToArchive *addToArchiveJob = new AddToArchive(&application);
            application.setQuitOnLastWindowClosed(false);
            QObject::connect(addToArchiveJob, &KJob::result, &application, &QCoreApplication::quit, Qt::QueuedConnection);

            if (parser.isSet(QStringLiteral("changetofirstpath"))) {
                qCDebug(ARK) << "Setting changetofirstpath";
                addToArchiveJob->setChangeToFirstPath(true);
            }

            if (parser.isSet(QStringLiteral("add-to"))) {
                qCDebug(ARK) << "Setting filename to" << parser.value(QStringLiteral("add-to"));
                addToArchiveJob->setFilename(QUrl::fromUserInput(parser.value(QStringLiteral("add-to")),
                                                                 QString(),
                                                                 QUrl::AssumeLocalFile));
            }

            if (parser.isSet(QStringLiteral("autofilename"))) {
                qCDebug(ARK) << "Setting autofilename to" << parser.value(QStringLiteral("autofilename"));
                addToArchiveJob->setAutoFilenameSuffix(parser.value(QStringLiteral("autofilename")));
            }

            for (int i = 0; i < urls.count(); ++i) {
                //TODO: use the returned value here?
                qCDebug(ARK) << "Adding url" << QUrl::fromUserInput(urls.at(i), QString(), QUrl::AssumeLocalFile);
                addToArchiveJob->addInput(QUrl::fromUserInput(urls.at(i), QString(), QUrl::AssumeLocalFile));
            }

            if (parser.isSet(QStringLiteral("dialog"))) {
                qCDebug(ARK) << "Using kerfuffle to open add dialog";
                if (!addToArchiveJob->showAddDialog()) {
                    return 0;
                }
            }

            addToArchiveJob->start();

        } else if (parser.isSet(QStringLiteral("batch"))) {

            if (urls.isEmpty()) {
                qCDebug(ARK) << "No urls to be extracted were provided.";
                parser.showHelp(-1);
            }

            BatchExtract *batchJob = new BatchExtract(&application);
            application.setQuitOnLastWindowClosed(false);
            QObject::connect(batchJob, &KJob::result, &application, &QCoreApplication::quit, Qt::QueuedConnection);

            for (int i = 0; i < urls.count(); ++i) {
                qCDebug(ARK) << "Adding url" << QUrl::fromUserInput(urls.at(i), QString(), QUrl::AssumeLocalFile);
                batchJob->addInput(QUrl::fromUserInput(urls.at(i), QString(), QUrl::AssumeLocalFile));
            }

            if (parser.isSet(QStringLiteral("autosubfolder"))) {
                qCDebug(ARK) << "Setting autosubfolder";
                batchJob->setAutoSubfolder(true);
            }

            if (parser.isSet(QStringLiteral("autodestination"))) {
                QString autopath = QFileInfo(QUrl::fromUserInput(urls.at(0), QString(), QUrl::AssumeLocalFile).path()).path();
                qCDebug(ARK) << "By autodestination, setting path to " << autopath;
                batchJob->setDestinationFolder(autopath);
            }

            if (parser.isSet(QStringLiteral("destination"))) {
                qCDebug(ARK) << "Setting destination to " << parser.value(QStringLiteral("destination"));
                batchJob->setDestinationFolder(parser.value(QStringLiteral("destination")));
            }

            if (parser.isSet(QStringLiteral("opendestination"))) {
                qCDebug(ARK) << "Setting opendestination";
                batchJob->setOpenDestinationAfterExtraction(true);
            }

            if (parser.isSet(QStringLiteral("dialog"))) {
                qCDebug(ARK) << "Opening extraction dialog";
                if (!batchJob->showExtractDialog()) {
                    return 0;
                }
            }

            batchJob->start();

        } else if (parser.isSet(QStringLiteral("mimetypes"))) {

            Kerfuffle::PluginManager pluginManager;
            const auto mimeTypes = pluginManager.supportedMimeTypes();
            QTextStream cout(stdout);
            for (const auto &mimeType : mimeTypes) {
                cout << mimeType << '\n';
            }
            return 0;

        } else {

            MainWindow *window = new MainWindow;
            if (!window->loadPart()) { // if loading the part fails
                delete window;
                return -1;
            }

            if (!urls.isEmpty()) {
                qCDebug(ARK) << "Trying to open" << QUrl::fromUserInput(urls.at(0), QString(), QUrl::AssumeLocalFile);

                if (parser.isSet(QStringLiteral("dialog"))) {
                    window->setShowExtractDialog(true);
                }
                window->openUrl(QUrl::fromUserInput(urls.at(0), QString(), QUrl::AssumeLocalFile));
            }
            new OpenFileEventHandler(&application, window);
            window->show();
        }
    }

    qCDebug(ARK) << "Entering application loop";
    return application.exec();
}

#include "main.moc"
