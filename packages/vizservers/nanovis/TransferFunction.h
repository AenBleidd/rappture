/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * ColorMap.h: color map class contains an array of (RGBA) values 
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef _TRANSFER_FUNCTION_H_
#define _TRANSFER_FUNCTION_H_


#include "Texture1D.h"
#include <R2/R2Object.h>


class TransferFunction : public R2Object {
    int _size;			//the resolution of the color map, how many
				//(RGBA) quadraples
    float* _data;
    Texture1D* _tex;		//the texture storing the colors 
    const char *_name;
    GLuint _id;			//OpenGL's texture identifier
protected :
    ~TransferFunction();
public:
    TransferFunction(int size, float *data);
    void update(float *data);
    void update(int size, float *data);
    GLuint id(void) {
	return _id;
    }
    void id(GLuint id) {
	_id = id;
    }
    Texture1D* getTexture(void) {
	return _tex;
    }
    float* getData(void) { 
	return _data; 
    }
    int getSize() const {
	return _size;
    }
    const char *name(void) const {
	return _name;
    }
    void name(const char *name) {
	_name = name;
    }
};

#endif
