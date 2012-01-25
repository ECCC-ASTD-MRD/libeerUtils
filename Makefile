VERSION   = 1.4
OS        = $(shell uname -s)
PROC      = $(shell uname -m)

INSTALL_DIR = /users/dor/afsr/005
TCL_DIR     = /cnfs/ops/cmoe/afsr005/Archive/tcl8.5.7
EER_DIR     = /users/dor/afsr/005

ifeq ($(OS),Linux)

   CC          = c99 
   AR          = ar rv
   LD          = ld -shared -x
   LIBS        = -L$(EER_DIR)/lib/$(BASE_ARCH) -lrmn -lpgc  
   INCLUDES    = -I./src -I$(ARMNLIB)/include -I$(TCL_DIR)/unix -I$(TCL_DIR)/generic -I$(ARMNLIB)/include/$(BASE_ARCH)
   LINK_EXEC   = -lm -lpthread -Wl,-rpath,$(EER_DIR)/lib/$(BASE_ARCH)  
   CCOPTIONS   = -O2 -finline-functions -fomit-frame-pointer -funroll-loops
   CDEBUGFLAGS =

   ifeq ($(PROC),x86_64)
        CCOPTIONS   := $(CCOPTIONS) -fPIC -m64 -DSTDC_HEADERS
	INCLUDES    := $(INCLUDES) -I$(ARMNLIB)/include/Linux_x86-64
   endif
else
   CC          = xlc
   AR          = ar rv
   LD          = ld
   LIBS        = -L$(EER_DIR)/lib/$(BASE_ARCH) -lrmnbeta_013
   INCLUDES    = -I./src -I$(ARMNLIB)/include -I$(ARMNLIB)/include/AIX
   LINK_EXEC   = -lxlf90 -lxlsmp -lc -lpthread -lmass -lm
   CCOPTIONS   = -O3 -qstrict -qmaxmem=-1 -Q -v  -qkeyword=restrict -qcache=auto -qtune=auto -qarch=auto
   CDEBUGFLAGS =
endif

DEFINES     = -DVERSION=$(VERSION) -D_$(OS)_ -DTCL_THREADS -D_GNU_SOURCE
CFLAGS      = $(CDEBUGFLAGS) $(CCOPTIONS) $(INCLUDES) $(DEFINES)

OBJ_C = $(subst .c,.o,$(wildcard src/*.c))
OBJ_F = $(subst .f,.o,$(wildcard src/*.f))

%.o:%.f
#	gfortran -src $< "-o $@"
	s.compile -src $< -optf="-o $@" -librmn
#	r.compile -src $< -optf="-o $@"

all: obj lib exec

obj: $(OBJ_C) $(OBJ_F)

lib:
	mkdir -p ./lib
	$(AR) lib/libeerUtils-$(VERSION).a $(OBJ_C) $(OBJ_F)
	ln -fs libeerUtils-$(VERSION).a lib/libeerUtils.a

exec:
	mkdir -p ./bin
	$(CC) Utilities/EZTiler.c -o bin/EZTiler-$(VERSION) $(CFLAGS) -L./lib -leerUtils-$(VERSION) $(LIBS) $(LINK_EXEC) 

install: all
	mkdir -p $(INSTALL_DIR)/lib/$(BASE_ARCH)
	mkdir -p $(INSTALL_DIR)/include
	cp -d ./lib/* $(INSTALL_DIR)/lib/$(BASE_ARCH)
	cp -d ./bin/* $(INSTALL_DIR)/bin/$(BASE_ARCH)
	cp -d ./src/*.h $(INSTALL_DIR)/include

clean:
	rm -f src/*.o src/*~

clear:	clean
	rm -fr bin lib
