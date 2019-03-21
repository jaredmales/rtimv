TEMPLATE = app
TARGET = rtimv
DESTDIR = bin/ 
DEPENDPATH += src/

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
           src/rtimvImage.hpp \
           src/pixaccess.h \
           src/colorMaps.hpp

SOURCES += src/graphicsview.cpp \
           src/imviewer.cpp \
           src/imviewer_main.cpp \
           src/imviewerform.cpp \
           src/imviewerControlPanel.cpp \
           src/StretchBox.cpp \
           src/StretchCircle.cpp \ 
           src/imviewerstats.cpp \
           src/rtimvImage.cpp
           
FORMS += forms/imviewergui.ui \
         forms/imviewerControlPanel.ui \
         forms/imviewerStats.ui 
           
LIBS += -lImageStreamIO
        
RESOURCES += res/imviewer.qrc

QT += widgets
