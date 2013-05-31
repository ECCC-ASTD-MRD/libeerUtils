VERSION   = 1.6
OS        = $(shell uname -s)
PROC      = $(shell uname -m)
RMN       = HAVE_RMN

INSTALL_DIR = /users/dor/afsr/005
TCL_DIR     = /cnfs/ops/cmoe/afsr005/Archive/tcl8.6.0
EER_DIR     = /users/dor/afsr/005
MPICH_PATH  = /ssm/net/hpcs/ext/master/mpich2_1.5_ubuntu-10.04-amd64-64

INCLUDES    = -I./src -I$(EER_DIR)/include -I$(ARMNLIB)/include 

ifeq ($(OS),Linux)

   CC          = c99 
   CC          = mpicc
   AR          = ar rv
   LD          = ld -shared -x
   LIBS        = -L${MPICH_PATH}/lib -L$(EER_DIR)/lib/$(BASE_ARCH) -lrmn -lpgc  
   INCLUDES   := $(INCLUDES) -I${MPICH_PATH}/include -I$(TCL_DIR)/unix -I$(TCL_DIR)/generic -I$(ARMNLIB)/include/$(BASE_ARCH)
   LINK_EXEC   = -lm -lpthread -Wl,-rpath,$(EER_DIR)/lib/$(BASE_ARCH)  
   CCOPTIONS   = -std=c99 -O2 -finline-functions -funroll-loops -fopenmp
   CDEBUGFLAGS =
   CPFLAGS     = -d

   ifeq ($(PROC),x86_64)
        CCOPTIONS   := $(CCOPTIONS) -fPIC -m64 -DSTDC_HEADERS
	INCLUDES    := $(INCLUDES) -I$(ARMNLIB)/include/Linux_x86-64
   endif
else
   CC          = xlc
   CC          = mpCC_r
   AR          = ar rv
   LD          = ld
   LIBS        = -L$(EER_DIR)/lib/$(BASE_ARCH) -lrmnbeta_013
   INCLUDES   := $(INCLUDES) -I$(ARMNLIB)/include/AIX
   LINK_EXEC   = -lxlf90 -lxlsmp -lc -lpthread -lmass -lm
   CCOPTIONS   = -O3 -qnohot -qstrict -Q -v  -qkeyword=restrict -qsmp=omp -qcache=auto -qtune=auto -qarch=auto 
   CDEBUGFLAGS =
   CPFLAGS     = -h
endif

DEFINES     = -DVERSION=\"$(VERSION)\" -D_$(OS)_ -DTCL_THREADS -D_GNU_SOURCE -D_MPI -D$(RMN)
CFLAGS      = $(CDEBUGFLAGS) $(CCOPTIONS) $(INCLUDES) $(DEFINES)

OBJ_C = $(subst .c,.o,$(wildcard src/*.c))
OBJ_F = $(subst .f,.o,$(wildcard src/*.f))

%.o:%.f
#	gfortran -src $< "-o $@"
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

clean:
	rm -f src/*.o src/*~

clear:	clean
	rm -fr bin lib include
