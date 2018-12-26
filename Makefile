all: png2tiff32k tiff32k2png

#if CPPFLAGS and LIBS are unspecified, we'll try to obtain them from pkg-config
#typical CPPFLAGS and LIBS (to be used to override pkg-config)
#CPPFLAGS = -I/usr/local/include
#for static libpng and libtiff
#LIBS = -L/usr/local/lib -lpng -ltiff -ljpeg -ljbig -llzma -lz
#for dynamic libpng and libtiff
#LIBS = -L/usr/local/lib -lpng -ltiff

#CPPFLAGS and LIBS not set, let's get them from pkg-config
CPPFLAGS ?= $(shell pkg-config --cflags libpng libtiff-4 zlib)
LIBS ?= $(shell pkg-config --libs libpng libtiff-4)

CFLAGS = -O2
CC = gcc

png2tiff32k: png2tiff32k.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o png2tiff32k png2tiff32k.c $(LIBS)

tiff32k2png: tiff32k2png.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o tiff32k2png tiff32k2png.c $(LIBS)

.phony clean:
	rm -f png2tiff32k tiff32k2png
