/* 
A* -------------------------------------------------------------------
B* This file contains source code for the PyMOL computer program
C* copyright 1998-2000 by Warren Lyford Delano of DeLano Scientific. 
D* -------------------------------------------------------------------
E* It is unlawful to modify or remove this copyright notice.
F* -------------------------------------------------------------------
G* Please see the accompanying LICENSE file for further information. 
H* -------------------------------------------------------------------
I* Additional authors of this source file include:
-* 
-* 
-*
Z* -------------------------------------------------------------------
*/
#define _HAVE_LIBPNG /* _HAVE_LIBPNG not integrated into autotools build *NJK* */

#ifdef _HAVE_LIBPNG
#include<png.h>

 /* The png_jmpbuf() macro, used in error handling, became available in
  * libpng version 1.0.6.  If you want to be able to run your code with older
  * versions of libpng, you must define the macro yourself (but only if it
  * is not already defined by libpng!).
  */

#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

#endif

#include"os_predef.h"
#include"os_std.h"

#include"Base.h"
#include "MyPNG.h"
#include"MemoryDebug.h"
#include "Setting.h"


/* Persistent image capture buffer *NJK* */
/* we make image buffer a pymol global to reduce time spent */
/* allocating and reallocating memory for image captures    */
struct _CImage {
    void *data;
    int   allocated;
    int   used;
};

int ImageInit(PyMOLGlobals *G)
{
   register CImage *I=NULL;
   if( (I=(G->Image=Calloc(CImage,1)))) {
 
     UtilZeroMem(I,sizeof(CImage));
     I->data = NULL;
	 I->used = 0;
	 I->allocated = 0;
     return 1;
   } else {
     return 0;
   }
}

/* PNG stdout support *NJK */
/* add ability to write PNG files to standard output        */
/* we have to write to memory first so we can inform client */
/* how much data is going to be sent                        */

static void
user_write_data(png_structp png_ptr, png_bytep buf, png_size_t length)
{
    voidp write_io_ptr = png_get_io_ptr(png_ptr);
    struct _CImage *data = write_io_ptr;
    int available;
	int needed;
	void *newdata;

	available = data->allocated - data->used;

	if (length > available) {
		needed = length - available;

		if (needed < 32768)
			needed = 32768;

		newdata = realloc(data->data, data->allocated + needed);

		if (newdata != NULL) {
		   data->data = newdata;
		   data->allocated += needed;
		}
		else
		    return;
	}

	memcpy((unsigned char *)data->data + data->used, buf, length);

	data->used += length;
    return;
}

static void
user_flush_data(png_structp png_ptr)
{
	return;
}

int MyPNGWrite(PyMOLGlobals *G,char *file_name,unsigned char *p,
               unsigned int width,unsigned int height,float dpi)
{
#ifdef _HAVE_LIBPNG

   FILE *fp = 0; /* PNG stdout support *NJK* */
   png_structp png_ptr;
   png_infop info_ptr;
	int bit_depth = 8;
	int bytes_per_pixel = 4;
   png_uint_32 k;
   png_byte *image = (png_byte*)p;
   png_bytep *row_pointers;

   row_pointers=Alloc(png_bytep,height);

   /* open the file */
   if (strcmp(file_name,"-.png") != 0) { /* PNG stdout support *NJK* */
      fp = fopen(file_name, "wb");
      if (fp == NULL) {
        return 0;
      } else if(feof(fp)) {
        fclose(fp);
	    return 0;
      }
   }
   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also check that
    * the library version is compatible with the one used at compile time,
    * in case we are using dynamically linked libraries.  REQUIRED.
    */
   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);

   if (png_ptr == NULL)
   {
      if (fp != NULL)  /* PNG stdout support *NJK* */
         fclose(fp);
      return 0;
   }

   /* Allocate/initialize the image information data.  REQUIRED */
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      if (fp != NULL)  /* PNG stdout support *NJK* */
          fclose(fp);
      png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
      return 0;
   }

   /* Set error handling.  REQUIRED if you aren't supplying your own
    * error handling functions in the png_create_write_struct() call.
    */
   if (setjmp(png_jmpbuf(png_ptr)))
   {
      /* If we get here, we had a problem reading the file */
      if (fp != NULL)  /* PNG stdout support *NJK* */
         fclose(fp);
      png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
      return 0;
   }

   /* set up the output control if you are using standard C streams */
   if (fp != NULL) /* PNG stdout support *NJK* */
      png_init_io(png_ptr, fp);
   else {
      G->Image->used = 0;
      png_set_write_fn(png_ptr, G->Image, user_write_data, user_flush_data);
   }

   /* Set the image information here.  Width and height are up to 2^31,
    * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
    * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
    * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
    * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
    * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
    * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
    */
   png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, PNG_COLOR_TYPE_RGB_ALPHA,
      PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

   if(dpi>0.0F) { /* only set resolution if dpi is positive */
     int dots_per_meter = (int)(dpi*39.3700787);
     png_set_pHYs(png_ptr, info_ptr, dots_per_meter, dots_per_meter, PNG_RESOLUTION_METER);
   }
   png_set_compression_level(png_ptr,1); /* PNG stdout support *NJK* */

   png_set_gamma(png_ptr, SettingGet(G,cSetting_png_screen_gamma), 
                 SettingGet(G,cSetting_png_file_gamma));


   /* Write the file header information.  REQUIRED */
   png_write_info(png_ptr, info_ptr);

   /* The easiest way to write the image (you may have a different memory
    * layout, however, so choose what fits your needs best).  You need to
    * use the first method if you aren't handling interlacing yourself.
    */
   for (k = 0; k < height; k++)
     row_pointers[(height-k)-1] = image + k*width*bytes_per_pixel;

   png_write_image(png_ptr, row_pointers);

   /* It is REQUIRED to call this to finish writing the rest of the file */
   png_write_end(png_ptr, info_ptr);

   /* clean up after the write, and free any memory allocated */
   png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

   /* close the file */
   if (fp != NULL) /* PNG stdout support *NJK* */
       fclose(fp);
   else {
       /* Sample counting patch *NJK */
       fprintf(stdout, "image follows: %d %d\n", G->Image->used, PyMOLEndPerf(G->GLQueryId));
       PyMOLStartPerf(G->GLQueryId);
       fwrite(G->Image->data,1,G->Image->used,stdout);
       fflush(stdout);
       G->Image->used = 0;
   }

	mfree(row_pointers);
   /* that's it */

   return 1;

#else
   return 0;
#endif
}

int MyPNGRead(char *file_name,unsigned char **p_ptr,unsigned int *width_ptr,unsigned int *height_ptr)
{

#ifdef _HAVE_LIBPNG

  FILE *png_file=NULL;
  png_struct    *png_ptr = NULL;
  png_info	*info_ptr = NULL;
  png_byte      buf[8];
  png_byte      *png_pixels = NULL;
  png_byte      **row_pointers = NULL;
  png_byte      *pix_ptr = NULL;
  png_uint_32   row_bytes=0;

  png_uint_32   width;
  png_uint_32   height;
  int           bit_depth;
  int           color_type;
  int           row, col;
  int           ret;
  int           i;
  int ok=true;
  unsigned char *p=NULL;
  double        file_gamma;

  if(!file_name)
    return 0;

   png_file = fopen(file_name, "rb");
   if (png_file == NULL)
     return 0;

   /* read and check signature in PNG file */
   ret = fread (buf, 1, 8, png_file);
   if (ret != 8)
     ok=false;
   
   if(ok) {
     ret = png_check_sig (buf, 8);
     if (!ret)
       ok=false;
   }
   /* create png and info structures */
   if(ok) {
     png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING,
                                       NULL, NULL, NULL);
     if (!png_ptr)
       ok=false;
   }
   
   if(ok) {
     info_ptr = png_create_info_struct (png_ptr);
     if (!info_ptr)
       ok=false;
   }
   
   if (setjmp (png_jmpbuf(png_ptr)))
     ok = false;
   
   if(ok) {
     /* set up the input control for C streams */
     png_init_io (png_ptr, png_file);
     png_set_sig_bytes (png_ptr, 8);  /* we already read the 8 signature bytes */
     
     /* read the file information */
     png_read_info (png_ptr, info_ptr);
     
     /* get size and bit-depth of the PNG-image */
     png_get_IHDR (png_ptr, info_ptr,
                   &width, &height, &bit_depth, &color_type,
                   NULL, NULL, NULL);
     
     /* set-up the transformations */

     if(color_type!=PNG_COLOR_TYPE_RGB_ALPHA) {
       png_set_expand(png_ptr);
       png_set_filler(png_ptr,0xFF,PNG_FILLER_AFTER);
     }
     
     if (color_type == PNG_COLOR_TYPE_GRAY ||
         color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
       png_set_gray_to_rgb (png_ptr);
     /* only if file has a file gamma, we do a correction */
     if (png_get_gAMA (png_ptr, info_ptr, &file_gamma))
       png_set_gamma (png_ptr, (double) 2.2, file_gamma);
     
     /* all transformations have been registered; now update info_ptr data,
      * get rowbytes and channels, and allocate image memory */
     
     png_read_update_info (png_ptr, info_ptr);
     
     /* get the new color-type and bit-depth (after expansion/stripping) */
     png_get_IHDR (png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
                   NULL, NULL, NULL);
     
     /* row_bytes is the width x number of channels x (bit-depth / 8) */
     row_bytes = png_get_rowbytes (png_ptr, info_ptr);
     if ((png_pixels = (png_byte *) malloc (row_bytes * height * sizeof (png_byte))) == NULL) {
       ok=false;
     }
   }
   
   if(ok) {
     
     if ((row_pointers = (png_byte **) malloc (height * sizeof (png_bytep))) == NULL)
       {
         png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
         free (png_pixels);
         png_pixels = NULL;
         ok=false;
       }
   }
   
   if(ok) {
     
     /* set the individual row_pointers to point at the correct offsets */
     for (i = 0; i < ((signed)height); i++)
       row_pointers[i] = png_pixels + i * row_bytes;
     
     /* now we can go ahead and just read the whole image */
     png_read_image (png_ptr, row_pointers);
     
     /* read rest of file, and get additional chunks in info_ptr - REQUIRED */
     png_read_end (png_ptr, info_ptr);
   }
   
   if(ok) {
     /* now reformat image into PyMOL format */
     
     p=(unsigned char*)mmalloc(4*width*height);
     if(!p)
       ok=false;
   }
   if(ok) {
     *(p_ptr)=p;
     *(width_ptr)=width;
     *(height_ptr)=height;
     
     for (row = 0; row < (signed)height; row++)
       {
         pix_ptr=row_pointers[(height-1)-row];
         for (col = 0; col < (signed)width; col++)
           {
             *p++=*pix_ptr++;
             *p++=*pix_ptr++;
             *p++=*pix_ptr++;
             *p++=*pix_ptr++;
           }
       }
     
   } 
   
   if (row_pointers != (unsigned char**) NULL)
     free (row_pointers);
   if (png_pixels != (unsigned char*) NULL)
     free (png_pixels);
   
   if(png_ptr) {
     png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp) NULL);
   }
   if(png_file)
     fclose(png_file);

   return(ok);
#else
   return (false);
#endif
  
} /* end of source */

/* BMP save image support *NJK* */
void
bmp_header_add_int(unsigned char* header, unsigned int pos, unsigned int data)
{ 
  header[pos++] = data & 0xff;
  header[pos++] = (data >> 8) & 0xff;
  header[pos++] = (data >> 16) & 0xff;
  header[pos++] = (data >> 24) & 0xff;
} 
 
void
bmp_header_add_short(unsigned char* header, unsigned int pos, unsigned short data)
{ 
  header[pos++] = data & 0xff;
  header[pos++] = (data >> 8) & 0xff;
} 
 
int MyBMPWrite(PyMOLGlobals *G,char *bmp,unsigned char *save_image,int width,int height,float dpi)
{ 
  unsigned char header[54];
  int pos = 0;
  int pad = 0;
  int fsize = 0;
  int row = 0;
  int col = 0;
  unsigned char *scr;
  unsigned char *src = save_image;
  unsigned char tmp; 
  struct _CImage *data = G->Image;
  
  if ((3 * width) % 4 > 0)
    pad = 4 - ((3 * width) % 4);
    
  header[pos++] = 'B';
  header[pos++] = 'M';
    
  /* file size in bytes */
  fsize = (3 * width + pad) * height + sizeof(header);
  bmp_header_add_int(header,pos,fsize);
  pos+=4;
  
  /* reserved value (must be 0) */
  bmp_header_add_int(header, pos, 0);
  pos+=4;
  
  /* offset in bytes to start of bitmap data */
  bmp_header_add_int(header, pos, sizeof(header));
  pos+=4;
  
  /* size of the BITMAPINFOHEADER */
  bmp_header_add_int(header, pos, 40);
  pos+=4;

  /* width of the image in pixels */
  bmp_header_add_int(header, pos, width);
  pos+=4;

  /* height of the image in pixels */
  bmp_header_add_int(header, pos, height);
  pos+=4;

  /* 1 plane */
  bmp_header_add_short(header, pos, 1);
  pos+=2;

  /* 24 bits/pixel */
  bmp_header_add_short(header, pos, 24);
  pos+=2;

  /* no compression */
  /* size of image for compression */

  bmp_header_add_int(header, pos, 0);
  pos+=4;
  bmp_header_add_int(header, pos, 0);
  pos+=4;

  /* x pixels per meter */
  /* y pixels per meter */
  bmp_header_add_int(header, pos, 0);
  pos+=4;
  bmp_header_add_int(header, pos, 0);
  pos+=4;

  /* number of colors used (0 = compute from bits/pixel) */
  /* number of important colors (0 = all colors important) */
  bmp_header_add_int(header, pos, 0);
  pos+=4;
  bmp_header_add_int(header, pos, 0);
  pos+=4;

  if (data->allocated < fsize) {
  	char *newdata;

	newdata = realloc(data->data, fsize);

	if (newdata) {
		data->data = newdata;
		data->allocated = fsize;
	}
  }

  if (data->allocated < fsize)
      return(0);

  scr = data->data;

  for (row=0; row < height; row++) {
    for (col=0; col < width; col++) {
      tmp = src[0];
      scr[0] = src[2];
      scr[1] = src[1];
      scr[2] = tmp;
      scr += 3;
      src += 4;
    }
    scr += pad;  /* skip over padding already in screen data */
  }

  if (strcmp(bmp,"-.bmp") == 0) {
    /* Sample counting patch *NJK* */
    fprintf(stdout,"image follows: %d %d\n",fsize, PyMOLEndPerf(G->GLQueryId));
    PyMOLStartPerf(G->GLQueryId);
    fwrite(header,1,54,stdout);
    fwrite(data->data,1,fsize-sizeof(header), stdout);
    fflush(stdout);
  }
  else {
    FILE *fp;

    fp = fopen(bmp,"wb");
    fwrite(header,1,54, fp);
    fwrite(data->data,1,fsize-sizeof(header), fp);
    fflush(fp);
    fclose(fp);
  }

  return(1);
}

