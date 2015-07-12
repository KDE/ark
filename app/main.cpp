/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (C) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "ark_version.h"
#include "logging.h"
#include "mainwindow.h"
#include "batchextract.h"
#include "kerfuffle/addtoarchive.h"
#include "kdelibs4configmigrator.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QByteArray>
#include <QFileInfo>

#include <KAboutData>
#include <KLocalizedString>
#include <kdbusservice.h>

Q_LOGGING_CATEGORY(ARK, "ark.main", QtWarningMsg)

using Kerfuffle::AddToArchive;

int main(int argc, char **argv)
{
    QApplication application(argc, argv);

    // Debug output can be turned on here:
    //QLoggingCategory::setFilterRules(QStringLiteral("ark.debug = true"));

    Kdelibs4ConfigMigrator migrate(QLatin1Literal("ark"));
    migrate.setConfigFiles(QStringList() << QLatin1Literal("arkrc"));
    migrate.setUiFiles(QStringList() << QLatin1Literal("arkuirc"));
    migrate.migrate();

    KLocalizedString::setApplicationDomain("ark");

    KAboutData aboutData(QStringLiteral("ark"),
                         i18n("Ark"),
                         QStringLiteral(ARK_VERSION_STRING),
                         i18n("KDE Archiving tool"),
                         KAboutLicense::GPL,
                         i18n("(c) 1997-2015, The Various Ark Developers"),
                         QStringLiteral(),
                         QStringLiteral("http://utils.kde.org/projects/ark"),
                         QStringLiteral()
                         );

    aboutData.setOrganizationDomain("kde.org");

    aboutData.addAuthor(i18n("Raphael Kubo da Costa"),
                        i18n("Maintainer"),
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
                        QStringLiteral(),
                        QStringLiteral("Georg.Robbers@urz.uni-hd.de"));
    aboutData.addAuthor(i18n("Roberto Selbach Teixeira"),
                        QStringLiteral(),
                        QStringLiteral("maragato@kde.org"));
    aboutData.addAuthor(i18n("Francois-Xavier Duranceau"),
                        QStringLiteral(),
                        QStringLiteral("duranceau@kde.org"));
    aboutData.addAuthor(i18n("Emily Ezust (Corel Corporation)"),
                        QStringLiteral(),
                        QStringLiteral("emilye@corel.com"));
    aboutData.addAuthor(i18n("Michael Jarrett (Corel Corporation)"),
                        QStringLiteral(),
                        QStringLiteral("michaelj@corel.com"));
    aboutData.addAuthor(i18n("Robert Palmbos"),
                        QStringLiteral(),
                        QStringLiteral("palm9744@kettering.edu"));

    aboutData.addCredit(i18n("Bryce Corkins"),
                        i18n("Icons"),
                        QStringLiteral("dbryce@attglobal.net"));
    aboutData.addCredit(i18n("Liam Smit"),
                        i18n("Ideas, help with the icons"),
                        QStringLiteral("smitty@absamail.co.za"));
    aboutData.addCredit(i18n("Andrew Smith"),
                        i18n("bkisofs code"),
                        QByteArray(),
                        QStringLiteral("http://littlesvr.ca/misc/contactandrew.php"));

    application.setWindowIcon(QIcon::fromTheme(QStringLiteral("ark")));
    application.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(aboutData.shortDescription());
    parser.addHelpOption();
    parser.addVersionOption();

    // url to open
    parser.addPositionalArgument(QStringLiteral("[urls]"), i18n("URLs to open."));

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("d") << QStringLiteral("dialog"),
                                        i18n("Show a dialog for specifying the options for the operation (extract/add)")));

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("o") << QStringLiteral("destination"),
                                        i18n("Destination folder to extract to. Defaults to current path if not specified."),
                                        QStringLiteral("directory")));

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
                                        i18n("Archive contents will be read, and if detected to not be a single folder archive, a subfolder with the name of the archive will be created.")));

    aboutData.setupCommandLine(&parser);
    
    KAboutData::setApplicationData(aboutData);
    
    // do the command line parsing
    parser.process(application);

    // handle standard options
    aboutData.processCommandLine(&parser);

    // This is needed to prevent Dolphin from freezing when opening an archive
    KDBusService dbusService(KDBusService::Multiple);

    //session restoring
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
    } else { //new ark window (no restored session)

        // open any given URLs
        const QStringList urls = parser.positionalArguments();

        if (parser.isSet("add") || parser.isSet("add-to")) {

            AddToArchive *addToArchiveJob = new AddToArchive(&application);
            application.connect(addToArchiveJob, SIGNAL(result(KJob*)), SLOT(quit()), Qt::QueuedConnection);

            if (parser.isSet("changetofirstpath")) {
                qCDebug(ARK) << "Setting changetofirstpath";
                addToArchiveJob->setChangeToFirstPath(true);
            }

            if (parser.isSet("add-to")) {
                qCDebug(ARK) << "Setting filename to" << parser.value("add-to");
                addToArchiveJob->setFilename(QUrl::fromUserInput(parser.value("add-to"), QString(), QUrl::AssumeLocalFile));
            }

            if (parser.isSet("autofilename")) {
                qCDebug(ARK) << "Setting autofilename to" << parser.value("autofilename");
                addToArchiveJob->setAutoFilenameSuffix(parser.value("autofilename"));
            }

            for (int i = 0; i < urls.count(); ++i) {
                //TODO: use the returned value here?
                qCDebug(ARK) << "Adding url" << QUrl::fromUserInput(urls.at(i), QString(), QUrl::AssumeLocalFile);
                addToArchiveJob->addInput(QUrl::fromUserInput(urls.at(i), QString(), QUrl::AssumeLocalFile));
            }

            if (parser.isSet("dialog")) {
                qCDebug(ARK) << "Using kerfuffle to open add dialog";
                if (!addToArchiveJob->showAddDialog()) {
                    return 0;
                }
            }

            addToArchiveJob->start();

        } else if (parser.isSet("batch")) {

            BatchExtract *batchJob = new BatchExtract(&application);
            application.connect(batchJob, SIGNAL(result(KJob*)), SLOT(quit()), Qt::QueuedConnection);

            for (int i = 0; i < urls.count(); ++i) {
                qCDebug(ARK) << "Adding url" << QUrl::fromUserInput(urls.at(i), QString(), QUrl::AssumeLocalFile);
                batchJob->addInput(QUrl::fromUserInput(urls.at(i), QString(), QUrl::AssumeLocalFile));
            }

            if (parser.isSet("autosubfolder")) {
                qCDebug(ARK) << "Setting autosubfolder";
                batchJob->setAutoSubfolder(true);
            }

            if (parser.isSet("autodestination")) {
                QString autopath = QFileInfo(QUrl::fromUserInput(urls.at(0), QString(), QUrl::AssumeLocalFile).path()).path();
                qCDebug(ARK) << "By autodestination, setting path to " << autopath;
                batchJob->setDestinationFolder(autopath);
            }

            if (parser.isSet("destination")) {
                qCDebug(ARK) << "Setting destination to " << parser.value("destination");
                batchJob->setDestinationFolder(parser.value("destination"));
            }

            if (parser.isSet("dialog")) {
                qCDebug(ARK) << "Opening extraction dialog";
                if (!batchJob->showExtractDialog()) {
                    return 0;
                }
            }

            batchJob->start();

        } else {

            MainWindow *window = new MainWindow;
            if (!window->loadPart()) { // if loading the part fails
                delete window;
                return -1;
            }

            if (urls.count()) {
                qCDebug(ARK) << "Trying to open" << QUrl::fromUserInput(urls.at(0), QString(), QUrl::AssumeLocalFile);

                if (parser.isSet("dialog")) {
                    window->setShowExtractDialog(true);
                }
                window->openUrl(QUrl::fromUserInput(urls.at(0), QString(), QUrl::AssumeLocalFile));
            }
            window->show();
        }
    }

    qCDebug(ARK) << "Entering application loop";
    return application.exec();
}
