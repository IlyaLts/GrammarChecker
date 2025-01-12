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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Application.h"
#include "NativeEventFilter.h"
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QSoundEffect>

#define DEFAULT_SHORTCUT_KEY    "CTRL+F12"
#define DEFAULT_MODEL           "gpt-4o-mini"
#define SMOOTH_TYPING_DELAY      25

extern const char *defaultPrompt;
extern const char *finalPrompt;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class UnhidableMenu;

/*
===========================================================

    MainWindow

===========================================================
*/
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void checkGrammar();

public Q_SLOTS:

    void show();

protected:

    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private Q_SLOTS:

    void quit();
    void setTrayVisible(bool visible);
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void switchLanguage(QLocale::Language language);
    void toggleNotificationSound();
    void toggleSmoothTyping();
    void toggleLaunchOnStartup();
    void toggleShowInTray();
    void openConfig();
    void keyChanged(const QKeySequence &keySequence);

private:

    void setupMenus();
    void readSettings();
    void writeSettings() const;
    void retranslate();

    NativeEventFilter filter;

    Ui::MainWindow *ui;

    QIcon iconMain;
    QIcon iconSettings;

    QList<QAction *> languageActions;
    QAction *notificationSoundAction;
    QAction *smoothTypingAction;
    QAction *launchOnStartupAction;
    QAction *showInTrayAction;
    QAction *openConfigAction;
    QAction *showAction;
    QAction *quitAction;
    QAction *version;

    QSystemTrayIcon *trayIcon;
    UnhidableMenu *trayIconMenu;
    UnhidableMenu *settingsMenu;
    UnhidableMenu *languageMenu;

    QSoundEffect notification;

    QString model;

    QLocale::Language language;
    bool appInitiated = false;
    int smoothTypingDelay = SMOOTH_TYPING_DELAY;
};

#endif // MAINWINDOW_H
