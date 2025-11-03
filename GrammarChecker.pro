QT += widgets multimedia

INCLUDEPATH += liboai-main/liboai/include/ \
               curl-8.11.1_3-win64-mingw/include \
               json/include

win32 {
    LIBS += -L"$$PWD/liboai-main/liboai" -lliboai \
            -lUser32 \
            -L"$$PWD/curl-8.11.1_3-win64-mingw/lib" -llibcurl -llibcurl.dll
}

unix {
    LIBS += -L"$$PWD/liboai-main/liboai" -loai \
            -lcurl \
            -lX11 \
            -lXtst
}

CONFIG += c++20 lrelease embed_translations pkgconfig
PKGCONFIG += libcurl

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Source/Application.cpp \
    Source/Main.cpp \
    Source/MainWindow.cpp \
    Source/Profile.cpp \
    Source/UnhidableMenu.cpp

win32 {
SOURCES += \
    Source/Win/Common.cpp \
    Source/Win/NativeEventFilter.cpp \
}

unix {
SOURCES += \
    Source/X11/Common.cpp \
    Source/X11/NativeEventFilter.cpp
}

HEADERS += \
    Source/Application.h \
    Source/Common.h \
    Source/MainWindow.h \
    Source/NativeEventFilter.h \
    Source/Profile.h \
    Source/UnhidableMenu.h

FORMS += \
    Source/MainWindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    Source/Icon.ico \
    Source/Resources.rc \
    LICENSE.txt \
    README.md

RESOURCES += \
    GrammarChecker.qrc

RC_FILE = Source/Resources.rc

# qml_debug flag for debug and profile build configurations
CONFIG(qml_debug): DEFINES += DEBUG

TRANSLATIONS += \
    Translations/de_DE.ts \
    Translations/en_US.ts \
    Translations/es_ES.ts \
    Translations/fr_FR.ts \
    Translations/hi_IN.ts \
    Translations/it_IT.ts \
    Translations/ja_JP.ts \
    Translations/ko_KR.ts \
    Translations/pl_PL.ts \
    Translations/pt_PT.ts \
    Translations/ru_RU.ts \
    Translations/tr_TR.ts \
    Translations/uk_UA.ts \
    Translations/zh_CN.ts
