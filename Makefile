OS         = $(shell uname -s)
PROC       = $(shell uname -m | tr _ -)
HAVE       =-DHAVE_RMN -DHAVE_GPC
#-DHAVE_RPNC

INSTALL_DIR = $(shell readlink -f .)
TCL_SRC_DIR = ${SSM_DEV}/src/ext/tcl8.6.6

#----- Test for  availability
ifdef VGRID_VERSION
   HAVE := $(HAVE) -DHAVE_VGRID
endif

#----- Test for GDAL availability
ifneq ("$(shell which gdal-config)","")
   HAVE    := $(HAVE) -DHAVE_GDAL
   LIBS     = $(shell gdal-config --libs) $(shell nc-config --libs)
   INCLUDES = $(shell gdal-config --cflags) $(shell nc-config --cflags)                         
endif

#----- Test for Tcl availability
ifneq ("$(wildcard ${TCL_SRC_DIR})","")
   HAVE    := $(HAVE) -DHAVE_TCL
endif

LIBS        := -L/$(shell echo $(EC_LD_LIBRARY_PATH) | sed 's/\s* / -L/g') -L./lib $(LIBS) $(shell xml2-config --libs) -lrmn_eer 
INCLUDES    := -I/$(shell echo $(EC_INCLUDE_PATH) | sed 's/\s* / -I/g') $(INCLUDES) -Isrc -Iinclude $(shell xml2-config --cflags) -I$(TCL_SRC_DIR)/unix -I$(TCL_SRC_DIR)/generic $(INCLUDES)        

AR          = ar rv
LD          = ld -shared -x

#LINK_EXEC   = -lm -lpthread -Wl,-rpath=$(INSTALL_DIR)/lib
LINK_EXEC   = -lm -lpthread
ifdef INTEL_LICENSE_FILE
   CC=icc
   CXX=icpc
   FC=ifort
#   LINK_EXEC := $(LINK_EXEC) -Wl,-rpath $(INTELCOMP_HOME)/lib/intel64_lin -lintlc -lifcore -lifport 
   LINK_EXEC := $(LINK_EXEC) -lintlc -lifcore -lifport 
else
	#----- For vgrid
	LINK_EXEC := $(LINK_EXEC) -lgfortran
endif

CCOPTIONS   := -std=c99 -O0 -finline-functions -funroll-loops -fomit-frame-pointer
DEFINES     := -DVERSION=\"$(VERSION)-r$(BUILDINFO)\" -DBUILD_TIMESTAMP=\"$(shell date -u '+%Y-%m-%dT%H:%M:%SZ')\" -D_$(OS)_ -DTCL_THREADS -D_GNU_SOURCE ${HAVE}
ifdef OMPI
   #----- CRAY wraps MPI by default
   ifdef CRAYPE_VERSION
      CC=cc
   else
      CC= mpicc
   endif
   CCOPTIONS   := $(CCOPTIONS) -fopenmp
   DEFINES    := $(DEFINES) -D_MPI
endif

CDEBUGFLAGS = -g -Winline 
CPFLAGS     = -d 

ifeq ($(PROC),x86-64)
   CCOPTIONS   := $(CCOPTIONS) -fPIC -DSTDC_HEADERS
endif

CFLAGS      = $(CDEBUGFLAGS) $(CCOPTIONS) $(INCLUDES) $(DEFINES)

OBJ_C = $(subst .c,.o,$(wildcard src/*.c))
OBJ_F = $(subst .f,.o,$(wildcard src/*.f))
OBJ_F := $(subst .F90,.o,$(wildcard src/*.F90))
OBJ_V := $(shell ar t lib/libvgrid_eer.a)
OBJ_VG = $(OBJ_V:%=src/%)

%.o:%.F90
	s.compile -src $< -optf="-o $@"
#	gfortran $< $(CCOPTIONS) -c -o $@ -L./lib -lrmn_eer

all: obj lib exec

obj: $(OBJ_C) $(OBJ_F)

lib: obj
	mkdir -p ./lib
	mkdir -p ./include

        ifdef VGRID_VERSION
	   cd src; ar x ../lib/libvgrid_eer.a; cd -
        endif
	$(AR) lib/libeerUtils$(OMPI)-$(VERSION).a $(OBJ_C) $(OBJ_F) $(OBJ_VG)
	ln -fs libeerUtils$(OMPI)-$(VERSION).a lib/libeerUtils$(OMPI).a
#	$(CC) -shared -Wl,-soname,libeerUtils$(OMPI)-$(VERSION).so -o lib/libeerUtils$(OMPI)-$(VERSION).so $(OBJ_C) $(OBJ_F) $(OBJ_VG) $(CFLAGS) $(LIBS) $(LINK_EXEC)
         
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

clean:
	rm -f src/*.o src/*~ lib/libeerUtils$(OMPI)-$(VERSION).a

clear:	clean
	rm -fr bin lib include
