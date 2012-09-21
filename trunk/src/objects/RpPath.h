/*
 * ======================================================================
 *  Rappture::Path
 *
 *  AUTHOR:  Derrick Kearney, Purdue University
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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
    bool eof();

    Path& first();
    Path& prev();
    Path& next();
    Path& last();
    Path& clear();
    size_t count();

    // turns input.number(blah) into number(blah)
    const char *component(void);    // get component of the current component
    void component(const char *p);  // set component of the current component

    // turns input.number(blah) into blah
    const char *id(void);       // get the id of the current component
    void id(const char *p);     // set the id of the current component

    // turns input.number(blah) into number
    const char *type(void);     // get the type of the current component
    void type(const char *p);   // set the type of the current component

    // turns input.number(blah) into input
    const char *parent(void);   // get the parent of the current component
    void parent(const char *p); // set the parent of the current component

    // turns input.number2(blah) into 2
    size_t degree(void);        // get the degree of the current component
    void degree(size_t degree); // set the degree of the current component

    const char *path(void);     // get the path
    void path(const char *p);   // set the path

private:

    typedef struct {
        const char *type;
        const char *id;
        size_t degree;
    } componentStruct;

    void __pathInit();
    void __pathFree();
    void __updateBuffer();
    Rp_Chain *__parse(const char *p);
    componentStruct *__createComponent(const char *p, int start,
                        int end, int idOpenParen, int idCloseParen,
                        size_t degree);
    void __deleteComponent(componentStruct *c);

    char _ifs;
    Rp_Chain *_pathList;
    Rp_ChainLink *_currLink;
    SimpleCharBuffer b;
    SimpleCharBuffer tmpBuf;

};

} // namespace Rappture

#ifdef __cplusplus
    }
#endif // ifdef __cplusplus

#endif // RAPPTURE_PATH_H
