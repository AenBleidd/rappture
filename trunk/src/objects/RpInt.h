
/*
 * bltInt.h --
 *
 * Copyright 1993-1998 Lucent Technologies, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and warranty
 * disclaimer appear in supporting documentation, and that the names
 * of Lucent Technologies any of their entities not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 *
 * Lucent Technologies disclaims all warranties with regard to this
 * software, including all implied warranties of merchantability and
 * fitness.  In no event shall Lucent Technologies be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether in
 * an action of contract, negligence or other tortuous action, arising
 * out of or in connection with the use or performance of this
 * software.
 */

#ifndef _RP_INT_H
#define _RP_INT_H

// #define USE_NON_CONST

#include <stdio.h>
#include <assert.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <memory.h>
#include <unistd.h>
#include <limits.h>

// keep this commented until i fix the autoheader setup to
// automatically #define these
//
// #include "bltConfig.h"
//#ifdef HAVE_STDLIB_H
//#include <stdlib.h>
//#endif /* HAVE_STDLIB_H */
//
//#ifdef HAVE_STRING_H
//#include <string.h>
//#endif /* HAVE_STRING_H */
//
//#ifdef HAVE_ERRNO_H
//#include <errno.h>
//#endif /* HAVE_ERRNO_H */
//
//#ifdef HAVE_CTYPE_H
//#include <ctype.h>
//#endif /* HAVE_CTYPE_H */
//
//#ifdef HAVE_MEMORY_H
//#include <memory.h>
//#endif /* HAVE_MEMORY_H */
//
//#ifdef HAVE_UNISTD_H
//#include <unistd.h>
//#endif /* HAVE_UNISTD_H */
//
//#ifdef HAVE_LIMITS_H
//#include <limits.h>
//#endif

#define RP_OK       0
#define RP_ERROR    1
#define RP_RETURN   2
#define RP_BREAK    3
#define RP_CONTINUE 4

#ifndef _CLIENTDATA
#   ifndef NO_VOID
    typedef void *ClientData;
#   else
    typedef int *ClientData;
#   endif
#   define _CLIENTDATA

#ifndef NO_CONST
#   define CONST const
#else
#   define CONST
#endif

/*
#ifndef _ANSI_ARGS_
#   define _ANSI_ARGS_(x)       ()
#endif
*/

#ifndef _ANSI_ARGS_
#if defined(__STDC__) || defined(__cplusplus)
#   define _ANSI_ARGS_(x)       x
#else
#   define _ANSI_ARGS_(x)       ()
#endif
#endif


#endif
#undef INLINE
#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE
#endif
// #undef EXPORT
// #define EXPORT

#ifndef EXPORT
#define EXPORT
#endif

#undef EXTERN

#ifdef __cplusplus
#   define EXTERN extern "C" EXPORT
#else
#   define EXTERN extern EXPORT
#endif


#undef VARARGS
#ifdef __cplusplus
#define ANYARGS (...)
#define VARARGS(first)  (first, ...)
#define VARARGS2(first, second)  (first, second, ...)
#else
#define ANYARGS ()
#define VARARGS(first) ()
#define VARARGS2(first, second) ()
#endif /* __cplusplus */

#undef MIN
#define MIN(a,b)    (((a)<(b))?(a):(b))

#undef MAX
#define MAX(a,b)    (((a)>(b))?(a):(b))

#undef MIN3
#define MIN3(a,b,c) (((a)<(b))?(((a)<(c))?(a):(c)):(((b)<(c))?(b):(c)))

#undef MAX3
#define MAX3(a,b,c) (((a)>(b))?(((a)>(c))?(a):(c)):(((b)>(c))?(b):(c)))

#define TRUE    1
#define FALSE   0

/*
 * The macro below is used to modify a "char" value (e.g. by casting
 * it to an unsigned character) so that it can be used safely with
 * macros such as isspace.
 */
#define UCHAR(c) ((unsigned char) (c))

#undef panic
#define panic(mesg) Rp_Panic("%s:%d %s", __FILE__, __LINE__, (mesg))

/*
 * Since the Tcl/Tk distribution doesn't perform any asserts, dynamic
 * loading can fail to find the __assert function.  As a workaround,
 * we'll include our own.
 */
#undef  assert
#ifdef  NDEBUG
    #define assert(EX) ((void)0)
#else
    extern void Rp_Assert _ANSI_ARGS_((char *expr, char *file, int line));
    #ifdef __STDC__
        #define assert(EX) (void)((EX) || (Rp_Assert(#EX, __FILE__, __LINE__), 0))
    #else
        #define assert(EX) (void)((EX) || (Rp_Assert("EX", __FILE__, __LINE__), 0))
    #endif /* __STDC__ */

#endif /* NDEBUG */

/*
 * ----------------------------------------------------------------------
 *
 *  Assume we need to declare free if there's no stdlib.h or malloc.h
 *
 * ----------------------------------------------------------------------
 */
#if !defined(HAVE_STDLIB_H) && !defined(HAVE_MALLOC_H)
extern void free _ANSI_ARGS_((void *));
#endif

extern int Rp_DictionaryCompare _ANSI_ARGS_((char *s1, char *s2));

extern void Rp_Panic _ANSI_ARGS_((const char *format, ...));


/* ---------------------------------------------------------------- */

typedef int (QSortCompareProc) _ANSI_ARGS_((const void *, const void *));

/*
 * -------------------------------------------------------------------
 *
 * Point2D --
 *
 *	2-D coordinate.
 *
 * -------------------------------------------------------------------
 */
typedef struct {
    double x, y;
} Point2D;

/*
 * -------------------------------------------------------------------
 *
 * Point3D --
 *
 *	3-D coordinate.
 *
 * -------------------------------------------------------------------
 */
typedef struct {
    double x, y, z;
} Point3D;

/*
 * -------------------------------------------------------------------
 *
 * Segment2D --
 *
 *	2-D line segment.
 *
 * -------------------------------------------------------------------
 */
typedef struct {
    Point2D p, q;		/* The two end points of the segment. */
} Segment2D;

/*
 * -------------------------------------------------------------------
 *
 * Dim2D --
 *
 *	2-D dimension.
 *
 * -------------------------------------------------------------------
 */
typedef struct {
    short int width, height;
} Dim2D;

/*
 *----------------------------------------------------------------------
 *
 * Region2D --
 *
 *      2-D region.  Used to copy parts of images.
 *
 *----------------------------------------------------------------------
 */
typedef struct {
    int left, right, top, bottom;
} Region2D;

#define RegionWidth(r)      ((r)->right - (r)->left + 1)
#define RegionHeight(r)     ((r)->bottom - (r)->top + 1)

typedef struct {
    double left, right, top, bottom;
} Extents2D;

typedef struct {
    double left, right, top, bottom, front, back;
} Extents3D;

#define PointInRegion(e,x,y) \
    (((x) <= (e)->right) && ((x) >= (e)->left) && \
     ((y) <= (e)->bottom) && ((y) >= (e)->top))

#define PointInRectangle(r,x0,y0) \
    (((x0) <= (int)((r)->x + (r)->width - 1)) && ((x0) >= (int)(r)->x) && \
     ((y0) <= (int)((r)->y + (r)->height - 1)) && ((y0) >= (int)(r)->y))


#define Rp_Malloc malloc
#define Rp_Calloc calloc
#define Rp_Realloc realloc
#define Rp_Free free
#define Rp_Strdup strdup

#endif /*_RP_INT_H*/
