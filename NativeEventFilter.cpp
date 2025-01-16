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

#include "NativeEventFilter.h"
#include "MainWindow.h"
#include "Common.h"

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

/*
===================
NativeEventFilter::nativeEventFilter
===================
*/
bool NativeEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
#ifdef Q_OS_WIN
    MSG *msg = static_cast<MSG *>(message);

    Q_UNUSED(eventType);
    Q_UNUSED(result);

    if (msg->message == WM_HOTKEY)
    {
        if (window)
        {
            for (int i = 0; i < window->keySequence().count(); i++)
            {
                UINT modifier = toNativeModifier(window->keySequence()[i].keyboardModifiers());
                UINT virtualKey = toNativeKey(window->keySequence()[i].key());

                if (modifier == LOWORD(msg->lParam) && virtualKey == HIWORD(msg->lParam))
                    window->checkGrammar();
            }
        }

        return true;
    }
#endif

    return false;
}
