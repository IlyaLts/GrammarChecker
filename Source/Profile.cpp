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

#include "Profile.h"
#include "Common.h"
#include "MainWindow.h"
#include <QSettings>
#include <QStandardPaths>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QKeySequenceEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>

int Profile::nextId = 0;

/*
===================
Profile::Profile
===================
*/
Profile::Profile(QWidget* parent) : QWidget(parent)
{
    id = nextId;
    nextId++;

    tabLayout = new QVBoxLayout;

    enableCheckBox = new QCheckBox;
    shortcutLabel = new QLabel;
    shortcutEdit = new QKeySequenceEdit;
    modelLabel = new QLabel;
    modelComboBox = new QComboBox;
    keyLabel = new QLabel;
    keyLineEdit = new QLineEdit;
    promptLabel = new QLabel;
    promptTextEdit = new QTextEdit;

    tabLayout->addWidget(enableCheckBox);
    tabLayout->addWidget(shortcutLabel);
    tabLayout->addWidget(shortcutEdit);
    tabLayout->addWidget(modelLabel);
    tabLayout->addWidget(modelComboBox);
    tabLayout->addWidget(keyLabel);
    tabLayout->addWidget(keyLineEdit);
    tabLayout->addWidget(promptLabel);
    tabLayout->addWidget(promptTextEdit);

    enableCheckBox->setChecked(true);
    shortcutEdit->setClearButtonEnabled(true);
    shortcutEdit->setMaximumSequenceLength(4);
    modelComboBox->setInsertPolicy(QComboBox::InsertAlphabetically);
    keyLineEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    promptTextEdit->setAcceptRichText(true);

    setLayout(tabLayout);

    connect(enableCheckBox, &QCheckBox::checkStateChanged, this, &Profile::enableStateChanged);
    connect(shortcutEdit, &QKeySequenceEdit::keySequenceChanged, this, &Profile::keySequenceChanged);
    connect(keyLineEdit, &QLineEdit::textChanged, this, &Profile::keyChanged);
    connect(modelComboBox, &QComboBox::activated, this, &Profile::modelChanged);
}

/*
===================
Profile::setProviders
===================
*/
void Profile::setProviders(QMap<QString, ModelProvider> &providers)
{
    modelComboBox->clear();

    for (auto it = providers.begin(); it != providers.end(); it++)
        for (auto &model : it.value().models)
            modelComboBox->addItem(it.key() + "/" + model, QVariant::fromValue(QPair(model, &it.value())));
}

/*
===================
Profile::retranslate
===================
*/
void Profile::retranslate()
{
    enableCheckBox->setText(QCoreApplication::translate("MainWindow", "Enable"));
    shortcutLabel->setText(QCoreApplication::translate("MainWindow", "Shortcut:"));
    modelLabel->setText(QCoreApplication::translate("MainWindow", "Model:"));
    keyLabel->setText(QCoreApplication::translate("MainWindow", "API key:"));
    promptLabel->setText(QCoreApplication::translate("MainWindow", "Prompt:"));

    if (QLineEdit *lineEdit = shortcutEdit->findChild<QLineEdit *>("qt_keysequenceedit_lineedit"))
        lineEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "Press shortcut"));
}

/*
===================
Profile::readSettings
===================
*/
void Profile::readSettings()
{
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + SETTINGS_FILENAME, QSettings::IniFormat);

    shortcutEdit->setKeySequence(QKeySequence(settings.value("Shortcut" + QString::number(id)).toString()));
    promptTextEdit->setText(settings.value("Prompt" + QString::number(id), defaultPrompt).toString());
    m_hiddenPrompt.assign(settings.value("HiddenPrompt" + QString::number(id), ::hiddenPrompt).toString());

    if (modelComboBox->count())
    {
        // Loads the last-used model
        auto model = modelComboBox->currentData().value<QPair<QString, ModelProvider *>>();
        QString currentModel = settings.value("CurrentModel" + QString::number(id)).toString();

        int index = modelComboBox->findText(currentModel);

        if (index >= 0)
            modelComboBox->setCurrentIndex(index);

        // Key line edit
        keyLineEdit->setText(modelComboBox->currentData().value<QPair<QString, ModelProvider *>>().second->key);
    }

    enableCheckBox->setChecked(settings.value("Profile" + QString::number(id) + "_Enabled", true).toBool());
}

/*
===================
Profile::writeSettings
===================
*/
void Profile::writeSettings() const
{
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + SETTINGS_FILENAME, QSettings::IniFormat);

    settings.setValue("Shortcut" + QString::number(id), shortcutEdit->keySequence());
    settings.setValue("Prompt" + QString::number(id), promptTextEdit->toPlainText());
    settings.setValue("HiddenPrompt" + QString::number(id), m_hiddenPrompt);

    if (modelComboBox->count())
        settings.setValue("CurrentModel" + QString::number(id), modelComboBox->currentText());

    settings.setValue("Profile" + QString::number(id) + "_Enabled", enableCheckBox->isChecked());
}

/*
===================
Profile::enableStateChanged
===================
*/
void Profile::enableStateChanged(Qt::CheckState state)
{
    bool enable = false;

    int offset = id * NUMBER_OF_TABS;

    if (state == Qt::Unchecked)
    {
        enable = false;

        for (int i = 0; i < 4; i++)
            unregisterShortcut(offset + i);
    }
    else if (state == Qt::Checked)
    {
        enable = true;

        for (int i = 0; i < shortcutEdit->keySequence().count(); i++)
            registerShortcut(offset + i, shortcutEdit->keySequence()[i]);
    }

    shortcutLabel->setEnabled(enable);
    shortcutEdit->setEnabled(enable);
    modelLabel->setEnabled(enable);
    modelComboBox->setEnabled(enable);
    keyLabel->setEnabled(enable);
    keyLineEdit->setEnabled(enable);
    promptLabel->setEnabled(enable);
    promptTextEdit->setEnabled(enable);
}

/*
===================
Profile::keySequenceChanged
===================
*/
void Profile::keySequenceChanged(const QKeySequence &keySequence)
{
    // Translates shortcut field placeholder
    if (keySequence.isEmpty())
        retranslate();

    int offset = id * NUMBER_OF_TABS;

    for (int i = 0; i < 4; i++)
        unregisterShortcut(offset + i);

    for (int i = 0; i < keySequence.count(); i++)
        registerShortcut(offset + i, keySequence[i]);

    shortcutEdit->clearFocus();
}

/*
===================
Profile::keyChanged
===================
*/
void Profile::keyChanged(const QString &key)
{
    if (!modelComboBox->count())
        return;

    modelComboBox->currentData().value<QPair<QString, ModelProvider *>>().second->key = key;
}

/*
===================
Profile::modelChanged
===================
*/
void Profile::modelChanged()
{
    if (!modelComboBox->count())
        return;

    keyLineEdit->setText(modelComboBox->currentData().value<QPair<QString, ModelProvider *>>().second->key);
}
