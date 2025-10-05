/*
===============================================================================
    Copyright (C) 2025 Ilya Lyakhovets

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
===============================================================================
*/

#include "Application.h"
#include "MainWindow.h"
#include "UnhidableMenu.h"
#include "Common.h"
#include "ui_MainWindow.h"
#include <QStringListModel>
#include <QSettings>
#include <QCloseEvent>
#include <QFileDialog>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QMenuBar>
#include <QTimer>
#include <QClipboard>
#include <QDesktopServices>
#include <QMessageBox>
#include <QLabel>
#include <QKeySequenceEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <liboai.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wunused-variable"

#pragma GCC diagnostic pop

using namespace liboai;

const char *defaultPrompt = "Correct text for grammar, syntax, and punctuation, "
                            "preserving the original meaning and tone. Use clear, "
                            "polite language, avoid unnecessary changes and complex vocabulary.";

const char *hiddenPrompt = "Don't ask for clarification, simply provide the updated "
                           "text, without any introductions or additions. Text to correct: ";

/*
===================
MainWindow::MainWindow
===================
*/
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->centralWidget->setLayout(ui->mainLayout);
    setWindowTitle("Grammar Checker");
    setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

    notification.setSource(QUrl::fromLocalFile("sounds/notification.wav"));

    for (int i = 0; i < NUMBER_OF_TABS; i++)
    {
        profiles[i] = new Profile();
        ui->tabWidget->addTab(profiles[i], tr("Profile") + " " + QString::number(i + 1));
    }

    setupMenus();
    readSettings();
    retranslate();
    switchLanguage(language);
    filter.window = this;
    qApp->installNativeEventFilter(&filter);
    appInitiated = true;
}

/*
===================
MainWindow::~MainWindow
===================
*/
MainWindow::~MainWindow()
{
    writeSettings();
    delete ui;
}

/*
===================
MainWindow::show
===================
*/
void MainWindow::show()
{
    if (QSystemTrayIcon::isSystemTrayAvailable() && showInTrayAction->isChecked())
    {
        trayIcon->show();
        gcApp->setQuitOnLastWindowClosed(false);
    }
    else
    {
        QMainWindow::show();
        trayIcon->hide();
        gcApp->setQuitOnLastWindowClosed(true);
    }
}

/*
===================
MainWindow::checkGrammar
===================
*/
void MainWindow::checkGrammar(int id)
{
    QClipboard *clipboard = QApplication::clipboard();
    QString savedClipboard = clipboard->text();

    if (!cutToClipboard())
        return;

    auto modelData = profiles[id]->currentModel();

    QString modelName = modelData.first;
    ModelProvider *model = modelData.second;
    OpenAI oai(model->url.toUtf8().data());
    Conversation convo;
    QString output;
    QString prompt = profiles[id]->prompt();

    prompt.append(profiles[id]->hiddenPrompt());
    prompt.append(clipboard->text());

    if (!convo.AddUserData(prompt.toUtf8().data()))
    {
        qDebug() << "Couldn't add user input to the conversation";
        return;
    }

    oai.auth.SetMaxTimeout(maxTimeout);

    if (oai.auth.SetKey(profiles[id]->key().toUtf8().data()))
    {
#if 0
        // Lists the currently available models
        Response response = oai.Model->list();

        std::cout << response["data"];
        return;
#endif

        try
        {
            Response response = oai.ChatCompletion->create(modelName.toUtf8().data(), convo);

            if (!convo.Update(response))
            {
                qDebug() << "Couldn't update the conversation given a response object";
                return;
            }

            output.append(convo.GetLastResponse());
        }
        catch (std::exception& e)
        {
#ifdef QT_DEBUG
            qDebug() << e.what();
#else
            QMessageBox::critical(nullptr, "Grammar Checker", e.what());
#endif
            return;
        }
    }
    else
    {
#ifdef QT_DEBUG
        qDebug() << "Coundn't set the authorization key for the OpenAI API";
#else
        QMessageBox::critical(nullptr, "Grammar Checker", "Coundn't set the authorization key for the OpenAI API");
#endif
        return;
    }

    clipboard->setText(output);
    gcApp->waitForClipboardChange();
    pasteFromClipboard(smoothTypingAction->isChecked(), smoothTypingDelay);

    clipboard->setText(savedClipboard);
    gcApp->waitForClipboardChange();

    if (notificationSoundAction->isChecked())
        notification.play();
    qDebug("true");
}

/*
===================
MainWindow::closeEvent
===================
*/
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (QSystemTrayIcon::isSystemTrayAvailable() && showInTrayAction->isChecked())
    {
        // Hides the window instead of closing as it can appear out of the screen after disconnecting a display.
        hide();
        event->ignore();
    }
    else
    {
        event->accept();
    }
}

/*
===================
MainWindow::showEvent
===================
*/
void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
}

/*
===================
MainWindow::quit
===================
*/
void MainWindow::quit()
{
    gcApp->quit();
}

/*
===================
MainWindow::setTrayVisible
===================
*/
void MainWindow::setTrayVisible(bool visible)
{
    if (QSystemTrayIcon::isSystemTrayAvailable())
        showInTrayAction->setChecked(visible);
    else
        showInTrayAction->setChecked(false);

    show();
    writeSettings();
}

/*
===================
MainWindow::trayIconActivated
===================
*/
void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
    {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:

#ifdef Q_OS_LINUX
    // Double click doesn't work on GNOME
    case QSystemTrayIcon::MiddleClick:

        // Fixes wrong window position after hiding the window.
        if (isHidden()) move(pos().x() + (frameSize().width() - size().width()), pos().y() + (frameSize().height() - size().height()));
#endif

        setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
        QMainWindow::show();
        raise();
        activateWindow();

        break;
    default:
        break;
    }
}

/*
===================
MainWindow::switchLanguage
===================
*/
void MainWindow::switchLanguage(QLocale::Language language)
{
    for (int i = 0; i < Application::languageCount(); i++)
        languageActions[i]->setChecked(language == languages[i].language);

    gcApp->setTranslator(language);
    this->language = language;
    retranslate();
}

/*
===================
MainWindow::toggleNotificationSound
===================
*/
void MainWindow::toggleNotificationSound()
{
    writeSettings();
}

/*
===================
MainWindow::toggleSmoothTyping
===================
*/
void MainWindow::toggleSmoothTyping()
{
    writeSettings();
}

/*
===================
MainWindow::launchOnStartup
===================
*/
void MainWindow::toggleLaunchOnStartup()
{
    gcApp->setLaunchOnStartup(launchOnStartupAction->isChecked());
    writeSettings();
}

/*
===================
MainWindow::showInTray
===================
*/
void MainWindow::toggleShowInTray()
{
    setTrayVisible(showInTrayAction->isChecked());
    writeSettings();
}

/*
===================
MainWindow::openConfig
===================
*/
void MainWindow::openConfig()
{
    QString path(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + SETTINGS_FILENAME);
    QDesktopServices::openUrl(path);
}

/*
===================
MainWindow::setupMenus
===================
*/
void MainWindow::setupMenus()
{
    iconMain.addFile(":/Icon.ico");
    iconSettings.addFile(":/Images/IconSettings.png");

    notificationSoundAction = new QAction("&" + tr("Notification Sound"), this);
    smoothTypingAction = new QAction("&" + tr("Smooth Typing"), this);
    launchOnStartupAction = new QAction("&" + tr("Launch on Startup"), this);
    showInTrayAction = new QAction("&" + tr("Show in System Tray"));
    openConfigAction = new QAction("&" + tr("Open Config"));
    showAction = new QAction("&" + tr("Show"), this);
    quitAction = new QAction("&" + tr("Quit"), this);
    reportBugAction = new QAction("&" + tr("Report a Bug"), this);
    version = new QAction(QString(tr("Version: %1")).arg(GRAMMAR_CHECKER_VERSION), this);

    for (int i = 0; i < Application::languageCount(); i++)
        languageActions.append(new QAction(tr(languages[i].name), this));

    version->setDisabled(true);

    notificationSoundAction->setCheckable(true);
    smoothTypingAction->setCheckable(true);
    launchOnStartupAction->setCheckable(true);
    showInTrayAction->setCheckable(true);

#ifdef Q_OS_WIN
    launchOnStartupAction->setChecked(QFile::exists(QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + "/Startup/GrammarChecker.lnk"));
#else
    launchOnStartupAction->setChecked(QFile::exists(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart/GrammarChecker.desktop"));
#endif

    for (int i = 0; i < Application::languageCount(); i++)
        languageActions[i]->setCheckable(true);

    languageMenu = new UnhidableMenu("&" + tr("Language"), this);

    for (int i = 0; i < Application::languageCount(); i++)
        languageMenu->addAction(languageActions[i]);

    settingsMenu = new UnhidableMenu("&" + tr("Settings"), this);
    settingsMenu->setIcon(iconSettings);
    settingsMenu->addMenu(languageMenu);
    settingsMenu->addAction(notificationSoundAction);
    settingsMenu->addAction(smoothTypingAction);
    settingsMenu->addAction(launchOnStartupAction);
    settingsMenu->addAction(showInTrayAction);
    settingsMenu->addSeparator();
    settingsMenu->addAction(openConfigAction);    
    settingsMenu->addAction(reportBugAction);
    settingsMenu->addSeparator();
    settingsMenu->addAction(version);

    trayIconMenu = new UnhidableMenu(this);
    trayIconMenu->addMenu(settingsMenu);
    trayIconMenu->addSeparator();

#ifdef Q_OS_LINUX
    trayIconMenu->addAction(showAction);
#endif

    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setToolTip("Grammar Checker");
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(iconMain);

    this->menuBar()->addMenu(settingsMenu);

    connect(notificationSoundAction, &QAction::triggered, this, &MainWindow::toggleNotificationSound);
    connect(smoothTypingAction, &QAction::triggered, this, &MainWindow::toggleSmoothTyping);
    connect(launchOnStartupAction, &QAction::triggered, this, &MainWindow::toggleLaunchOnStartup);
    connect(showInTrayAction, &QAction::triggered, this, &MainWindow::toggleShowInTray);
    connect(openConfigAction, &QAction::triggered, this, &MainWindow::openConfig);
    connect(showAction, &QAction::triggered, this, std::bind(&MainWindow::trayIconActivated, this, QSystemTrayIcon::DoubleClick));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(quit()));
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    connect(reportBugAction, &QAction::triggered, this, [this](){ QDesktopServices::openUrl(QUrl(BUG_TRACKER_LINK)); });

    for (int i = 0; i < Application::languageCount(); i++)
        connect(languageActions[i], &QAction::triggered, this, std::bind(&MainWindow::switchLanguage, this, languages[i].language));
}

/*
===================
MainWindow::readSettings
===================
*/
void MainWindow::readSettings()
{
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + SETTINGS_FILENAME, QSettings::IniFormat);

    providers.clear();

    resize(QSize(settings.value("Width", 500).toInt(), settings.value("Height", 300).toInt()));
    setWindowState(settings.value("Fullscreen", false).toBool() ? Qt::WindowMaximized : Qt::WindowActive);

    notificationSoundAction->setChecked(settings.value("NotificationSound", true).toBool());
    notification.setVolume(settings.value("NotificationSoundVolume", 1.0f).toFloat());
    smoothTypingAction->setChecked(settings.value("SmoothTyping", true).toBool());
    bool showInTray = settings.value("ShowInTray", QSystemTrayIcon::isSystemTrayAvailable()).toBool();
    showInTrayAction->setChecked(showInTray);
    language = static_cast<QLocale::Language>(settings.value("Language", QLocale::system().language()).toInt());
    smoothTypingDelay = settings.value("SmoothTypingDynamicDelay", SMOOTH_TYPING_DELAY).toInt();
    maxTimeout = settings.value("MaxTimeout", MAX_TIMEOUT).toInt();

    providers.insert("OpenAI", { "https://api.openai.com/v1", "", {"gpt-4o-mini", "gpt-5-mini", "gpt-5"} });

    providers.insert("Google", { "https://generativelanguage.googleapis.com/v1beta", "", { "gemini-2.5-flash",
																						   "gemini-2.5-pro",
                                                                                           "gemini-2.0-flash",
                                                                                           "gemini-2.5-flash-lite",
                                                                                           "gemini-flash-latest",
                                                                                           "gemini-flash-lite-latest" } });

    providers.insert("DeepSeek", { "https://api.deepseek.com", "", { "deepseek-chat" } });
    providers.insert("xAI", { "https://api.x.ai/v1", "", { "grok-3-mini", "grok-3", "grok-4-0709" } });

    // Loads models
    for (auto &provider : settings.childGroups())
    {
        if (!providers.contains(provider))
            providers.insert(provider, ModelProvider());

        ModelProvider &modelProvider = providers[provider];

        settings.beginGroup(provider);

        modelProvider.url = settings.value("URL", modelProvider.url).toString();
        modelProvider.key = settings.value("Key", modelProvider.key).toString();

        settings.beginGroup("Models");

        for (auto &model : settings.allKeys())
            modelProvider.models.insert(model);

        settings.endGroup();
        settings.endGroup();
    }

    for (int i = 0; i < NUMBER_OF_TABS; i++)
    {
        profiles[i]->setProviders(providers);
        profiles[i]->readSettings();
    }
}

/*
===================
MainWindow::writeSettings
===================
*/
void MainWindow::writeSettings() const
{
    if (!appInitiated)
        return;

    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + SETTINGS_FILENAME, QSettings::IniFormat);

    if (!isMaximized())
    {
        settings.setValue("Width", size().width());
        settings.setValue("Height", size().height());
    }

    settings.setValue("Fullscreen", isMaximized());
    settings.setValue("NotificationSound", notificationSoundAction->isChecked());
    settings.setValue("NotificationSoundVolume", notification.volume());
    settings.setValue("SmoothTyping", smoothTypingAction->isChecked());
    settings.setValue("ShowInTray", showInTrayAction->isChecked());
    settings.setValue("Language", language);
    settings.setValue("SmoothTypingDynamicDelay", smoothTypingDelay);
    settings.setValue("MaxTimeout", maxTimeout);

    // Models
    for (auto it = providers.begin(); it != providers.end(); it++)
    {
        settings.beginGroup(it.key());
        settings.setValue("URL", it.value().url);
        settings.setValue("Key", it.value().key);
        settings.beginGroup("Models");

        for (auto &model : it.value().models)
            settings.setValue(model, "");

        settings.endGroup();
        settings.endGroup();
    }

    for (int i = 0; i < NUMBER_OF_TABS; i++)
        profiles[i]->writeSettings();
}

/*
===================
MainWindow::retranslate
===================
*/
void MainWindow::retranslate()
{
    settingsMenu->setTitle("&" + tr("Settings"));
    languageMenu->setTitle("&" + tr("Language"));
    notificationSoundAction->setText("&" + tr("Notification Sound"));
    smoothTypingAction->setText("&" + tr("Smooth Typing"));
    launchOnStartupAction->setText("&" + tr("Launch on Startup"));
    showInTrayAction->setText("&" + tr("Show in System Tray"));
    openConfigAction->setText("&" + tr("Open Config"));
    showAction->setText("&" + tr("Show"));
    quitAction->setText("&" + tr("Quit"));
    reportBugAction->setText("&" + tr("Report a Bug"));
    version->setText(QString(tr("Version: %1")).arg(GRAMMAR_CHECKER_VERSION));

    for (int i = 0; i < ui->tabWidget->count(); i++)
        ui->tabWidget->setTabText(i, tr("Profile") + " " + QString::number(i + 1));

    for (int i = 0; i < NUMBER_OF_TABS; i++)
        profiles[i]->retranslate();
}
