/*
    SPDX-FileCopyrightText: 2022 Jiří Wolker <woljiri@gmail.com>
    SPDX-FileCopyrightText: 2022 Eugene Popov <popov895@ukr.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "welcomeview.h"

#include "mainwindow.h"
#include "recentfilesmodel.h"

#include <KAboutData>
#include <KConfigGroup>
#include <KIconLoader>
#include <KRecentFilesMenu>
#include <KSharedConfig>
#include <KIO/OpenFileManagerWindowJob>

#include <QClipboard>
#include <QDesktopServices>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QMenu>
#include <QTimer>

class Placeholder : public QLabel
{
public:
    explicit Placeholder(QWidget *parent = nullptr)
        : QLabel(parent)
    {
        setAlignment(Qt::AlignCenter);
        setMargin(20);
        setWordWrap(true);
        // Match opacity of QML placeholder label component
        QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect;
        opacityEffect->setOpacity(0.5);
        setGraphicsEffect(opacityEffect);
    }
};

WelcomeView::WelcomeView(MainWindow *mainWindow, QWidget *parent)
    : QScrollArea(parent)
    , m_mainWindow(mainWindow)
{
    setupUi(this);

    const KAboutData aboutData = KAboutData::applicationData();
    labelTitle->setText(i18n("Welcome to %1", aboutData.displayName()));
    labelDescription->setText(aboutData.shortDescription());
    labelIcon->setPixmap(aboutData.programLogo().value<QIcon>().pixmap(KIconLoader::SizeEnormous));

    m_placeholderRecentFiles = new Placeholder;
    m_placeholderRecentFiles->setText(i18n("No recent files"));

    QVBoxLayout *layoutPlaceholderRecentFiles = new QVBoxLayout;
    layoutPlaceholderRecentFiles->addWidget(m_placeholderRecentFiles);
    listViewRecentFiles->setLayout(layoutPlaceholderRecentFiles);

    m_recentFilesModel = new RecentFilesModel(this);
    connect(m_recentFilesModel, &RecentFilesModel::modelReset, this, [this]() {
        const bool noRecentFiles = m_recentFilesModel->rowCount() == 0;
        buttonClearRecentFiles->setDisabled(noRecentFiles);
        m_placeholderRecentFiles->setVisible(noRecentFiles);
    });

    KRecentFilesMenu *recentFilesMenu = m_mainWindow->recentFilesMenu();
    m_recentFilesModel->refresh(recentFilesMenu->recentFiles());
    connect(recentFilesMenu, &KRecentFilesMenu::recentFilesChanged, this, [this, recentFilesMenu]() {
        m_recentFilesModel->refresh(recentFilesMenu->recentFiles());
    });

    listViewRecentFiles->setModel(m_recentFilesModel);
    connect(listViewRecentFiles, &QListView::customContextMenuRequested,
            this, &WelcomeView::onRecentFilesContextMenuRequested);
    connect(listViewRecentFiles, &QListView::activated, this, [this](const QModelIndex &index) {
        if (index.isValid()) {
            const QUrl url = m_recentFilesModel->url(index);
            Q_ASSERT(url.isValid());
            m_mainWindow->openUrl(url);
        }
    });

    connect(buttonNewArchive, SIGNAL(clicked()), m_mainWindow, SLOT(newArchive()));
    connect(buttonOpenArchive, SIGNAL(clicked()), m_mainWindow, SLOT(openArchive()));
    connect(buttonClearRecentFiles, &QPushButton::clicked, this, [recentFilesMenu]() {
        recentFilesMenu->clearRecentFiles();
    });

    connect(labelHomepage, qOverload<>(&KUrlLabel::leftClickedUrl), this, [aboutData]() {
        QDesktopServices::openUrl(QUrl(aboutData.homepage()));
    });
    connect(labelHandbook, qOverload<>(&KUrlLabel::leftClickedUrl), this, [this]() {
        m_mainWindow->appHelpActivated();
    });

    connect(buttonClose, &QPushButton::clicked, m_mainWindow, &MainWindow::hideWelcomeScreen);

    static const char showOnStartupKey[] = "ShowWelcomeScreenOnStartup";
    KConfigGroup configGroup = KSharedConfig::openConfig()->group("General");
    checkBoxShowOnStartup->setChecked(configGroup.readEntry(showOnStartupKey, true));
    connect(checkBoxShowOnStartup, &QCheckBox::toggled, this, [configGroup](bool checked) mutable {
        configGroup.writeEntry(showOnStartupKey, checked);
    });

    updateFonts();
    updateButtons();
}

bool WelcomeView::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::FontChange:
        updateFonts();
        updateButtons();
        break;
    case QEvent::Resize:
        if (updateLayout()) {
            return true;
        }
        break;
    default:
        break;
    }

    return QScrollArea::event(event);
}

void WelcomeView::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);

    updateLayout();
}

void WelcomeView::onRecentFilesContextMenuRequested(const QPoint &pos)
{
    const QModelIndex index = listViewRecentFiles->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    const QUrl url = m_recentFilesModel->url(index);
    Q_ASSERT(url.isValid());

    QMenu contextMenu;

    QAction *action = new QAction(i18n("Copy &Location"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy-path")));
    connect(action, &QAction::triggered, this, [url]() {
        qApp->clipboard()->setText(url.toString(QUrl::PreferLocalFile));
    });
    contextMenu.addAction(action);

    action = new QAction(i18n("&Open Containing Folder"));
    action->setEnabled(url.isLocalFile());
    action->setIcon(QIcon::fromTheme(QStringLiteral("document-open-folder")));
    connect(action, &QAction::triggered, this, [url]() {
        KIO::highlightInFileManager({ url });
    });
    contextMenu.addAction(action);

    action = new QAction(i18n("&Remove"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    connect(action, &QAction::triggered, this, [this, url]() {
        KRecentFilesMenu *recentFilesMenu = m_mainWindow->recentFilesMenu();
        recentFilesMenu->removeUrl(url);
        m_recentFilesModel->refresh(recentFilesMenu->recentFiles());
    });
    contextMenu.addAction(action);

    contextMenu.exec(listViewRecentFiles->mapToGlobal(pos));
}

void WelcomeView::updateButtons()
{
    QVector<QPushButton*> buttons {
        buttonNewArchive,
        buttonOpenArchive
    };
    const int maxWidth = std::accumulate(buttons.cbegin(), buttons.cend(), 0, [](int maxWidth, const QPushButton *button) {
        return std::max(maxWidth, button->sizeHint().width());
    });
    for (QPushButton *button : std::as_const(buttons)) {
        button->setFixedWidth(maxWidth);
    }
}

void WelcomeView::updateFonts()
{
    QFont titleFont = font();
    titleFont.setPointSize(titleFont.pointSize() + 6);
    titleFont.setWeight(QFont::Bold);
    labelTitle->setFont(titleFont);

    QFont panelTitleFont = font();
    panelTitleFont.setPointSize(panelTitleFont.pointSize() + 2);
    labelRecentFiles->setFont(panelTitleFont);
    labelHelp->setFont(panelTitleFont);

    QFont placeholderFont = font();
    placeholderFont.setPointSize(qRound(placeholderFont.pointSize() * 1.3));
    m_placeholderRecentFiles->setFont(placeholderFont);
}

bool WelcomeView::updateLayout()
{
    // Align labelHelp with labelRecentFiles
    labelHelp->setMinimumHeight(labelRecentFiles->height());

    bool result = false;

    // show/hide widgetHeader depending on the view height
    if (widgetHeader->isVisible()) {
        if (height() <= frameContent->height() + widgetClose->height()) {
            widgetHeader->hide();
            result = true;
        }
    } else {
        const int implicitHeight = frameContent->height()
                                   + widgetHeader->height()
                                   + layoutContent->spacing()
                                   + widgetClose->height();
        if (height() > implicitHeight) {
            widgetHeader->show();
            result = true;
        }
    }

    // show/hide widgetHelp depending on the view height
    if (widgetHelp->isVisible()) {
        if (width() <= frameContent->width() + widgetClose->width()) {
            widgetHelp->hide();
            result = true;
        }
    } else {
        const int implicitWidth = frameContent->width()
                                  + widgetHelp->width()
                                  + layoutPanels->spacing()
                                  + widgetClose->width();
        if (width() > implicitWidth) {
            widgetHelp->show();
            return true;
        }
    }

    return result;
}
