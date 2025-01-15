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

#ifndef COMMON_H
#define COMMON_H

#include <QKeySequence>

void cutToClipboard();
void pasteFromClipboard(bool smoothPasting, int smoothPastingDelay);
void registerShortcut(const QKeyCombination &keyCombination);
void unregisterShortcut();
unsigned int toNativeModifier(Qt::KeyboardModifiers modifiers);
unsigned int toNativeKey(Qt::Key key);

#endif // COMMON_H
