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
#include <QMutex>

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
            static QMutex mutex;

            for (int i = 0; i < NUMBER_OF_TABS; i++)
            {
                for (int j = 0; j < window->keySequence(i).count(); j++)
                {
                    UINT modifier = toNativeModifier(window->keySequence(i)[j].keyboardModifiers());
                    UINT virtualKey = toNativeKey(window->keySequence(i)[j].key());

                    if (modifier == LOWORD(msg->lParam) && virtualKey == HIWORD(msg->lParam))
                    {
                        if (!mutex.tryLock())
                            return true;

                        window->checkGrammar(i);
                        mutex.unlock();
                        return true;
                    }
                }
            }
        }
    }
#endif

    return false;
}
