
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

all: rtimv

rtimv:
	$(QMAKE) -makefile librtimv.pro
	sudo $(MAKE) -f makefile.librtimv install
	$(QMAKE) -makefile rtimv.pro
	$(MAKE) -f makefile.rtimv

install: rtimv
	$(MAKE) -f makefile.librtimv install
	$(MAKE) -f makefile.rtimv install 

clean:
	rm -f obj/*.o *~
	rm -f moc/moc_* res/qrc_* forms/ui_*
	rm -f makefile.librtimv
	rm -f makefile.rtimv
	rm -f bin/librtimv.so
	rm -f bin/rtimv
