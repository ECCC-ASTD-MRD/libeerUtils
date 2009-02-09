ARCH        = $(shell uname)
VERSION     = 1.1

include Makefile.$(ARCH)

INSTALL_DIR = /users/dor/afse/eer
TCL_DIR     = /data/cmoe/afsr005/Archive/tcl8.4.13
EER_DIR     = /users/dor/afse/eer

LIBS        = -L$(EER_DIR)/lib/$(ARCH) -lrmn
INCLUDES    = -I./src -I${ARMNLIB}/include -I${ARMNLIB}/include/${ARCH} -I$(TCL_DIR)/generic

DEFINES     = -DVERSION=$(VERSION) -D_$(ARCH)_
CFLAGS      = $(CDEBUGFLAGS) $(CCOPTIONS) $(INCLUDES) $(DEFINES)

OBJ_C = $(subst .c,.o,$(wildcard src/*.c))
OBJ_F = $(subst .f,.o,$(wildcard src/*.f))

%.o:%.f
	r.compile -src $< -optf="-o $@"

all: obj lib exec

obj: $(OBJ_C) $(OBJ_F)

lib:
	mkdir -p ./lib
	$(AR) lib/libeerUtils-$(VERSION).a $(OBJ_C) $(OBJ_F)
	ln -fs libeerUtils-$(VERSION).a lib/libeerUtils.a

exec:
	mkdir -p ./bin
	$(CC) Utilities/EZTiler.c -o bin/EZTiler-$(VERSION) $(CFLAGS) -L./lib -leerUtils-$(VERSION) $(LIBS) -lm -lpthread -Wl,-rpath,$(EER_DIR)/lib/$(ARCH)
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
