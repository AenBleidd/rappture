/*
 * bltChain.h --
 *
 * Copyright 1993-2000 Lucent Technologies, Inc.
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
#ifndef _RP_CHAIN_H
#define _RP_CHAIN_H

typedef struct Rp_ChainLinkStruct Rp_ChainLink;

/*
 * A Rp_ChainLink is the container structure for the Rp_Chain.
 */

struct Rp_ChainLinkStruct {
    Rp_ChainLink *prevPtr;  /* Link to the previous link */
    Rp_ChainLink *nextPtr;  /* Link to the next link */
    ClientData clientData;  /* Pointer to the data object */
};

typedef int (Rp_ChainCompareProc) _ANSI_ARGS_((Rp_ChainLink **l1PtrPtr, 
    Rp_ChainLink **l2PtrPtr));

/*
 * A Rp_Chain is a doubly chained list structure.
 */
typedef struct {
    Rp_ChainLink *headPtr;  /* Pointer to first element in chain */
    Rp_ChainLink *tailPtr;  /* Pointer to last element in chain */
    int nLinks;             /* Number of elements in chain */
} Rp_Chain;

extern void Rp_ChainInit _ANSI_ARGS_((Rp_Chain * chainPtr));
extern Rp_Chain *Rp_ChainCreate _ANSI_ARGS_(());
extern void Rp_ChainDestroy _ANSI_ARGS_((Rp_Chain * chainPtr));
extern Rp_ChainLink *Rp_ChainNewLink _ANSI_ARGS_((void));
extern Rp_ChainLink *Rp_ChainAllocLink _ANSI_ARGS_((unsigned int extraSize));
extern Rp_ChainLink *Rp_ChainAppend _ANSI_ARGS_((Rp_Chain * chainPtr,
    ClientData clientData));
extern Rp_ChainLink *Rp_ChainPrepend _ANSI_ARGS_((Rp_Chain * chainPtr,
    ClientData clientData));
extern void Rp_ChainReset _ANSI_ARGS_((Rp_Chain * chainPtr));
extern void Rp_ChainLinkAfter _ANSI_ARGS_((Rp_Chain * chainPtr,
    Rp_ChainLink * linkPtr, Rp_ChainLink * afterLinkPtr));
extern void Rp_ChainLinkBefore _ANSI_ARGS_((Rp_Chain * chainPtr,
    Rp_ChainLink * linkPtr, Rp_ChainLink * beforeLinkPtr));
extern void Rp_ChainUnlinkLink _ANSI_ARGS_((Rp_Chain * chainPtr,
    Rp_ChainLink * linkPtr));
extern void Rp_ChainDeleteLink _ANSI_ARGS_((Rp_Chain * chainPtr,
    Rp_ChainLink * linkPtr));
extern Rp_ChainLink *Rp_ChainGetNthLink _ANSI_ARGS_((Rp_Chain * chainPtr, int n));
extern void Rp_ChainSort _ANSI_ARGS_((Rp_Chain * chainPtr,
    Rp_ChainCompareProc * proc));

#define Rp_ChainGetLength(c)        (((c) == NULL) ? 0 : (c)->nLinks)
#define Rp_ChainFirstLink(c)        (((c) == NULL) ? NULL : (c)->headPtr)
#define Rp_ChainLastLink(c)         (((c) == NULL) ? NULL : (c)->tailPtr)
#define Rp_ChainPrevLink(l)         ((l)->prevPtr)
#define Rp_ChainNextLink(l)         ((l)->nextPtr)
#define Rp_ChainGetValue(l)         ((l)->clientData)
#define Rp_ChainSetValue(l, value)  ((l)->clientData = (ClientData)(value))
#define Rp_ChainAppendLink(c, l) \
    (Rp_ChainLinkBefore((c), (l), (Rp_ChainLink *)NULL))
#define Rp_ChainPrependLink(c, l) \
    (Rp_ChainLinkAfter((c), (l), (Rp_ChainLink *)NULL))

#endif /* _RP_CHAIN_H */
