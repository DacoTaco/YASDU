TARGET_MODULE:=Dtpass
CC   = gcc
CFLAGS   = -Wall $(PKGFLAGS)
LDFLAGS  = $(PKGFLAGS)

#you need libfuse-dev for this to compile btw

PKGFLAGS = `pkg-config fuse --cflags --libs`

objects = aes.o driver.o main.o

all: build_driver

main.o : main.c driver.h aes.h

driver.o : driver.c driver.h aes.h

aes.o : aes.c aes.h

build_driver : $(objects)
	$(CC) $(objects) $(LDFLAGS) -o $(TARGET_MODULE)
	
clean:
	rm -f $(objects) $(TARGET_MODULE)

