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
           src/pixaccess.h \
           src/colorMaps.hpp

SOURCES += src/imviewer.cpp \
           src/rtimvMainWindow.cpp \
           src/imviewer_main.cpp \
           src/imviewerControlPanel.cpp \
           src/imviewerstats.cpp \
           src/rtimvImage.cpp
           
FORMS += forms/rtimvMainWindow.ui \
         forms/imviewerControlPanel.ui \
         forms/imviewerStats.ui 
           
LIBS += -lImageStreamIO
LIBS += -lcfitsio
LIBS += -lrtimv
LIBS += -lmxlib -L$$(MKLROOT)/lib/intel64 -Wl,--no-as-needed -lmkl_intel_lp64 -lmkl_gnu_thread -lmkl_core -lgomp -lpthread -lm -ldl -lfftw3 -lfftw3f -lfftw3l -lfftw3q -lboost_system -lboost_filesystem -L/usr/local/cuda/lib64/ -lcudart -lcublas -lcufft -lcurand -lgsl -lxpa -llevmar -L /home/jrmales/lib -lsofa_c
        
RESOURCES += res/imviewer.qrc

#########################
# installation 
#########################

unix:target.path = /usr/local/bin
INSTALLS += target

# unix:includefiles.path = /usr/local/include/rtimv
# includefiles.files = src/rtimvInterfaces.hpp src/rtimvGraphicsView.hpp
# INSTALLS += includefiles




