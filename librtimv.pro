TEMPLATE = lib
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

MAKEFILE = makefile.librtimv

# Input
HEADERS += src/rtimvGraphicsView.hpp \
           src/StretchGraphicsItem.hpp \
           src/StretchBox.hpp \
           src/StretchCircle.hpp \
           src/StretchLine.hpp

SOURCES += src/rtimvGraphicsView.cpp \
           src/StretchBox.cpp \
           src/StretchCircle.cpp \
           src/StretchLine.cpp



#########################
# installation 
#########################

unix:target.path = /usr/local/lib
INSTALLS += target

unix:includefiles.path = /usr/local/include/rtimv
includefiles.files = src/rtimvInterfaces.hpp src/rtimvGraphicsView.hpp src/StretchGraphicsItem.hpp src/StretchBox.hpp src/StretchCircle.hpp src/StretchLine.hpp
INSTALLS += includefiles
