# tiff32k
Conversion utilities between PNG and 32k color space TIFF commonly used on the [FM TOWNS](https://en.wikipedia.org/wiki/FM_Towns).

## Usage
`tiff32k2png input32k.tif output.png`

`png2tiff32k input.png output32k.tif`

## Build dependencies
- GNU make (run `make` to build the conversion utilities)
- [libtiff-4](https://download.osgeo.org/libtiff/)
- [libpng](http://www.libpng.org/pub/png/libpng.html)
- [zlib](https://zlib.net/)
- optionally pkg-config, or `CPPFLAGS` and `LIBS` can be set manually in `Makefile`
