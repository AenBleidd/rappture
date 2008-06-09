
#ifndef RPOP_H
#define RPOP_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 *---------------------------------------------------------------------------
 *
 * Rp_OpSpec --
 *
 * 	Structure to specify a set of operations for a Tcl command.
 *      This is passed to the Rp_GetOpFromObj procedure to look
 *      for a function pointer associated with the operation name.
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    const char *name;		/* Name of operation */
    int minChars;		/* Minimum # characters to disambiguate */
    void *proc;
    int minArgs;		/* Minimum # args required */
    int maxArgs;		/* Maximum # args required */
    const char *usage;		/* Usage message */
} Rp_OpSpec;

typedef enum {
    RP_OP_ARG0,		/* Op is the first argument. */
    RP_OP_ARG1,		/* Op is the second argument. */
    RP_OP_ARG2,		/* Op is the third argument. */
    RP_OP_ARG3,		/* Op is the fourth argument. */
    RP_OP_ARG4		/* Op is the fifth argument. */

} Rp_OpIndex;

#define RP_OP_BINARY_SEARCH	0
#define RP_OP_LINEAR_SEARCH	1

extern void *Rp_GetOpFromObj _ANSI_ARGS_((Tcl_Interp *interp, 
	int nSpecs, Rp_OpSpec *specs, int operPos, int objc, 
	Tcl_Obj *const *objv, int flags));

#ifdef __cplusplus
}
#endif
#endif /*RPOBJ_H*/
