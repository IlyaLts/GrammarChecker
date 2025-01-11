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
#include "ui_MainWindow.h"
#include <Windows.h>
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wunused-variable"

#include <liboai.h>

#pragma GCC diagnostic pop

using namespace liboai;

const char *defaultPrompt = "Correct text for grammar, syntax, and punctuation, "
                            "preserving the original meaning and tone. Use clear, "
                            "polite language, avoid unnecessary changes and complex vocabulary.";

const char *finalPrompt = "Don't ask for clarification, simply provide the updated "
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

    connect(QApplication::clipboard(), QClipboard::dataChanged, this, &MainWindow::clipboardChanged);
    connect(ui->shortcutEdit, QKeySequenceEdit::keySequenceChanged, this, &MainWindow::keyChanged);

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
        syncApp->setQuitOnLastWindowClosed(false);
    }
    else
    {
        QMainWindow::show();
        trayIcon->hide();
        syncApp->setQuitOnLastWindowClosed(true);
    }
}

/*
===================
MainWindow::checkGrammar
===================
*/
void MainWindow::checkGrammar()
{
    QClipboard *clipboard = QApplication::clipboard();
    QString savedClipboard = clipboard->text();

    INPUT inputs[4];
    memset(inputs, 0, sizeof(inputs));

    clipboard->clear();
    waitForClipboardChange();

    // Ctrl
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_LCONTROL;

    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_LCONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    // Cut
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'X';

    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'X';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

    if (SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT)) != ARRAYSIZE(inputs))
    {
        qDebug("SendInput failed: 0x%ld\n", HRESULT_FROM_WIN32(GetLastError()));
        return;
    }

    waitForClipboardChange();

    QString text;
    QString prompt = ui->promptTextEdit->toPlainText();
    prompt.append(finalPrompt);
    prompt.append(clipboard->text());

    OpenAI oai;
    Conversation convo;
    convo.AddUserData(prompt.toUtf8().data());

    if (oai.auth.SetKey(ui->keyLineEdit->text().toLocal8Bit().data()))
    {
#if 0
        // Lists the currently available models
        Response response = oai.Model->list();

        std::cout << response["data"];
        return;
#endif
        text.append(convo.GetLastResponse());

        try
        {
            Response response = oai.ChatCompletion->create(model.toLocal8Bit().data(), convo);
            convo.Update(response);
            text.append(convo.GetLastResponse());
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
    }
    else
    {
        qDebug("Coundn't set the authorization key for the OpenAI API");
    }

    if (smoothTypingAction->isChecked())
    {
        INPUT input;
        memset(&input, 0, sizeof(input));

        for (int i = 0; i < text.length(); i++)
        {
            input.type = INPUT_KEYBOARD;
            input.ki.dwFlags = KEYEVENTF_UNICODE;
            input.ki.wScan = text[i].unicode();

            if (SendInput(1, &input, sizeof(INPUT)) != 1)
            {
                qDebug("SendInput failed: 0x%ld\n", HRESULT_FROM_WIN32(GetLastError()));
                return;
            }

            input.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

            if (SendInput(1, &input, sizeof(INPUT)) != 1)
            {
                qDebug("SendInput failed: 0x%ld\n", HRESULT_FROM_WIN32(GetLastError()));
                return;
            }

            Sleep(smoothTypingDelay);
        }
    }
    else
    {
        clipboard->setText(text);
        waitForClipboardChange();

        // Paste
        inputs[1].ki.wVk = 'V';
        inputs[2].ki.wVk = 'V';

        if (SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT)) != ARRAYSIZE(inputs))
        {
            qDebug("SendInput failed: 0x%ld\n", HRESULT_FROM_WIN32(GetLastError()));
            return;
        }

        // Waits for clipboard content to be pasted
        QTime dieTime = QTime::currentTime().addSecs(1);
        while (QTime::currentTime() < dieTime)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }

    clipboard->setText(savedClipboard);
    waitForClipboardChange();

    if (notificationSoundAction->isChecked())
        notification.play();
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
    syncApp->quit();
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

    syncApp->setTranslator(language);
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
    syncApp->setLaunchOnStartup(launchOnStartupAction->isChecked());
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
MainWindow::clipboardChanged
===================
*/
void MainWindow::clipboardChanged()
{
    m_clipboardChanged = false;
}

/*
===================
toNativeKey
===================
*/
unsigned int toNativeKey(Qt::Key key)
{
    // 0 - 9
    if (key >= Qt::Key_0 && key <= Qt::Key_9)
        return key;

    // A - Z
    if (key >= Qt::Key_A && key <= Qt::Key_Z)
        return key;

    // F1 - F24
    if (key >= Qt::Key_F1 && key <= Qt::Key_F24)
        return VK_F1 + (key - Qt::Key_F1);

    switch (key)
    {
    case Qt::Key_Escape:
        return VK_ESCAPE;
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
        return VK_TAB;
    case Qt::Key_Backspace:
        return VK_BACK;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return VK_RETURN;
    case Qt::Key_Insert:
        return VK_INSERT;
    case Qt::Key_Delete:
        return VK_DELETE;
    case Qt::Key_Pause:
        return VK_PAUSE;
    case Qt::Key_Print:
        return VK_PRINT;
    case Qt::Key_Clear:
        return VK_CLEAR;
    case Qt::Key_Home:
        return VK_HOME;
    case Qt::Key_End:
        return VK_END;
    case Qt::Key_Left:
        return VK_LEFT;
    case Qt::Key_Up:
        return VK_UP;
    case Qt::Key_Right:
        return VK_RIGHT;
    case Qt::Key_Down:
        return VK_DOWN;
    case Qt::Key_PageUp:
        return VK_PRIOR;
    case Qt::Key_PageDown:
        return VK_NEXT;
    case Qt::Key_Space:
        return VK_SPACE;
    case Qt::Key_Asterisk:
        return VK_MULTIPLY;
    case Qt::Key_Plus:
        return VK_ADD;
    case Qt::Key_Comma:
        return VK_SEPARATOR;
    case Qt::Key_Minus:
        return VK_SUBTRACT;
    case Qt::Key_Slash:
        return VK_DIVIDE;
    default:
        return 0;
    }

    return 0;
}

/*
===================
MainWindow::keyChanged
===================
*/
void MainWindow::keyChanged(const QKeySequence &keySequence)
{
    if (keySequence.isEmpty())
    {
        // Translates shortcut field placeholder
        retranslate();
        return;
    }

    UnregisterHotKey(NULL, 0);

    unsigned int modifier = 0;
    unsigned int virtualKey = toNativeKey(keySequence[0].key());

    Qt::KeyboardModifiers keyboardModifier = keySequence[0].keyboardModifiers();

    if (keyboardModifier & Qt::AltModifier)
        modifier |= MOD_ALT;
    if (keyboardModifier & Qt::ControlModifier)
        modifier |= MOD_CONTROL;
    if (keyboardModifier & Qt::ShiftModifier)
        modifier |= MOD_SHIFT;
    if (keyboardModifier & Qt::MetaModifier)
        modifier |= MOD_WIN;
    //A keypad button is pressed.
    //if (keyboardModifier & Qt::KeypadModifier)
    //    modifier |= MOD_;

    RegisterHotKey(NULL, 0, modifier, virtualKey);
    ui->shortcutEdit->clearFocus();
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

    notificationSoundAction = new QAction(tr("&Notification Sound"), this);
    smoothTypingAction = new QAction(tr("&Smooth Typing"), this);
    launchOnStartupAction = new QAction(tr("&Launch on Startup"), this);
    showInTrayAction = new QAction(tr("&Show in System Tray"));
    openConfigAction = new QAction(tr("&Open Config"));
    showAction = new QAction(tr("&Show"), this);
    quitAction = new QAction(tr("&Quit"), this);
    version = new QAction(QString(tr("Version: %1")).arg(GRAMMAR_CHECKER_VERSION), this);

    for (int i = 0; i < Application::languageCount(); i++)
        languageActions.append(new QAction(tr(languages[i].name), this));

    version->setDisabled(true);

    notificationSoundAction->setCheckable(true);
    smoothTypingAction->setCheckable(true);
    launchOnStartupAction->setCheckable(true);
    showInTrayAction->setCheckable(true);

#ifdef Q_OS_WIN
    launchOnStartupAction->setChecked(QFile::exists(QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + "/Startup/SyncManager.lnk"));
#else
    launchOnStartupAction->setChecked(QFile::exists(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart/SyncManager.desktop"));
#endif

    for (int i = 0; i < Application::languageCount(); i++)
        languageActions[i]->setCheckable(true);

    languageMenu = new UnhidableMenu(tr("&Language"), this);

    for (int i = 0; i < Application::languageCount(); i++)
        languageMenu->addAction(languageActions[i]);

    settingsMenu = new UnhidableMenu(tr("&Settings"), this);
    settingsMenu->setIcon(iconSettings);
    settingsMenu->addMenu(languageMenu);
    settingsMenu->addAction(notificationSoundAction);
    settingsMenu->addAction(smoothTypingAction);
    settingsMenu->addAction(launchOnStartupAction);
    settingsMenu->addAction(showInTrayAction);
    settingsMenu->addSeparator();
    settingsMenu->addAction(openConfigAction);
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

    resize(QSize(settings.value("Width", 500).toInt(), settings.value("Height", 300).toInt()));
    setWindowState(settings.value("Fullscreen", false).toBool() ? Qt::WindowMaximized : Qt::WindowActive);

    notificationSoundAction->setChecked(settings.value("NotificationSound", true).toBool());
    notification.setVolume(settings.value("NotificationSoundVolume", 1.0f).toFloat());
    smoothTypingAction->setChecked(settings.value("SmoothTyping", true).toBool());
    bool showInTray = settings.value("ShowInTray", QSystemTrayIcon::isSystemTrayAvailable()).toBool();
    showInTrayAction->setChecked(showInTray);
    language = static_cast<QLocale::Language>(settings.value("Language", QLocale::system().language()).toInt());

    ui->shortcutEdit->setKeySequence(QKeySequence(settings.value("Shortcut", QKeySequence(DEFAULT_SHORTCUT_KEY)).toString()));
    model = settings.value("Model", DEFAULT_MODEL).toString();
    ui->keyLineEdit->setText(settings.value("key", QString()).toString());
    ui->promptTextEdit->setText(settings.value("Prompt", defaultPrompt).toString());
    smoothTypingDelay = settings.value("SmoothTypingDelay", SMOOTH_TYPING_DELAY).toInt();
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
    settings.setValue("AppVersion", GRAMMAR_CHECKER_VERSION);
    settings.setValue("Language", language);
    settings.setValue("Shortcut", ui->shortcutEdit->keySequence());
    settings.setValue("Model", model);
    settings.setValue("Key", ui->keyLineEdit->text());
    settings.setValue("Prompt", ui->promptTextEdit->toPlainText());
    settings.setValue("SmoothTypingDelay", smoothTypingDelay);
}

/*
===================
MainWindow::waitForClipboardChange
===================
*/
void MainWindow::waitForClipboardChange()
{
    m_clipboardChanged = true;

    while (m_clipboardChanged)
        QApplication::processEvents();
}

/*
===================
MainWindow::retranslate
===================
*/
void MainWindow::retranslate()
{
    settingsMenu->setTitle(tr("&Settings"));
    languageMenu->setTitle(tr("&Language"));
    notificationSoundAction->setText(tr("&Notification Sound"));
    smoothTypingAction->setText(tr("&Smooth Typing"));
    launchOnStartupAction->setText(tr("&Launch on Startup"));
    showInTrayAction->setText(tr("&Show in System Tray"));
    openConfigAction->setText(tr("&Open Config"));
    showAction->setText(tr("&Show"));
    quitAction->setText(tr("&Quit"));
    version->setText(QString(tr("Version: %1")).arg(GRAMMAR_CHECKER_VERSION));

    ui->shortcutLabel->setText(tr("Shortcut:"));
    ui->keyLabel->setText(tr("OpenAI API key:"));
    ui->promptLabel->setText(tr("Prompt:"));

    if (QLineEdit *lineEdit = ui->shortcutEdit->findChild<QLineEdit *>("qt_keysequenceedit_lineedit"))
        lineEdit->setPlaceholderText(tr("Press shortcut"));
}
