NAME       = eerUtils
DESC       = SMC-CMC-CMOE Utility librairie package.
VERSION    = 1.7
MAINTAINER = $(USER)
OS         = $(shell uname -s)
PROC       = $(shell uname -m | tr _ -)
RMN        = HAVE_RMN

SSM_NAME    = ${NAME}_${VERSION}_${ORDENV_PLAT}

INSTALL_DIR = $(HOME)
TCL_DIR     = /cnfs/ops/cmoe/afsr005/Archive/tcl8.6.0
LIB_DIR     = /cnfs/ops/cmoe/afsr005/Lib/${ORDENV_PLAT}

LIBS        = -L$(LIB_DIR)/librmn-14/lib -L$(shell echo $(EC_LD_LIBRARY_PATH) | sed 's/\s* / -L/g')
INCLUDES    = -Isrc -I$(shell echo $(EC_INCLUDE_PATH) | sed 's/\s* / -I/g')

ifeq ($(OS),Linux)

#   CC          = c99 
#   CC          = mpicc
   CC          = /usr/bin/mpicc.openmpi
#   CC          = s.cc
   AR          = ar rv
   LD          = ld -shared -x
#   LIBS       := $(LIBS) -lrmne -lpgc  
   LIBS       := $(LIBS) -lrmne
   INCLUDES   := $(INCLUDES) -I$(TCL_DIR)/unix -I$(TCL_DIR)/generic
   LINK_EXEC   = -lm -lpthread  
   CCOPTIONS   = -std=c99 -O2 -finline-functions -funroll-loops -fopenmp
#phi   CCOPTIONS   = -c99 -O2 -Mmpi=mpich2
   CDEBUGFLAGS =
   CPFLAGS     = -d

   ifeq ($(PROC),x86-64)
        CCOPTIONS   := $(CCOPTIONS) -fPIC -m64 -DSTDC_HEADERS
	INCLUDES    := $(INCLUDES)
   endif
else
#   CC          = xlc
   CC          = mpCC_r
   AR          = ar rv
   LD          = ld
   LIBS       := $(LIBS) -lrmne
   INCLUDES   := $(INCLUDES)
   LINK_EXEC   = -lxlf90 -lxlsmp -lc -lpthread -lmass -lm 
   CCOPTIONS   = -O3 -qnohot -qstrict -Q -v -qkeyword=restrict -qsmp=omp -qthreaded -qcache=auto -qtune=auto -qarch=auto -qlibmpi -qinline
   CDEBUGFLAGS =
   CPFLAGS     = -h
endif

DEFINES     = -DVERSION=\"$(VERSION)\" -D_$(OS)_ -DTCL_THREADS -D_GNU_SOURCE -D$(RMN) -D_MPI
CFLAGS      = $(CDEBUGFLAGS) $(CCOPTIONS) $(INCLUDES) $(DEFINES)

OBJ_C = $(subst .c,.o,$(wildcard src/*.c))
OBJ_F = $(subst .f,.o,$(wildcard src/*.f))

%.o:%.f
#	gfortran $< $(CCOPTIONS) -c -o $@ -L$(LIB_DIR)/librmn-14/lib -lrmn
	s.compile -src $< -optf="-o $@"

all: obj lib exec

obj: $(OBJ_C) $(OBJ_F)

lib:
	mkdir -p ./lib
	mkdir -p ./include
	$(AR) lib/libeerUtils-$(VERSION).a $(OBJ_C) $(OBJ_F)
	ln -fs libeerUtils-$(VERSION).a lib/libeerUtils.a
	cp $(CPFLAGS) ./src/*.h ./include

exec:
	@if test "$(RMN)" = "HAVE_RMN"; then \
	   mkdir -p ./bin; \
	   $(CC) Utilities/EZTiler.c -o bin/EZTiler-$(VERSION) $(CFLAGS) -L./lib -leerUtils-$(VERSION) $(LIBS) $(LINK_EXEC); \
	   ln -fs EZTiler-$(VERSION) bin/EZTiler; \
	   $(CC) Utilities/CodeInfo.c -o bin/CodeInfo-$(VERSION) $(CFLAGS) -L./lib -leerUtils-$(VERSION) $(LIBS) $(LINK_EXEC);  \
	   ln -fs CodeInfo-$(VERSION) bin/CodeInfo; \
	fi

install: all
	mkdir -p $(INSTALL_DIR)/lib/$(ORDENV_PLAT)
	mkdir -p $(INSTALL_DIR)/include
	cp $(CPFLAGS) ./lib/* $(INSTALL_DIR)/lib/$(ORDENV_PLAT)
	cp $(CPFLAGS) ./bin/* $(INSTALL_DIR)/bin/$(ORDENV_PLAT)
	cp $(CPFLAGS) ./include/* $(INSTALL_DIR)/include

ssm: all
	rm -f -r  $(SSM_DEV)/workspace/$(SSM_NAME) $(SSM_DEV)/package/$(SSM_NAME).ssm 
	mkdir -p $(SSM_DEV)/workspace/$(SSM_NAME)/.ssm.d  $(SSM_DEV)/workspace/$(SSM_NAME)/etc/profile.d $(SSM_DEV)/workspace/$(SSM_NAME)/lib $(SSM_DEV)/workspace/$(SSM_NAME)/include $(SSM_DEV)/workspace/$(SSM_NAME)/bin
	cp $(CPFLAGS) ./bin/* $(SSM_DEV)/workspace/$(SSM_NAME)/bin
	cp $(CPFLAGS) ./lib/* $(SSM_DEV)/workspace/$(SSM_NAME)/lib
	cp $(CPFLAGS) $(LIB_DIR)/librmn-14/lib/* $(SSM_DEV)/workspace/$(SSM_NAME)/lib
	cp $(CPFLAGS) ./include/* $(SSM_DEV)/workspace/$(SSM_NAME)/include
	cp $(CPFLAGS) .ssm.d/post-install  $(SSM_DEV)/workspace/$(SSM_NAME)/.ssm.d
	sed -e 's/NAME/$(NAME)/' -e 's/VERSION/$(VERSION)/' -e 's/PLATFORM/$(ORDENV_PLAT)/' -e 's/MAINTAINER/$(MAINTAINER)/' -e 's/DESC/$(DESC)/' .ssm.d/control >  $(SSM_DEV)/workspace/$(SSM_NAME)/.ssm.d/control
	cd $(SSM_DEV)/workspace; tar -zcvf $(SSM_DEV)/package/$(SSM_NAME).ssm $(SSM_NAME)

clean:
	rm -f src/*.o src/*~

clear:	clean
	rm -fr bin lib include
