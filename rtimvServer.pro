TEMPLATE = app
TARGET = rtimvServer
DESTDIR = bin/
DEPENDPATH += src/

MOC_DIR = moc/
OBJECTS_DIR = obj/
RCC_DIR = res/
UI_DIR = forms/

CONFIG(release, debug|release) {
    CONFIG += optimize_full
}

QMAKE_CXXFLAGS += -std=c++20

CONFIG += -O2

QT += widgets
QT += core
QT += network
QT -= gui

MAKEFILE = makefile.rtimvServer

DEFINES += RTIMVBASE_QBASET_QOBJECT
# Input
HEADERS += src/rtimvBaseObject.hpp \
           src/rtimvBase.hpp \
           src/rtimvServerThread.hpp \
           src/rtimvServer.hpp \
           src/rtimvImage.hpp \
           src/colorMaps.hpp \
           src/images/shmimImage.hpp \
           src/images/fitsImage.hpp \
           src/images/fitsDirectory.hpp \
           src/images/mzmqImage.hpp \
           src/images/pixaccess.hpp \
           src/proto/rtimv.grpc.pb.h

SOURCES += src/rtimvBaseObject.cpp \
           src/rtimvBase.cpp \
           src/rtimvServerThread.cpp \
           src/rtimvServer.cpp \
           src/rtimvServerMain.cpp \
           src/images/shmimImage.cpp \
           src/images/fitsImage.cpp \
           src/images/fitsDirectory.cpp \
           src/images/mzmqImage.cpp \
           src/proto/rtimv.grpc.pb.cc \
           src/proto/rtimv.pb.cc

unix:!macx {
    $$system(which milk, blob, which_milk_exit_code)
    equals(which_milk_exit_code, 0) {
        LIBS += -lImageStreamIO
        DEFINES += RTIMV_MILK
    }
}

#MILK include path
unix::INCLUDEPATH += /usr/local/milk/include
unix::INCLUDEPATH += /usr/include
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

#grpc
LIBS += -lgrpc++ -lgpr -labsl_synchronization -lprotobuf -lgrpc++_reflection

RESOURCES += res/imviewer.qrc

#########################
# installation
#########################

unix:target.path = /usr/local/bin
INSTALLS += target
