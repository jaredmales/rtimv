#Seems necessary to give full path.  
#On some systems may be able to just make this qmake
QMAKE=/usr/lib/x86_64-linux-gnu/qt5/bin/qmake 

#Give path to ImageStreamIO includes
IMAGESTREAMIO_INCLUDE=
export IMAGESTREAMIO_INCLUDE

IMAGESTREAMIO_LIB=-lImageStreamIO
export IMAGESTREAMIO_LIB


all: imviewerFull

imviewerFull:
	$(QMAKE) -makefile imviewer.pro
	$(MAKE) -f makefile.rtimv

imviewer:
	$(MAKE) -f makefile.rtimv

clean:
	rm -f obj/*.o *~
	rm -f moc/moc_* res/qrc_* forms/ui_*
	rm -f makefile.rtimv
	rm -f bin/rtimv

