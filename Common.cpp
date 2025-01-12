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
#include "Common.h"
#include <QClipboard>
#include <QTime>
#include <windows.h>

/*
===================
cutToClipboard
===================
*/
void cutToClipboard()
{
    INPUT inputs[4];
    memset(inputs, 0, sizeof(inputs));

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
        qDebug("SendInput failed: 0x%ld", HRESULT_FROM_WIN32(GetLastError()));
        return;
    }

    gcApp->waitForClipboardChange();
}

/*
===================
pasteFromClipboard
===================
*/
void pasteFromClipboard(bool smoothPasting, int smoothPastingDelay)
{
    QString clipboard = QApplication::clipboard()->text();

    if (smoothPasting)
    {
        INPUT input[2];
        memset(&input, 0, sizeof(input));

        for (int i = 0; i < clipboard.length(); i++)
        {
            input[0].type = INPUT_KEYBOARD;
            input[0].ki.dwFlags = KEYEVENTF_UNICODE;
            input[0].ki.wScan = clipboard[i].unicode();
            input[1].type = INPUT_KEYBOARD;
            input[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
            input[1].ki.wScan = clipboard[i].unicode();

            if (SendInput(ARRAYSIZE(input), input, sizeof(INPUT)) != ARRAYSIZE(input))
            {
                qDebug("SendInput failed: 0x%ld", HRESULT_FROM_WIN32(GetLastError()));
                return;
            }

            Sleep(smoothPastingDelay);
        }
    }
    else
    {
        INPUT input[4];
        memset(input, 0, sizeof(input));

        // Ctrl
        input[0].type = INPUT_KEYBOARD;
        input[0].ki.wVk = VK_LCONTROL;

        input[3].type = INPUT_KEYBOARD;
        input[3].ki.wVk = VK_LCONTROL;
        input[3].ki.dwFlags = KEYEVENTF_KEYUP;

        // Paste
        input[1].type = INPUT_KEYBOARD;
        input[1].ki.wVk = 'V';

        input[2].type = INPUT_KEYBOARD;
        input[2].ki.wVk = 'V';
        input[2].ki.dwFlags = KEYEVENTF_KEYUP;

        if (SendInput(ARRAYSIZE(input), input, sizeof(INPUT)) != ARRAYSIZE(input))
        {
            qDebug("SendInput failed: 0x%ld", HRESULT_FROM_WIN32(GetLastError()));
            return;
        }

        // Waits for clipboard content to be pasted
        QTime dieTime = QTime::currentTime().addSecs(1);
        while (QTime::currentTime() < dieTime)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
}

/*
===================
unregisterShortcut
===================
*/
void unregisterShortcut()
{
    UnregisterHotKey(NULL, 0);
}

/*
===================
registerShortcut
===================
*/
void registerShortcut(const QKeySequence &keySequence)
{
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
