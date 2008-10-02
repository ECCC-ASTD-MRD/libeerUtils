ARCH        = $(shell uname)
VERSION     = 0.1

CC          = cc
AR          = ar rv
LD          = ld -shared -x

INSTALL_DIR = /users/dor/afse/eer
TCL_DIR     = /data/cmoe/afsr005/Archive/tcl8.4.13
EER_DIR     = /users/dor/afse/eer
SPI_DIR     = /users/dor/afse/eer/eer_SPI-7.2.4a

LIBS        = -L$(SPI_DIR)/Shared/$(ARCH) -lrmn
INCLUDES    = -I./ -I${ARMNLIB}/include -I${ARMNLIB}/include/${ARCH} -I$(TCL_DIR)/generic

CCOPTIONS   = -O2 -funroll-all-loops -finline-functions -fPIC -c99
CDEBUGFLAGS =
DEFINES     = -DVERSION=$(VERSION)
CFLAGS      = $(CDEBUGFLAGS) $(CCOPTIONS) $(INCLUDES) $(DEFINES)

OBJ_C = eerUtils.o tclUtils.o Vector.o Astro.o EZTile.o

all: obj lib exec

obj: $(OBJ_C)

lib:
	mkdir -p ./lib
	$(AR) lib/libeerUtils-$(VERSION).a $(OBJ_C)

exec:
	mkdir -p ./bin
	$(CC) Utilities/EZTiler.c -o bin/EZTiler-$(VERSION) $(CFLAGS) -L./lib -leerUtils-$(VERSION) $(LIBS) -lm -lpthread -Wl,-rpath,$(SPI_DIR)/Shared/$(ARCH)

install: all
	mkdir -p $(INSTALL_DIR)/lib/$(ARCH)
	mkdir -p $(INSTALL_DIR)/include
	cp ./lib/* $(INSTALL_DIR)/lib/$(ARCH)
	cp ./bin/* $(INSTALL_DIR)/bin/$(ARCH)
	cp .h $(INSTALL_DIR)/include

clean:
	rm -f *.o *~

clear:	clean
	rm -fr bin lib
