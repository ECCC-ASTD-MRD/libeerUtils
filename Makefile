OS        = $(shell uname -s)
PROC      = $(shell uname -m)
#ARCH      = $(OS)_$(PROC)
ARCH      = $(OS)
VERSION     = 1.4

include Makefile.$(OS)

INSTALL_DIR = /users/dor/afsr/005
TCL_DIR     = /cnfs/ops/cmoe/afsr005/Archive/tcl8.5.7
EER_DIR     = /users/dor/afsr/005

ifeq ($(OS),Linux)

   LIBS = -L$(EER_DIR)/lib/$(ARCH) -lrmn -lpgc  
   LINK_EXEC = -lm -lpthread -Wl,-rpath,$(EER_DIR)/lib/$(ARCH) -shared 

   ifeq ($(PROC),x86_64)
        ARCH = $(OS)_$(PROC)
	INCLUDES    = -I./src -I$(ARMNLIB)/include -I$(ARMNLIB)/include/Linux_x86-64 -I$(TCL_DIR)/unix -I$(TCL_DIR)/generic 
   else
	INCLUDES    = -I./src -I$(ARMNLIB)/include -I$(ARMNLIB)/include/${ARCH} -I$(TCL_DIR)/unix -I$(TCL_DIR)/generic 
   endif
else
   INCLUDES    = -I./src -I$(ARMNLIB)/include -I$(ARMNLIB)/include/$(ARCH) 
   LIBS        = -L$(EER_DIR)/lib/$(ARCH) -L/home/ordenv/ssm-domains1/ssm-rmnlib-dev/multi/lib/AIX/xlf13 -lrmn_012
   LINK_EXEC   = -lxlf90 -lxlsmp -lc -lpthread -lmass
endif

DEFINES     = -DVERSION=$(VERSION) -D_$(OS)_  -DTCL_THREADS
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
#	$(CC) Utilities/EZVrInter.c -o bin/EZVrInter-$(VERSION) $(CFLAGS) -L./lib -leerUtils-$(VERSION) $(LIBS) -lm -lpthread -Wl,-rpath,$(EER_DIR)/lib/$(ARCH)

install: all
	mkdir -p $(INSTALL_DIR)/lib/$(ARCH)
	mkdir -p $(INSTALL_DIR)/include
	cp -d ./lib/* $(INSTALL_DIR)/lib/$(ARCH)
	cp -d ./bin/* $(INSTALL_DIR)/bin/$(ARCH)
	cp -d ./src/*.h $(INSTALL_DIR)/include

clean:
	rm -f src/*.o src/*~

clear:	clean
	rm -fr bin lib
