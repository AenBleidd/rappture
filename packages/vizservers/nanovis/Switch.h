/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Switch.h --
 *
 *	Copyright 1993-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use,
 *	copy, modify, merge, publish, distribute, sublicense, and/or
 *	sell copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following
 *	conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the
 *	Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *	KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *	WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *	PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *	OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *	OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *	OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef NV_SWITCH_H
#define NV_SWITCH_H

#ifdef HAVE_STDDEF_H
#  include <stddef.h>
#endif /* HAVE_STDDEF_H */

namespace nv {

typedef int (SwitchParseProc)(ClientData clientData, Tcl_Interp *interp, 
                              const char *switchName, Tcl_Obj *valueObjPtr,
                              char *record, int offset,
                              int flags);

typedef void (SwitchFreeProc)(char *record, int offset, int flags);

typedef struct {
    SwitchParseProc *parseProc; /* Procedure to parse a switch
				 * value and store it in its *
				 * converted form in the data *
				 * record. */

    SwitchFreeProc *freeProc;	/* Procedure to free a switch. */

    ClientData clientData;	/* Arbitrary one-word value used by
				 * switch parser, passed to
				 * parseProc. */
} SwitchCustom;


/*
 * Type values for SwitchSpec structures.  See the user
 * documentation for details.
 */
typedef enum {
    SWITCH_BOOLEAN, 
    SWITCH_DOUBLE, 
    SWITCH_BITMASK, 
    SWITCH_BITMASK_NEG, 
    SWITCH_FLOAT, 
    SWITCH_INT, 
    SWITCH_INT_NNEG, 
    SWITCH_INT_POS,
    SWITCH_LIST, 
    SWITCH_LONG, 
    SWITCH_LONG_NNEG, 
    SWITCH_LONG_POS,
    SWITCH_OBJ,
    SWITCH_STRING, 
    SWITCH_VALUE, 
    SWITCH_CUSTOM, 
    SWITCH_END
} SwitchTypes;


typedef struct {
    SwitchTypes type;		/* Type of option, such as
				 * SWITCH_COLOR; see definitions
				 * below.  Last option in table must
				 * have type SWITCH_END. */

    const char *switchName;	/* Switch used to specify option in
				 * argv.  NULL means this spec is part
				 * of a group. */

    const char *help;		/* Help string. */
    int offset;			/* Where in widget record to store
				 * value; use Blt_Offset macro to
				 * generate values for this. */

    int flags;			/* Any combination of the values
				 * defined below. */

    unsigned int mask;

    SwitchCustom *customPtr;	/* If type is SWITCH_CUSTOM then
				 * this is a pointer to info about how
				 * to parse and print the option.
				 * Otherwise it is irrelevant. */
} SwitchSpec;

#define SWITCH_DEFAULTS		(0)  
#define SWITCH_ARGV_PARTIAL	(1<<1)
#define SWITCH_OBJV_PARTIAL	(1<<1)

/*
 * Possible flag values for Blt_SwitchSpec structures.  Any bits at or
 * above BLT_SWITCH_USER_BIT may be used by clients for selecting
 * certain entries.
 */
#define SWITCH_NULL_OK		(1<<0)
#define SWITCH_DONT_SET_DEFAULT	(1<<3)
#define SWITCH_SPECIFIED	(1<<4)
#define SWITCH_USER_BIT		(1<<8)

extern int ParseSwitches(Tcl_Interp *interp, SwitchSpec *specPtr, 
                         int objc, Tcl_Obj *const *objv,
                         void *rec, int flags);

extern void FreeSwitches(SwitchSpec *specs, void *rec, int flags);

extern int SwitchChanged TCL_VARARGS(SwitchSpec *, specs);
 
}

#endif
