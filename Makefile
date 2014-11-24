NAME       = eerUtils
DESC       = SMC-CMC-CMOE Utility librairie package.
VERSION    = 1.9.0
MAINTAINER = $(USER)
OS         = $(shell uname -s)
PROC       = $(shell uname -m | tr _ -)
RMN        = HAVE_RMN

ifdef COMP_ARCH
   COMP=-${COMP_ARCH}
endif

SSM_NAME    = ${NAME}_${VERSION}${COMP}_${ORDENV_PLAT}

INSTALL_DIR = $(HOME)
TCL_DIR     = /users/dor/afsr/005/Links/dev/Archive/tcl8.6.0
LIB_DIR     = /users/dor/afsr/005/Links/dev/Lib/${ORDENV_PLAT}
RMN_DIR     = $(LIB_DIR)/librmn-15

ifeq ($(OS),Linux)

   LIBS        = -L$(RMN_DIR)/lib -L$(LIB_DIR)/libxml2-2.9.1/lib
   INCLUDES    = -Isrc -I$(LIB_DIR)/libxml2-2.9.1/include/libxml2

#   CC          = mpicc
   CC          = /usr/bin/mpicc.openmpi
   CC          = s.cc
   AR          = ar rv
   LD          = ld -shared -x
   LIBS       := $(LIBS) -lxml2 -lz -lrmn
   INCLUDES   := $(INCLUDES) -I$(TCL_DIR)/unix -I$(TCL_DIR)/generic
   LINK_EXEC   = -lm -lpthread -Wl,-rpath=$(LIB_DIR)/libxml2-2.9.1/lib
 
   CCOPTIONS   = -std=c99 -O2 -finline-functions -funroll-loops -fomit-frame-pointer
   ifdef OMPI
      CCOPTIONS   := $(CCOPTIONS) -fopenmp -mpi
   endif

   CDEBUGFLAGS =
   CPFLAGS     = -d

   ifeq ($(PROC),x86-64)
        CCOPTIONS   := $(CCOPTIONS) -fPIC -m64 -DSTDC_HEADERS
	INCLUDES    := $(INCLUDES)
   endif
else

   LIBS        = -L$(RMN_DIR)/lib -L$(shell echo $(EC_LD_LIBRARY_PATH) | sed 's/\s* / -L/g')
   RMN_INCLUDE = -I/ssm/net/rpn/libs/15.0/aix-7.1-ppc7-64/include -I/ssm/net/rpn/libs/15.0/all/include -I/ssm/net/rpn/libs/15.0/all/include/AIX-powerpc7/
   INCLUDES    =  -Isrc $(RMN_INCLUDE) -I$(shell echo $(EC_INCLUDE_PATH) | sed 's/\s* / -I/g')  -I/usr/include/libxml2

   ifdef OMPI
      CC          = mpCC_r
   else
      CC          = xlc
   endif

   AR          = ar rv
   LD          = ld
   LIBS       := $(LIBS) -lrmn -lxml2 -lz
   INCLUDES   := $(INCLUDES)
   LINK_EXEC   = -lxlf90 -lxlsmp -lc -lpthread -lmass -lm 

   CCOPTIONS   = -std=c99 -O3 -qnohot -qstrict -Q -v -qkeyword=restrict -qcache=auto -qtune=auto -qarch=auto -qinline
   ifdef OMPI
      CCOPTIONS  := $(CCOPTIONS) -qsmp=omp -qthreaded -qlibmpi -qinline
   endif

   CDEBUGFLAGS =
   CPFLAGS     = -h
endif

DEFINES     = -DVERSION=\"$(VERSION)\" -D_$(OS)_ -DTCL_THREADS -D_GNU_SOURCE -D$(RMN)
ifdef OMPI
   DEFINES    := $(DEFINES) -D_MPI
endif

CFLAGS      = $(CDEBUGFLAGS) $(CCOPTIONS) $(INCLUDES) $(DEFINES)

OBJ_C = $(subst .c,.o,$(wildcard src/*.c))
OBJ_F = $(subst .f,.o,$(wildcard src/*.f))

%.o:%.f
#	gfortran $< $(CCOPTIONS) -c -o $@ -L$(LIB_DIR)/librmn-15/lib -lrmn
	s.compile -src $< -optf="-o $@"

all: obj lib exec

obj: $(OBJ_C) $(OBJ_F)

lib: obj
	mkdir -p ./lib
	mkdir -p ./include
	$(AR) lib/libeerUtils$(OMPI)-$(VERSION).a $(OBJ_C) $(OBJ_F)
	ln -fs libeerUtils$(OMPI)-$(VERSION).a lib/libeerUtils$(OMPI).a

#	@if test "$(OS)" != "AIX"; then \
#	   $(CC) -shared -Wl,-soname,libeerUtils-$(VERSION).so -o lib/libeerUtils$(OMPI)-$(VERSION).so $(OBJ_C) $(OBJ_T) $(CFLAGS) $(LIBS) $(LINK_EXEC); \
#	   ln -fs  libeerUtils$(OMPI)-$(VERSION).so lib/libeerUtils.so; \
#	fi
	cp $(CPFLAGS) ./src/*.h ./include

exec: obj 
	@if test "$(RMN)" = "HAVE_RMN"; then \
	   mkdir -p ./bin; \
	   $(CC) util/EZTiler.c -o bin/EZTiler-$(VERSION) $(CFLAGS) -L./lib -leerUtils-$(VERSION) $(LIBS) $(LINK_EXEC); \
	   ln -fs EZTiler-$(VERSION) bin/EZTiler; \
	   ln -fs EZTiler bin/o.eztiler; \
	   $(CC) util/CodeInfo.c -o bin/CodeInfo-$(VERSION) $(CFLAGS) -L./lib -leerUtils-$(VERSION) $(LIBS) $(LINK_EXEC);  \
	   ln -fs CodeInfo-$(VERSION) bin/CodeInfo; \
	   ln -fs CodeInfo bin/o.codeinfo; \
	   $(CC) util/Dict.c -o bin/Dict-$(VERSION) $(CFLAGS) -L./lib -leerUtils-$(VERSION) $(LIBS) $(LINK_EXEC);  \
	   ln -fs Dict-$(VERSION) bin/Dict; \
	   ln -fs Dict bin/o.dict; \
	fi

test: obj 
	@if test "$(RMN)" = "HAVE_RMN"; then \
	   mkdir -p ./bin; \
	   $(CC) util/Test.c -o bin/Test $(CFLAGS) -L./lib -leerUtils-$(VERSION) $(LIBS) $(LINK_EXEC); \
	   $(CC) util/TestQTree.c -o bin/TestQTree $(CFLAGS) -L./lib -leerUtils-$(VERSION) $(LIBS) $(LINK_EXEC);  \
        fi

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
	cp $(CPFLAGS) $(RMN_DIR)/lib/* $(SSM_DEV)/workspace/$(SSM_NAME)/lib
	cp $(CPFLAGS) ./include/* $(SSM_DEV)/workspace/$(SSM_NAME)/include
	cp $(CPFLAGS) .ssm.d/post-install  $(SSM_DEV)/workspace/$(SSM_NAME)/.ssm.d
	sed -e 's/NAME/$(NAME)/' -e 's/VERSION/$(VERSION)/' -e 's/PLATFORM/$(ORDENV_PLAT)/' -e 's/MAINTAINER/$(MAINTAINER)/' -e 's/DESC/$(DESC)/' .ssm.d/control >  $(SSM_DEV)/workspace/$(SSM_NAME)/.ssm.d/control
	cd $(SSM_DEV)/workspace; tar -zcvf $(SSM_DEV)/package/$(SSM_NAME).ssm $(SSM_NAME)

clean:
	rm -f src/*.o src/*~

clear:	clean
	rm -fr bin lib include
