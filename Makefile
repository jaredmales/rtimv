
#############################
# The qt5 qmake 
#
# On a system where qt5 qmake is the one in the path, then no
# argument is needed.  If not, then invoke with, e.g., `make QMAKE=qmake-qt5`

QMAKE?=qmake

QMAKE_PATH := $(shell which qmake 2>/dev/null)

$(info $(QMAKE_PATH))

ifeq "$(QMAKE_PATH)" ""
  QMAKE=qmake-qt5
endif


##############################

all: librtimv rtimv

makefile.librtimv: librtimv.pro
	$(QMAKE) -makefile librtimv.pro

makefile.rtimv: rtimv.pro
	$(QMAKE) -makefile rtimv.pro

librtimv: makefile.librtimv
	$(MAKE) -f makefile.librtimv

rtimv: makefile.rtimv
	$(MAKE) -f makefile.rtimv

install: makefile.librtimv makefile.rtimv librtimv rtimv
	$(MAKE) -f makefile.librtimv install
	$(MAKE) -f makefile.rtimv install

clean:
	rm -f obj/*.o *~
	rm -f moc/moc_* res/qrc_* forms/ui_*
	rm -f makefile.librtimv
	rm -f makefile.rtimv
	rm -f bin/librtimv.so*
	rm -f bin/rtimv

.PHONY: all install clean librtimv rtimv
