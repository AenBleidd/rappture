/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "BMPImageLoaderImpl.h"
#include "Image.h"
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>

BMPImageLoaderImpl::BMPImageLoaderImpl()
{
}

BMPImageLoaderImpl::~BMPImageLoaderImpl()
{
}

// I referred to cjbackhouse@hotmail.com                www.backhouse.tk
Image* BMPImageLoaderImpl::load(const char* fileName)
{
    printf("BMP loader\n");
    fflush(stdout);
    Image* image = 0;

    printf("opening image file \"%s\"\n", fileName);
    FILE* f = fopen(fileName, "rb");

    if (!f)  {
        printf("File not found\n");
        return 0;
    }

    char header[54];
    if (fread(&header, 54, 1, f) != 1) {
	printf("can't read header of BMP file\n");
	return 0;
    };

    if (header[0] != 'B' ||  header[1] != 'M') {
	printf("File is not BMP format\n");
	return 0;
    }

    //it seems gimp sometimes makes its headers small, so we have to do
    //this. hence all the fseeks
    int offset=*(unsigned int*)(header+10);
        
    const unsigned int width =*(int*) (header+18);
    const unsigned int height =*(int*) (header+22);

    int bits=int(header[28]);           //colourdepth

    printf("image width = %d height = %d bits=%d\n", width, height, bits);
    fflush(stdout);
    
    image = new Image(width, height, _targetImageFormat, 
		      Image::IMG_UNSIGNED_BYTE, 0);

    printf("image created\n");
    fflush(stdout);

    unsigned char* bytes = (unsigned char*) image->getImageBuffer();
    memset(bytes, 0, sizeof(unsigned char) * width * height * _targetImageFormat);

    printf("reset image buffer\n");
    fflush(stdout);

    unsigned int x,y;
    unsigned char cols[256*4];	//colourtable
    switch(bits) {
    case 24:
	fseek(f,offset,SEEK_SET);
	if (_targetImageFormat == Image::IMG_RGB) {
	    if (fread(bytes,width*height*3,1,f) != 1) {
		fprintf(stderr, "can't read image data\n");
	    }
	    for(x=0;x<width*height*3;x+=3)  { //except the format is BGR, grr
		unsigned char temp = bytes[x];
		bytes[x] = bytes[x+2];
		bytes[x+2] = temp;
	    }
	} else if (_targetImageFormat == Image::IMG_RGBA) {
	    char* buff = (char*) malloc(width * height * sizeof(unsigned char) * 3);
	    if (fread(buff,width*height*3,1,f) != 1) {
		fprintf(stderr, "can't read BMP image data\n");
	    }
	    for(x=0, y = 0;x<width*height*3;x+=3, y+=4)     {       //except the format is BGR, grr
		bytes[y] = buff[x+2];
		bytes[y+2] = buff[x];
		bytes[y+3] = 255;
	    }
	    free(buff);
	}
	break;
    case 32:
	fseek(f,offset,SEEK_SET);
	if (_targetImageFormat == Image::IMG_RGBA) {
	    if (fread(bytes,width*height*4,1,f) != 1) {
		fprintf(stderr, "can't read image data\n");
	    }
	    for(x=0;x<width*height*4;x+=4)  { //except the format is BGR, grr
		unsigned char temp = bytes[x];
		bytes[x] = bytes[x+2];
		bytes[x+2] = temp;
	    }
	} 
	else if (_targetImageFormat == Image::IMG_RGB) {
	    char* buff = (char*) malloc(width * height * sizeof(unsigned char) * 3);
	    if (fread(buff,width*height*4,1,f) != 1) {
		fprintf(stderr, "can't read BMP image data\n");
	    }
	    for(x=0, y = 0;x<width*height*4;x+=4, y+=3)     {       //except the format is BGR, grr
		bytes[y] = buff[x+2];
		bytes[y+2] = buff[x];
	    }
	    free(buff);
	}
	break;
    case 8:
	if (fread(cols,256 * 4,1,f) != 1) {
	    fprintf(stderr, "can't read colortable from BMP file\n");
	}
	fseek(f,offset,SEEK_SET);  
	for(y=0;y<height;++y) {	//(Notice 4bytes/col for some reason)
	    for(x=0;x<width;++x) {
		unsigned char byte;                 
		if (fread(&byte,1,1,f) != 1) {
		    fprintf(stderr, "error reading BMP file\n");
		}
		for(int c=0; c< 3; ++c) {
		    //bytes[(y*width+x)*3+c] = cols[byte*4+2-c];        //and look up in the table 
		    bytes[(y*width+x)*_targetImageFormat + c] = cols[byte*4+2-c];       //and look up in the table 
		}
	    } 
	}
	break;
	/*
	  case(4):
	  fread(cols,16*4,1,f);
	  fseek(f,offset,SEEK_SET);
	  for(y=0;y<256;++y)
	  for(x=0;x<256;x+=2)
	  {
	  BYTE byte;
	  fread(&byte,1,1,f);                                               //as above, but need to exract two
	  
	  for(c=0;c<_targetImageFormat;++c)                                         //pixels from each byte
	  bmp.pixel(x,y,c)=cols[byte/16*4+2-c];

	  for(c=0;c<_targetImageFormat;++c)
	  bmp.pixel(x+1,y,c)=cols[byte%16*4+2-c];
	*/
    }
    
    printf("image initialized\n");
    fflush(stdout);
    return image;
}
