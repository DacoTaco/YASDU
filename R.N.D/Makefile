CC = gcc
CFLAGS = -O2 -Wall -Wextra -pedantic -std=gnu11 -fPIC
LDFLAGS = 


.PHONY: clean


all:
	$(MAKE) nandtool

.c.o:
	$(CC) $(INCLUDE) -c $(CFLAGS) -o $@ $<

nandtool: aes.o main.o
	$(CC) -o $@ $^ $(LDFLAGS)
	
aes.o: aes.h

clean:
	rm -f *.o nandtool nandtool.exe
    
clean_full:
	rm -f *.o nandtool nandtool.exe

dist: clean_full
	$(eval nandtoolVER = $(shell grep '\bnandtool_VERSION\b' version.h \
		| cut -d' ' -f3 \
		| sed -e 's/"//g'))
	mkdir nandtool-$(nandtoolVER)
	cp -R *.c *.h config.mk.template Makefile README.md LICENSE nandtool-$(nandtoolVER)
	tar czf nandtool-$(nandtoolVER).tar.gz nandtool-$(nandtoolVER)
	rm -r nandtool-$(nandtoolVER)
