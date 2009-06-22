/*
 * ======================================================================
 *  Rappture::Path
 *
 *  AUTHOR:  Derrick Kearney, Purdue University
 *
 *  Copyright (c) 2004-2009  Purdue Research Foundation
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef RAPPTURE_PATH_H
#define RAPPTURE_PATH_H

#include <RpOutcome.h>
#include <RpInt.h>
#include <RpChain.h>
#include <RpSimpleBuffer.h>


#ifdef __cplusplus
    extern "C" {
#endif // ifdef __cplusplus

namespace Rappture {

/**
 * Block of memory that dynamically resizes itself as needed.
 * The buffer also allows caller to massage the data it holds.
 * allows for easy loading of files.
 */

class Path {
public:
    Path();
    Path(const char* bytes);
    Path(const Path& buffer);
    /*
    Path& operator=(const Path& b);
    Path  operator+(const Path& b) const;
    Path& operator+=(const Path& b);
    */
    virtual ~Path();

    const char *ifs(const char *el);    // set/get comp separator
    Path& add(const char *el);  // turns input into input.number(blah)
    Path& del();                // turns input.number(blah) into input

    // turns input.number(blah) into number(blah)
    const char *component(void);    // get component of the path
    void component(const char *p);  // set component of the path

    // turns input.number(blah) into blah
    const char *id(void);       // get the id of the path
    void id(const char *p);     // set the id of the path

    // turns input.number(blah) into number
    const char *type(void);     // get the type of the path
    void type(const char *p);   // set the type of the path

    // turns input.number(blah) into input
    const char *parent(void);   // get the parent of the path
    void parent(const char *p); // set the parent of the path

    const char *path(void);     // get the path
    void path(const char *p);   // set the path

private:

    typedef struct {
        const char *type;
        const char *id;
    } componentStruct;

    void __pathInit();
    void __pathFree();
    void __updateBuffer();
    Rp_Chain *__parse(const char *p);
    componentStruct *__createComponent(const char *p, int start,
                        int end, int idOpenParen, int idCloseParen);
    void __deleteComponent(componentStruct *c);

    char _ifs;
    Rp_Chain *_pathList;
    SimpleCharBuffer b;
    SimpleCharBuffer tmpBuf;

};

} // namespace Rappture

#ifdef __cplusplus
    }
#endif // ifdef __cplusplus

#endif // RAPPTURE_PATH_H
