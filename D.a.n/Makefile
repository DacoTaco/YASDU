TARGET_MODULE:=Dan
CC   = gcc
CFLAGS   = -Wall $(PKGFLAGS)
LDFLAGS  = $(PKGFLAGS)

#you need libfuse-dev for this to compile btw

PKGFLAGS = `pkg-config fuse --cflags --libs`

objects = aes.o driver.o main.o mount.o

all: build_driver

main.o : mount.o main.c

mount.o : driver.o fuse.h mount.c mount.h

driver.o : driver.c driver.h fuse.h aes.o

aes.o : aes.c aes.h

build_driver : $(objects)
	$(CC) $(objects) $(LDFLAGS) -o $(TARGET_MODULE)
	
clean:
	rm -f $(objects) $(TARGET_MODULE)

