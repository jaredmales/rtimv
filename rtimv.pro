TEMPLATE = app
TARGET = rtimv
DESTDIR = bin/ 
DEPENDPATH += src/

#INCLUDEPATH += /usr/include/x86_64-linux-gnu/qt5/
#INCLUDEPATH += /usr/include/qt5/

MOC_DIR = moc/
OBJECTS_DIR = obj/
RCC_DIR = res/
UI_DIR = forms/

CONFIG(release, debug|release) {
    CONFIG += optimize_full
}

CONFIG += c++14

MAKEFILE = makefile.rtimv

# Input
HEADERS += src/graphicsview.hpp \
           src/imviewer.hpp \
           src/imviewerform.hpp \ 
           src/imviewerControlPanel.h \
           src/StretchBox.h \
           src/StretchCircle.h \
           src/imviewerstats.hpp \
           src/pixaccess.h \
           src/colorMaps.hpp

SOURCES += src/graphicsview.cpp \
           src/imviewer.cpp \
           src/imviewer_main.cpp \
           src/imviewerform.cpp \
           src/imviewerControlPanel.cpp \
           src/StretchBox.cpp \
           src/StretchCircle.cpp \ 
           src/imviewerstats.cpp
           
FORMS += forms/imviewergui.ui \
         forms/imviewerControlPanel.ui \
         forms/imviewerStats.ui 
           
LIBS += -lImageStreamIO
        
RESOURCES += res/imviewer.qrc

QT += widgets
