
#############################
# The qt5 qmake 
#
# On a system where qt5 qmake is the one in the bath, then no
# argument is needed.  If not, then invoke with, e.g., `make QMAKE=qmake-qt5`

QMAKE?=qmake

##############################


all: rtimv

rtimv:
	$(QMAKE) -makefile rtimv.pro
	$(MAKE) -f makefile.rtimv


clean:
	rm -f obj/*.o *~
	rm -f moc/moc_* res/qrc_* forms/ui_*
	rm -f makefile.rtimv
	rm -f bin/rtimv
