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

#include <X11/X.h>
#include <X11/Xlib.h>
#include <xcb/xcb.h>

/*
===================
NativeEventFilter::nativeEventFilter
===================
*/
bool NativeEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);

    if (!window)
        return false;

    xcb_generic_event_t *event = static_cast<xcb_generic_event_t *>(message);

    if (event->response_type == XCB_KEY_PRESS)
    {
        static QMutex mutex;
        xcb_key_press_event_t keyEvent = *static_cast<xcb_key_press_event_t *>(message);
        unsigned int eventModifier = keyEvent.state & (ShiftMask | ControlMask | Mod1Mask | Mod4Mask);
        unsigned int eventKeycode = keyEvent.detail;

        for (int i = 0; i < NUMBER_OF_TABS; i++)
        {
            for (int j = 0; j < window->keySequence(i).count(); j++)
            {
                unsigned int modifier = toNativeModifier(window->keySequence(i)[j].keyboardModifiers());
                unsigned int keycode = toNativeKey(window->keySequence(i)[j].key());

                if (modifier == eventModifier && keycode == eventKeycode)
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

    return false;
}
