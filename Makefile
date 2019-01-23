NAME       = eerUtils
DESCRIPTION= SMC-CMC-CMOE Utility librairie package
SUMMARY    = Library of common functions used by various CCMEP tools (SPI, EER Models, ...)
VERSION    = 3.4.0
BUILDINFO  = $(shell HOME=/dev/null git describe --always)
MAINTAINER = $(USER)
OS         = $(shell uname -s)
PROC       = $(shell uname -m | tr _ -)
HAVE       =-DHAVE_RMN -DHAVE_GPC
#-DHAVE_RPNC

ifdef VGRID_PATH
   HAVE := $(HAVE) -DHAVE_VGRID
endif

ifdef COMP_ARCH
   COMP=-${COMP_ARCH}
endif

SSM_VERSION = ${VERSION}${COMP}
SSM_NAME    = ${NAME}_${SSM_VERSION}_${ORDENV_PLAT}

INSTALL_DIR = $(shell readlink -f .)
TCL_SRC_DIR = ${SSM_DEV}/src/tcl8.6.6

LIBS        := -L/$(shell echo $(EC_LD_LIBRARY_PATH) | sed 's/\s* / -L/g') -L./lib 
INCLUDES    := -I/$(shell echo $(EC_INCLUDE_PATH) | sed 's/\s* / -I/g') 

ifeq ($(OS),Linux)

   LIBS        := $(LIBS) $(shell xml2-config --libs) $(shell gdal-config --libs) $(shell nc-config --libs) -lrmn
   INCLUDES    := -Isrc $(shell xml2-config --cflags) $(shell gdal-config --cflags) $(shell nc-config --cflags) -I$(TCL_SRC_DIR)/unix -I$(TCL_SRC_DIR)/generic $(INCLUDES)                                 

   AR          = ar rv
   LD          = ld -shared -x
   
   LINK_EXEC   = -lm -lpthread -Wl,-rpath=$(INSTALL_DIR)/lib
   ifdef INTEL_LICENSE_FILE
      LINK_EXEC   := $(LINK_EXEC) -lintlc -lifcore -lifport
   endif
 
   CCOPTIONS   = -std=c99 -O2 -finline-functions -funroll-loops -fomit-frame-pointer -DHAVE_GDAL
   ifdef OMPI
      ifdef INTEL_LICENSE_FILE
         CCOPTIONS   := $(CCOPTIONS) -fopenmp -mpi
      else
         CCOPTIONS   := $(CCOPTIONS) -fopenmp
         CC= mpicc
      endif
   endif

   CDEBUGFLAGS = -g -Winline 
   CPFLAGS     = -d 

   ifeq ($(PROC),x86-64)
        CCOPTIONS   := $(CCOPTIONS) -fPIC -DSTDC_HEADERS
   endif
else

   LIBS        := $(LIBS) -lxml2 -lrmn
   RMN_INCLUDE = -I/ssm/net/rpn/libs/15.2/aix-7.1-ppc7-64/include -I/ssm/net/rpn/libs/15.2/all/include -I/ssm/net/rpn/libs/15.2/all/include/AIX-powerpc7 -I${VGRID_PATH}/include
   INCLUDES    := -Isrc $(RMN_INCLUDE) -I/usr/include/libxml2 -I$(LIB_DIR)/gdal-1.11.0/include $(INCLUDES) 

   ifdef OMPI
      CC          = mpCC_r
   else
      CC          = xlc
   endif

   AR          = ar rv
   LD          = ld
   LINK_EXEC   = -lxlf90 -lxlsmp -lpthread -lm 

   CCOPTIONS   = -std=c99 -O3 -qtls -qnohot -qstrict -Q -v -qkeyword=restrict -qcache=auto -qtune=auto -qarch=auto -qinline 
   ifdef OMPI
      CCOPTIONS  := $(CCOPTIONS) -qsmp=omp -qthreaded -qlibmpi -qinline
   endif

   CDEBUGFLAGS =
   CPFLAGS     = -h
endif

DEFINES     = -DVERSION=\"$(VERSION)-r$(BUILDINFO)\" -D_$(OS)_ -DTCL_THREADS -D_GNU_SOURCE ${HAVE}
ifdef OMPI
   DEFINES    := $(DEFINES) -D_MPI
endif

CFLAGS      = $(CDEBUGFLAGS) $(CCOPTIONS) $(INCLUDES) $(DEFINES)

OBJ_C = $(subst .c,.o,$(wildcard src/*.c))
OBJ_F = $(subst .f,.o,$(wildcard src/*.f))
OBJ_F := $(subst .F90,.o,$(wildcard src/*.F90))
OBJ_V := $(shell ar t ${VGRID_PATH}/lib/libdescrip.a)
OBJ_VG = $(OBJ_V:%=src/%)

%.o:%.F90
	s.compile -src $< -optf="-o $@"
#	gfortran $< $(CCOPTIONS) -c -o $@ -L./lib -lrmn

all: obj lib exec

obj: $(OBJ_C) $(OBJ_F)

lib: obj
	mkdir -p ./lib
	mkdir -p ./include

        ifdef VGRID_PATH
	   # Need to include vgrid archive
	   cd src; ar x ${VGRID_PATH}/lib/libdescrip.a; cd -
        endif
	$(AR) lib/libeerUtils$(OMPI)-$(VERSION).a $(OBJ_C) $(OBJ_F) $(OBJ_VG)
	ln -fs libeerUtils$(OMPI)-$(VERSION).a lib/libeerUtils$(OMPI).a

        #----- Need shared version for Cray
#        ifdef CRAYPE_VERSION
#	   $(CC) -shared -Wl,-soname,libeerUtils-$(VERSION).so -o lib/libeerUtils$(OMPI)-$(VERSION).so $(OBJ_C) $(OBJ_T) $(CFLAGS) $(LIBS)
#	   ln -fs  libeerUtils$(OMPI)-$(VERSION).so lib/libeerUtils$(OMPI).so
#	endif

	cp $(CPFLAGS) ./src/*.h ./include

exec: obj 
	mkdir -p ./bin
	$(CC) util/Dict.c -o bin/Dict-$(VERSION) $(CFLAGS) -L./lib -leerUtils-$(VERSION) $(LIBS) $(LINK_EXEC) 
	ln -fs Dict-$(VERSION) bin/Dict
	ln -fs Dict bin/o.dict
	$(CC) util/EZTiler.c -o bin/EZTiler-$(VERSION) $(CFLAGS) -L./lib -leerUtils-$(VERSION) $(LIBS) $(LINK_EXEC)
	ln -fs EZTiler-$(VERSION) bin/EZTiler
	ln -fs EZTiler bin/o.eztiler
	$(CC) util/CodeInfo.c -o bin/CodeInfo-$(VERSION) $(CFLAGS) -L./lib -leerUtils-$(VERSION) $(LIBS) $(LINK_EXEC) 
	ln -fs CodeInfo-$(VERSION) bin/CodeInfo
	ln -fs CodeInfo bin/o.codeinfo
	$(CC) util/ReGrid.c -o bin/ReGrid-$(VERSION) $(CFLAGS) -L./lib -leerUtils-$(VERSION) $(LIBS) $(LINK_EXEC)
	ln -fs ReGrid-$(VERSION) bin/ReGrid
	ln -fs ReGrid bin/o.regrid
#	$(CC) util/RPNC_Convert.c -o bin/RPNC_Convert-$(VERSION) $(CFLAGS) -L./lib -leerUtils-$(VERSION) $(LIBS) $(LINK_EXEC)
#	ln -fs RPNC_Convert-$(VERSION) bin/RPNC_Convert
#	ln -fs RPNC_Convert bin/o.rpnc_convert

test: obj 
	mkdir -p ./bin
	$(CC) util/Test.c -o bin/Test $(CFLAGS) -L./lib -leerUtils-$(VERSION) $(LIBS) $(LINK_EXEC)
	$(CC) util/TestQTree.c -o bin/TestQTree $(CFLAGS) -L./lib -leerUtils-$(VERSION) $(LIBS) $(LINK_EXEC)

install: 
	mkdir -p $(INSTALL_DIR)/bin/$(ORDENV_PLAT)
	mkdir -p $(INSTALL_DIR)/lib/$(ORDENV_PLAT)
	mkdir -p $(INSTALL_DIR)/include
	cp $(CPFLAGS) ./lib/* $(INSTALL_DIR)/lib/$(ORDENV_PLAT)
	cp $(CPFLAGS) ./bin/* $(INSTALL_DIR)/bin/$(ORDENV_PLAT)
	cp $(CPFLAGS) ./include/* $(INSTALL_DIR)/include

ssm:
	rm -f -r  $(SSM_DEV)/workspace/$(SSM_NAME) $(SSM_DEV)/package/$(SSM_NAME).ssm 
	mkdir -p $(SSM_DEV)/workspace/$(SSM_NAME)/.ssm.d  $(SSM_DEV)/workspace/$(SSM_NAME)/etc/profile.d $(SSM_DEV)/workspace/$(SSM_NAME)/lib $(SSM_DEV)/workspace/$(SSM_NAME)/include $(SSM_DEV)/workspace/$(SSM_NAME)/bin
	cp $(CPFLAGS) ./bin/* $(SSM_DEV)/workspace/$(SSM_NAME)/bin
	cp $(CPFLAGS) ./lib/* $(SSM_DEV)/workspace/$(SSM_NAME)/lib
	cp $(CPFLAGS) ./include/* $(SSM_DEV)/workspace/$(SSM_NAME)/include
	cp $(CPFLAGS) .ssm.d/post-install  $(SSM_DEV)/workspace/$(SSM_NAME)/.ssm.d
	sed -e 's/NAME/$(NAME)/' -e 's/VERSION/$(SSM_VERSION)/' -e 's/PLATFORM/$(ORDENV_PLAT)/' -e 's/MAINTAINER/$(MAINTAINER)/' -e 's/BUILDINFO/${BUILDINFO}/' -e 's/DESCRIPTION/$(DESCRIPTION)/' -e 's/SUMMARY/$(SUMMARY)/' .ssm.d/control.json >  $(SSM_DEV)/workspace/$(SSM_NAME)/.ssm.d/control.json
	cd $(SSM_DEV)/workspace; tar -zcvf $(SSM_DEV)/package/$(SSM_NAME).ssm $(SSM_NAME)
#	rm -f -r  $(SSM_DEV)/workspace/$(SSM_NAME)

clean:
	rm -f src/*.o src/*~

clear:	clean
	rm -fr bin lib include
