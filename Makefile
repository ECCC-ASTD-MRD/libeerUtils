NAME      = eerUtils
DESC      = SMC-CMC-CMOE Utility librairie package.
VERSION   = 1.6
OS        = $(shell uname -s)
PROC      = $(shell uname -m | tr _ -)
RMN       = HAVE_RMN

SSM_BASE    = /cnfs/dev/cmoe/afsr005/ssm
SSM_WORK    = $(SSM_BASE)/workspace/$(NAME)_$(VERSION)_${ORDENV_PLAT}

INSTALL_DIR = /users/dor/afsr/005
TCL_DIR     = /cnfs/ops/cmoe/afsr005/Archive/tcl8.6.0
EER_DIR     = /users/dor/afsr/005

LIBS        = -L$(EER_DIR)/lib/$(BASE_ARCH) -L$(shell echo $(EC_LD_LIBRARY_PATH) | sed 's/\s* / -L/g')
INCLUDES    = -Isrc -I$(shell echo $(EC_INCLUDE_PATH) | sed 's/\s* / -I/g')

ifeq ($(OS),Linux)

   CC          = c99 
   CC          = mpicc
#   CC          = s.cc
   AR          = ar rv
   LD          = ld -shared -x
   LIBS       := $(LIBS) -lrmnbetashared_013e -lpgc  
   INCLUDES   := $(INCLUDES) -I$(TCL_DIR)/unix -I$(TCL_DIR)/generic
   LINK_EXEC   = -lm -lpthread -Wl,-rpath,$(EER_DIR)/lib/$(BASE_ARCH)  
   CCOPTIONS   = -std=c99 -O2 -finline-functions -funroll-loops -fopenmp
#   CCOPTIONS   = -c99 -O2 -Mmpi=mpich2
   CDEBUGFLAGS =
   CPFLAGS     = -d

   ifeq ($(PROC),x86-64)
        CCOPTIONS   := $(CCOPTIONS) -fPIC -m64 -DSTDC_HEADERS
	INCLUDES    := $(INCLUDES)
   endif
else
   CC          = xlc
   CC          = mpCC_r
   AR          = ar rv
   LD          = ld
   LIBS       := $(LIBS) -lrmnbeta_013
   INCLUDES   := $(INCLUDES)
   LINK_EXEC   = -lxlf90 -lxlsmp -lc -lpthread -lmass -lm 
   CCOPTIONS   = -O3 -qnohot -qstrict -Q -v  -qkeyword=restrict -qsmp=omp -qcache=auto -qtune=auto -qarch=auto 
   CDEBUGFLAGS =
   CPFLAGS     = -h
endif

DEFINES     = -DVERSION=\"$(VERSION)\" -D_$(OS)_ -DTCL_THREADS -D_GNU_SOURCE -D$(RMN) -D_MPI
CFLAGS      = $(CDEBUGFLAGS) $(CCOPTIONS) $(INCLUDES) $(DEFINES)

OBJ_C = $(subst .c,.o,$(wildcard src/*.c))
OBJ_F = $(subst .f,.o,$(wildcard src/*.f))

%.o:%.f
#	gfortran $< $(CCOPTIONS) -c -o $@ -L$(EER_DIR)/lib/$(BASE_ARCH) -lrmn
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
	   ln -fs EZTile-$(VERSION) bin/EZTile; \
	   $(CC) Utilities/CodeInfo.c -o bin/CodeInfo-$(VERSION) $(CFLAGS) -L./lib -leerUtils-$(VERSION) $(LIBS) $(LINK_EXEC);  \
	   ln -fs CodeInfo-$(VERSION) bin/CodeInfo; \
	fi

install: all
	mkdir -p $(INSTALL_DIR)/lib/$(BASE_ARCH)
	mkdir -p $(INSTALL_DIR)/include
	cp $(CPFLAGS) ./lib/* $(INSTALL_DIR)/lib/$(BASE_ARCH)
	cp $(CPFLAGS) ./bin/* $(INSTALL_DIR)/bin/$(BASE_ARCH)
	cp $(CPFLAGS) ./include/* $(INSTALL_DIR)/include

ssm: all
	rm -f -r $(SSM_WORK) $(SSM_BASE)/package/$(NAME)_$(VERSION)_${ORDENV_PLAT}.ssm 
	mkdir -p $(SSM_WORK)/.ssm.d $(SSM_WORK)/etc/profile.d $(SSM_WORK)/lib  $(SSM_WORK)/include $(SSM_WORK)/bin
	cp $(CPFLAGS) ./bin/* $(SSM_WORK)/bin
	cp $(CPFLAGS) ./lib/* $(SSM_WORK)/lib
	cp $(CPFLAGS) ./include/* $(SSM_WORK)/include
	cp $(CPFLAGS) .ssm.d/post-install $(SSM_WORK)/.ssm.d
	sed -e 's/NAME/$(NAME)/' -e 's/VERSION/$(VERSION)/' -e 's/PLATFORM/$(ORDENV_PLAT)/' -e 's/MAINTAINER/$(USER)/' -e 's/DESC/$(DESC)/' .ssm.d/control > $(SSM_WORK)/.ssm.d/control
	cd $(SSM_BASE)/workspace; tar -zcvf $(SSM_BASE)/package/$(NAME)_$(VERSION)_${ORDENV_PLAT}.ssm $(NAME)_$(VERSION)_${ORDENV_PLAT}

clean:
	rm -f src/*.o src/*~

clear:	clean
	rm -fr bin lib include
