CC = gcc
CXX = g++
CFLAGS = -O2 -Wall -Wextra -pedantic -std=gnu11 -fPIC
CXXFLAGS = -O2 -Wall -Wextra -pedantic -std=gnu++11 -fPIC 
LDFLAGS = -lcrypto


.PHONY: clean


all:
	$(MAKE) nandtool

.c.o:
	$(CXX) $(INCLUDE) $(LDFLAGS) -c $(CXXFLAGS) -o $@ $<

nandtool: xts_crypto.o main.o
	$(CXX) -o $@ $^ $(LDFLAGS)

xts_crypto.o : xts_crypto.h

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
