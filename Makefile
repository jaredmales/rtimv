QMAKE=/usr/lib/x86_64-linux-gnu/qt5/bin/qmake 
PRO_FILE=imviewer.pro

QWT_INCLUDE=/usr/local/qwt-5.2.1/include/
export QWT_INCLUDE
QWT_LIB=-L/usr/lib/libqwt-qt5  -lqwt-qt5 
export QWT_LIB
F2C_LIB=-lf2c
export F2C_LIB

LEVMAR_INC=$(HOME)/include
export LEVMAR_INC
LEVMAR_LIB=-L$(HOME)/lib -llevmar
export LEVMAR_LIB

#BLASPACK=-L/usr/local/atlas/lib -llapack -lf77blas -lcblas -latlas
BLASPACK=-L${MKLROOT}/lib/intel64 -Wl,--no-as-needed -lmkl_intel_lp64 -lmkl_gnu_thread -lmkl_core -lgomp -lpthread -lm -ldl
export BLASPACK

all: imviewerFull

imviewerFull:
	$(QMAKE) -makefile $(PRO_FILE)
	$(MAKE) -f makefile.rtimv

imviewer:
	$(MAKE) -f makefile.rtimv
	
ifeq ($(system),VISAO)
install: all
	install -d $(VISAO_ROOT)/bin --owner=$(AOSUP_USER) --group=$(AOSUP_GROUP)
	install imviewer $(VISAO_ROOT)/bin
endif

clean:
	rm -f obj/*.o *~
	rm -f moc/moc_* res/qrc_* forms/ui_*
	rm -f makefile.rtimv
	rm -f bin/rtimv

