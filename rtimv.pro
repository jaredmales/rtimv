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

CONFIG += -O3

QT += widgets

MAKEFILE = makefile.rtimv

# Input
HEADERS += src/rtimvGraphicsView.hpp \
           src/imviewer.hpp \
           src/rtimvMainWindow.hpp \
           src/rtimvInterfaces.hpp \
           src/imviewerControlPanel.h \
           src/imviewerstats.hpp \
           src/rtimvImage.hpp \
           src/shmimImage.hpp \
           src/pixaccess.h \
           src/colorMaps.hpp

SOURCES += src/imviewer.cpp \
           src/rtimvMainWindow.cpp \
           src/imviewer_main.cpp \
           src/imviewerControlPanel.cpp \
           src/imviewerstats.cpp \
           src/shmimImage.cpp
           
FORMS += forms/rtimvMainWindow.ui \
         forms/imviewerControlPanel.ui \
         forms/imviewerStats.ui 
           
LIBS += -lImageStreamIO
LIBS += -lcfitsio
LIBS += -lrtimv
LIBS += -lmxlib 
        
RESOURCES += res/imviewer.qrc

#########################
# installation 
#########################

unix:target.path = /usr/local/bin
INSTALLS += target

# unix:includefiles.path = /usr/local/include/rtimv
# includefiles.files = src/rtimvInterfaces.hpp src/rtimvGraphicsView.hpp
# INSTALLS += includefiles




