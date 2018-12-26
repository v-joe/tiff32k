/***************************************
Convert PNG file to 32k color TIFF file
License: GPLv3
***************************************/

#include <tiffio.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>

const char libpngerr[] = "png I/O error\n";

int main(int argc, char* argv[])
{
    png_infop info_p;
    png_structp png_p;
    FILE *fp;
    TIFF *tif;
    int w,h;
    png_byte bits,colortype;

    if(argc!=3) {
        fprintf(stderr, "Usage: %s input.png output32k.tif\n", argv[0]);
        return 1;
    }

    fp = fopen(argv[1], "r");
    if(!fp) {
        fprintf(stderr, "Error: cannot open input png file %s.\n", argv[1]);
        return 1;
    }

    /* initialize PNG reader */

    png_p = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png_p) {
        fputs(libpngerr, stderr);
        fclose(fp);
        return 1;
    }

    info_p = png_create_info_struct(png_p);
    if(!info_p) {
        fputs(libpngerr, stderr);
        fclose(fp);
        png_destroy_read_struct(&png_p, (png_infopp) NULL, NULL);
        return 1;
    }

    if(setjmp(png_jmpbuf(png_p))) {
        fputs(libpngerr, stderr);
        fclose(fp);
        png_free_data(png_p, info_p, PNG_FREE_ALL, -1);
        png_destroy_read_struct(&png_p, &info_p, NULL);
        exit(1);
    }

    png_init_io(png_p, fp);

    png_read_info(png_p, info_p);

    /* get PNG color depth and dimensions, etc. */

    w = png_get_image_width(png_p, info_p);
    h = png_get_image_height(png_p, info_p);
    bits = png_get_bit_depth(png_p, info_p);
    colortype = png_get_color_type(png_p, info_p);

    /* make sure libpng converts PNG input to 32bit RGBA data when reading */

    if(bits==16) png_set_strip_16(png_p);
    if(colortype==PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_p);
    if(colortype==PNG_COLOR_TYPE_GRAY && bits<8)
        png_set_expand_gray_1_2_4_to_8(png_p);
    if(png_get_valid(png_p, info_p, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_p);
    if(colortype==PNG_COLOR_TYPE_GRAY || colortype==PNG_COLOR_TYPE_PALETTE
        || colortype==PNG_COLOR_TYPE_RGB) png_set_filler(png_p, 0xff, PNG_FILLER_AFTER);
    if(colortype==PNG_COLOR_TYPE_GRAY || colortype==PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_p);

    png_read_update_info(png_p, info_p);

    TIFFSetWarningHandler(NULL);

    /* open TIFF file for output */
    tif = TIFFOpen(argv[2], "wl");
    if(tif) {
        png_bytep ibuf;
        uint16 *obuf;
        short ts=16,ts2=1;

        ibuf = (png_bytep) malloc(4 * sizeof(png_byte) * w);
        obuf = _TIFFmalloc(w*ts);
        if(!ibuf || !obuf) {
            fputs("Error allocating memory.\n", stderr);
            if(obuf) _TIFFfree(obuf);
            TIFFClose(tif);
            fclose(fp);
            return 1;
        }

        if(setjmp(png_jmpbuf(png_p))) {
        fputs(libpngerr, stderr);
            if(obuf) _TIFFfree(obuf);
            TIFFClose(tif);
            fclose(fp);
            png_free_data(png_p, info_p, PNG_FREE_ALL, -1);
            png_destroy_read_struct(&png_p, &info_p, NULL);
            exit(1);
        }

        /* set TIFF dimensions, special 32k color space, etc. */

        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, w);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, h);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, ts);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, ts2);
        TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, h);
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
        TIFFSetField(tif, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tif, TIFFTAG_MAXSAMPLEVALUE, 32767);
        TIFFSetField(tif, TIFFTAG_XRESOLUTION, 150.);
        TIFFSetField(tif, TIFFTAG_YRESOLUTION, 150.);
        TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);

        /* 'cause there's nothing like a good quote in a comment... */
        TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION,
            "15-bit color depth TIFF (5 bits per channel) "
            "converted by png2tiff32k"
            "\n\nI will not buy this record, it is scratched.\n"
            "It's certainly uncontaminated by cheese.\n\n");

    for(uint32 row = 0; row < h; row++) {
        /* read a row from PNG */
            png_read_row(png_p, ibuf, NULL);
            /* convert 24bit RGB to 32k colors */
            for(size_t i=0;i<w;i++) {
                obuf[i] = (((uint16) (ibuf[4*i] & 248))   << 2)
                        + (((uint16) (ibuf[4*i+1] & 248)) << 7)
                        + (((uint16) (ibuf[4*i+2] >> 3)));
            }
            /* write row to TIFF */
            if(TIFFWriteScanline(tif, obuf, row, 0)==-1) {
                fputs("Error writing TIFF image data.\n", stderr);
                _TIFFfree(obuf);
                TIFFClose(tif);
                fclose(fp);
                png_free_data(png_p, info_p, PNG_FREE_ALL, -1);
                png_destroy_read_struct(&png_p, &info_p, NULL);
                return 1;
            }
        }

        _TIFFfree(ibuf);
        TIFFClose(tif);
        fclose(fp);
        png_free_data(png_p, info_p, PNG_FREE_ALL, -1);
        png_destroy_read_struct(&png_p, &info_p, NULL);
    }
    else
    {
        fprintf(stderr, "Error opening output TIFF file %s.\n", argv[2]);
        return 1;
    }

    return 0;
}
