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

CONFIG += c++17

CONFIG += -O3

QT += widgets

MAKEFILE = makefile.rtimv

# Input
HEADERS += src/rtimvGraphicsView.hpp \
           src/rtimvBase.hpp \
           src/rtimvMainWindow.hpp \
           src/rtimvInterfaces.hpp \
           src/rtimvControlPanel.hpp \
           src/cubeCtrl.hpp \
           src/rtimvStats.hpp \
           src/rtimvImage.hpp \
           src/colorMaps.hpp \
           src/images/shmimImage.hpp \
           src/images/fitsImage.hpp \
           src/images/fitsDirectory.hpp \
           src/images/mzmqImage.hpp \
           src/images/pixaccess.hpp \
           src/images/ImageStruct.hpp

SOURCES += src/rtimvBase.cpp \
           src/rtimvMainWindow.cpp \
           src/rtimvMain.cpp \
           src/rtimvControlPanel.cpp \
           src/cubeCtrl.cpp \
           src/rtimvStats.cpp \
           src/images/shmimImage.cpp \
           src/images/fitsImage.cpp \
           src/images/fitsDirectory.cpp \
           src/images/mzmqImage.cpp

FORMS += forms/rtimvMainWindow.ui \
         forms/imviewerControlPanel.ui \
         forms/imviewerStats.ui \
         forms/cubeCtrl.ui

unix:!macx {
    $$system(which milk, blob, which_milk_exit_code)
    equals(which_milk_exit_code, 0) {
        LIBS += -lImageStreamIO
        DEFINES += RTIMV_MILK
    }
}

#MILK include path
unix::INCLUDEPATH += /usr/local/milk/include
unix::LIBPATH += /usr/local/milk/lib/


CONFIG += link_pkgconfig

#CFITSIO
PKGCONFIG += cfitsio

#EIGEN
PKGCONFIG += eigen3

packagesExist(libzmq) {
   PKGCONFIG += libzmq
} else {
   LIBS += -lzmq
}

LIBS += -lmxlib
LIBS += -lxrif
LIBS += -L./bin -lrtimv

RESOURCES += res/imviewer.qrc

#########################
# installation
#########################

unix:target.path = /usr/local/bin
INSTALLS += target
