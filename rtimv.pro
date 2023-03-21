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
           src/images/fitsDirectory.hpp \
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
           src/images/fitsDirectory.cpp \
           src/images/mzmqImage.cpp

FORMS += forms/rtimvMainWindow.ui \
         forms/imviewerControlPanel.ui \
         forms/imviewerStats.ui

unix:!macx {
    $$system(which milk, blob, which_milk_exit_code)
    equals(which_milk_exit_code, 0) {
        LIBS += -lImageStreamIO
        DEFINES += RTIMV_MILK
    }
}

CONFIG += link_pkgconfig
PKGCONFIG += cfitsio 

packagesExist(libzmq) {
   PKGCONFIG += libzmq
} else {
   LIBS += -lzmq 
}

_conda_prefix = $$(CONDA_PREFIX)
!isEmpty(_conda_prefix) {
    INCLUDEPATH += $$(CONDA_PREFIX)/include
    LIBS += -L$$(CONDA_PREFIX)/lib
}

LIBS += -lmxlib
LIBS += -lxrif
LIBS += -lrtimv

RESOURCES += res/imviewer.qrc

#########################
# installation
#########################

unix:target.path = /usr/local/bin
INSTALLS += target

!isEmpty(_conda_prefix) {
    unix:target.path = $$(CONDA_PREFIX)/bin
}
