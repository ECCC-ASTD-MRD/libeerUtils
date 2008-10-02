ARCH        = $(shell uname)
VERSION     = 0.1

CC          = cc
AR          = ar rv
LD          = ld -shared -x

INSTALL_DIR = /users/dor/afse/eer
TCL_DIR     = /data/cmoe/afsr005/Archive/tcl8.4.13

LIBS        = 
INCLUDES    = -I./ -I${ARMNLIB}/include -I${ARMNLIB}/include/${ARCH} -I$(TCL_DIR)/generic

CCOPTIONS   = -O2 -funroll-all-loops -finline-functions -fPIC -c99
CDEBUGFLAGS =
DEFINES     = -DVERSION=$(VERSION)
CFLAGS      = $(CDEBUGFLAGS) $(CCOPTIONS) $(INCLUDES) $(DEFINES)

OBJ_C = eerUtils.o tclUtils.o Vector.o Astro.o

all: obj lib

obj: $(OBJ_C)

lib:
	$(AR) libeerUtils-$(VERSION).a $(OBJ_C)  


install: all
	mkdir -p $(INSTALL_DIR)/lib/$(ARCH)
	mkdir -p $(INSTALL_DIR)/include
	cp *.a $(INSTALL_DIR)/lib/$(ARCH)
	cp .h $(INSTALL_DIR)/include

clean:
	rm -f *.o *~

clear:	clean
	rm -f *.so *.a
