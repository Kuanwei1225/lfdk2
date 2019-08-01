CROSS_COMPILE		=
AS					=   $(CROSS_COMPILE)as
AR					=   $(CROSS_COMPILE)ar
CC					=   $(CROSS_COMPILE)gcc
CPP					=   $(CC) -E
LD					=   $(CROSS_COMPILE)ld
NM					=   $(CROSS_COMPILE)nm
OBJCOPY				=   $(CROSS_COMPILE)objcopy
OBJDUMP				=   $(CROSS_COMPILE)objdump
RANLIB				=   $(CROSS_COMPILE)ranlib
READELF				=   $(CROSS_COMPILE)readelf
SIZE				=   $(CROSS_COMPILE)size
STRINGS				=   $(CROSS_COMPILE)strings
STRIP				=   $(CROSS_COMPILE)strip

CFLAGS				=
LDFLAGS				=   -lpanel -lncurses -O0 -g
OBJS				=   lfdk
LIBS				=	lib/libio.c lib/libsio.c lib/libcmd.c lib/libmem.c
OUTPUT_PATH=bin


all:
ifeq "$(wildcard $(OUTPUT_PATH))" ""
	mkdir $(OUTPUT_PATH)
endif
	$(CC) $(CFLAGS) -o $(OBJS) $(OBJS).c $(LIBS) $(LDFLAGS)
	mv -f lfdk bin

clean:
	$(MAKE) -C lfdk clean
	rm -rf bin


