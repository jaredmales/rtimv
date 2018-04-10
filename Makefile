#Seems necessary to give full path.  
#On some systems may be able to just make this qmake
QMAKE=/usr/lib/x86_64-linux-gnu/qt5/bin/qmake 

#Give path to CACAO includes
CACAO_INCLUDE=/home/jrmales/Source/CACAO/
export CACAO_INCLUDE

#Give path to QWT includes
QWT_INCLUDE=/usr/local/qwt-5.2.1/include/
export QWT_INCLUDE

#Give the link command for qwt
QWT_LIB=-L/usr/lib/libqwt-qt5  -lqwt-qt5 
export QWT_LIB

#Give path to levmar include
LEVMAR_INC=$(HOME)/include
export LEVMAR_INC

#Give the link command for levmar
LEVMAR_LIB=-L$(HOME)/lib -llevmar
export LEVMAR_LIB

#BLAS/LAPACK linking for ATLAS
#BLASPACK=-L/usr/local/atlas/lib -llapack -lf77blas -lcblas -latlas

#BLAS/LAPACK linking for MKL
BLASPACK=-L${MKLROOT}/lib/intel64 -Wl,--no-as-needed -lmkl_intel_lp64 -lmkl_gnu_thread -lmkl_core -lgomp -lpthread -lm -ldl
export BLASPACK


all: imviewerFull

imviewerFull:
	$(QMAKE) -makefile imviewer.pro
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

