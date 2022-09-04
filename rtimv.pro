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

#MILK include path
unix::INCLUDEPATH += /usr/local/milk/include 
unix::LIBPATH += /usr/local/milk/lib/

#EIGEN include path 
unix::INCLUDEPATH += /usr/local/include/eigen3/

# Input
HEADERS += src/rtimvGraphicsView.hpp \
           src/rtimvBase.hpp \
           src/rtimvMainWindow.hpp \
           src/rtimvInterfaces.hpp \
           src/rtimvControlPanel.hpp \
           src/rtimvStats.hpp \
           src/rtimvImage.hpp \
           src/colorMaps.hpp \
           src/images/shmimImage.hpp \
           src/images/fitsImage.hpp \
           src/images/mzmqImage.hpp \
           src/images/pixaccess.hpp \
           src/images/ImageStruct.hpp 

SOURCES += src/rtimvBase.cpp \
           src/rtimvMainWindow.cpp \
           src/rtimvMain.cpp \
           src/rtimvControlPanel.cpp \
           src/rtimvStats.cpp \
           src/images/shmimImage.cpp \
           src/images/fitsImage.cpp \
           src/images/mzmqImage.cpp
           
FORMS += forms/rtimvMainWindow.ui \
         forms/imviewerControlPanel.ui \
         forms/imviewerStats.ui 
           
LIBS += -lImageStreamIO
LIBS += -lcfitsio
LIBS += -lrtimv
LIBS += -lmxlib 
LIBS += -lzmq
LIBS += -lxrif

RESOURCES += res/imviewer.qrc

#########################
# installation 
#########################

unix:target.path = /usr/local/bin
INSTALLS += target

# unix:includefiles.path = /usr/local/include/rtimv
# includefiles.files = src/rtimvInterfaces.hpp src/rtimvGraphicsView.hpp
# INSTALLS += includefiles




