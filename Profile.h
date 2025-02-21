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

#ifndef PROFILE_H
#define PROFILE_H

#include <QWidget>
#include <QComboBox>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QTextEdit>

class QVBoxLayout;
class ModelProvider;
class QLabel;
class QKeySequenceEdit;

/*
===========================================================

    Profile

===========================================================
*/
class Profile : public QWidget
{
    Q_OBJECT

public:

    Profile(QWidget* parent = nullptr);

    void setProviders(QMap<QString, ModelProvider> &providers);
    void retranslate();
    void readSettings();
    void writeSettings() const;

    inline QPair<QString, ModelProvider *> currentModel() const { return modelComboBox->currentData().value<QPair<QString, ModelProvider *>>(); }
    inline QKeySequence keySequence() const { return shortcutEdit->keySequence(); }
    inline QString key() const { return keyLineEdit->text(); }
    inline QString prompt() const { return promptTextEdit->toPlainText(); }

private Q_SLOTS:

    void keySequenceChanged(const QKeySequence &keySequence);
    void keyChanged(const QString &key);
    void modelChanged();

private:

    static int nextId;

    int id;
    QVBoxLayout *tabLayout;
    QLabel *shortcutLabel;
    QKeySequenceEdit *shortcutEdit;
    QLabel *modelLabel;
    QComboBox *modelComboBox;
    QLabel *keyLabel;
    QLineEdit *keyLineEdit;
    QLabel *promptLabel;
    QTextEdit *promptTextEdit;
};

#endif // PROFILE_H
