QT += widgets multimedia

INCLUDEPATH += liboai-main/liboai/include/ \
               curl-8.11.1_3-win64-mingw/include \
               json/include

LIBS += -lUser32 \
        -L"$$PWD/liboai-main/liboai" -lliboai \
        -L"$$PWD/curl-8.11.1_3-win64-mingw/lib" -llibcurl -llibcurl.dll

CONFIG += c++20 lrelease embed_translations

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Application.cpp \
    Common.cpp \
    Main.cpp \
    MainWindow.cpp \
    NativeEventFilter.cpp \
    Profile.cpp \
    UnhidableMenu.cpp

HEADERS += \
    Application.h \
    Common.h \
    MainWindow.h \
    NativeEventFilter.h \
    Profile.h \
    UnhidableMenu.h

FORMS += \
    MainWindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    Icon.ico \
    Resources.rc \
    LICENSE.txt \
    README.md

RESOURCES += \
    GrammarChecker.qrc

RC_FILE = Resources.rc

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
    Translations/pt_PT.ts \
    Translations/ru_RU.ts \
    Translations/uk_UA.ts \
    Translations/zh_CN.ts
