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

#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include <QTranslator>

#define gcApp (static_cast<Application *>(QCoreApplication::instance()))

#define GRAMMAR_CHECKER_VERSION     "1.2"
#define SETTINGS_FILENAME           "Settings.ini"

struct Language
{
    QLocale::Language language;
    QLocale::Country country;
    const char *path;
    const char *name;
};

extern Language defaultLanguage;
extern Language languages[];

/*
===========================================================

    Application

===========================================================
*/
class Application : public QApplication
{
public:

    Application(int &argc, char **argv);

    bool waitForClipboardChange();
    void setLaunchOnStartup(bool enable);
    void setTranslator(QLocale::Language language);

    static int languageCount();

private Q_SLOTS:

    void clipboardChanged();

private:

    QTranslator translator;
    QLocale locale;
    bool m_clipboardChanged = false;
};

#endif // APPLICATION_H
