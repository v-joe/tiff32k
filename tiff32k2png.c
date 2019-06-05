/***************************************
Convert 32k color TIFF file to PNG file
License: GPLv3
https://github.com/v-joe/tiff32k
***************************************/

#include <tiffio.h>
#include <png.h>
#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>

const char libpngerr[] = "png I/O error\n";

int main(int argc, char* argv[])
{
    TIFF *tif;

    if(argc!=3) {
        fprintf(stderr, "Usage: %s input32k.tif output.png\n", argv[0]);
        return 1;
    }

    TIFFSetWarningHandler(NULL);

    /* open TIFF for reading */
    tif = TIFFOpen(argv[1], "r");
    if(tif) {
        png_infop info_p;
        png_structp png_p;
        FILE *fp;
        uint16 *ibuf,maxsamp;
        png_bytep obuf;
        uint32 w,h,row;
        short ts,ts2;
        png_text text;

        /* get TIFF file info */

        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &ts);
        TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &ts2);
        TIFFGetField(tif, TIFFTAG_MAXSAMPLEVALUE, &maxsamp);
        if(ts!=16 || ts2!=1) {
            fputs("Error: input TIFF isn't single-sample 16bit.\n", stderr);
            return 1;
        }
        if(maxsamp!=32767) {
            fputs("Error: input TIFF doesn't use 15 bits for color.\n", stderr);
            return 1;
        }
        if(TIFFScanlineSize(tif)!=w*2) {
            fputs("Error: TIFF scanline length != image width not supported\n", stderr);
            return 1;
        }

        ibuf = _TIFFmalloc(w*ts);
        obuf = (png_bytep) malloc(3 * sizeof(png_byte) * w);
        if(!ibuf || !obuf) {
            fputs("Error allocating memory.\n", stderr);
            if(ibuf) _TIFFfree(ibuf);
            TIFFClose(tif);
            return 1;
        }

        /* open PNG for output */
        fp = fopen(argv[2], "wb");
        if(!fp) {
            fprintf(stderr, "Error: cannot create output png file %s.\n", argv[2]);
            return 1;
        }

        /* initialize PNG writer */

        png_p = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if(!png_p) {
            fputs(libpngerr, stderr);
            fclose(fp);
            _TIFFfree(ibuf);
            TIFFClose(tif);
            return 1;
        }

        info_p = png_create_info_struct(png_p);
        if(!info_p) {
            fputs(libpngerr, stderr);
            fclose(fp);
            png_destroy_write_struct(&png_p, (png_infopp) NULL);
            _TIFFfree(ibuf);
            TIFFClose(tif);
            return 1;
        }

        if(setjmp(png_jmpbuf(png_p))) {
            fputs(libpngerr, stderr);
            fclose(fp);
            png_free_data(png_p, info_p, PNG_FREE_ALL, -1);
            png_destroy_write_struct(&png_p, &info_p);
            _TIFFfree(ibuf);
            TIFFClose(tif);
            exit(1);
        }

        png_init_io(png_p, fp);

        png_set_compression_level(png_p, Z_BEST_COMPRESSION);

        png_set_IHDR(png_p, info_p, w, h, 8, PNG_COLOR_TYPE_RGB,
            PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

        text.compression = PNG_TEXT_COMPRESSION_NONE;
        text.key = "Comment";
        /* 'cause there's nothing like a good quote in a comment... */
        text.text = "24-bit color depth PNG "
            "converted by tiff32k2png"
            "\n\nI will not buy this record, it is scratched.\n"
            "It's certainly uncontaminated by cheese.\n\n";
        png_set_text(png_p, info_p, &text, 1);
    
        png_write_info(png_p, info_p);

        for(row = 0; row < h; row++) {
            /* read row from TIFF */
            TIFFReadScanline(tif, ibuf, row, 0);
            /* scale 32k color space up to 24bit RGB */
            for(size_t i=0;i<w;i++) {
                obuf[3*i] = (uint16) (ibuf[i] & 992) >> 2;
                obuf[3*i+1] = (uint16) (ibuf[i] & 31744) >> 7;
                obuf[3*i+2] = (uint16) (ibuf[i] & 31) << 3;
            }
            /* write row to PNG */
            png_write_row(png_p, obuf);
        }

        png_write_end(png_p, NULL);

        fclose(fp);
        png_free_data(png_p, info_p, PNG_FREE_ALL, -1);
        png_destroy_write_struct(&png_p, &info_p);
        _TIFFfree(ibuf);
        TIFFClose(tif);
    }
    else
    {
        fprintf(stderr, "Error opening TIFF file %s.\n", argv[1]);
        return 1;
    }

    return 0;
}
