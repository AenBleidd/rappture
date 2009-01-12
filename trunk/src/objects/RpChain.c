/*
 * bltChain.c --
 *
 *  The module implements a generic linked list package.
 *
 * Copyright 1991-1998 Lucent Technologies, Inc.
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

#include "RpInt.h"
#include "RpChain.h"

#ifndef ALIGN
#define ALIGN(a) \
    (((size_t)a + (sizeof(double) - 1)) & (~(sizeof(double) - 1)))
#endif /* ALIGN */

/*
 *----------------------------------------------------------------------
 *
 * Rp_ChainCreate --
 *
 *  Creates a new linked list (chain) structure and initializes 
 *  its pointers;
 *
 * Results:
 *  Returns a pointer to the newly created chain structure.
 *
 *----------------------------------------------------------------------
 */
Rp_Chain *
Rp_ChainCreate()
{
    Rp_Chain *chainPtr;

    chainPtr = (Rp_Chain *) Rp_Malloc(sizeof(Rp_Chain));
    if (chainPtr != NULL) {
        Rp_ChainInit(chainPtr);
    }
    return chainPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Rp_ChainAllocLink --
 *
 *  Creates a new chain link.  Unlink Rp_ChainNewLink, this 
 *  routine also allocates extra memory in the node for data.
 *
 * Results:
 *  The return value is the pointer to the newly created entry.
 *
 *----------------------------------------------------------------------
 */
Rp_ChainLink *
Rp_ChainAllocLink(extraSize)
    unsigned int extraSize;
{
    Rp_ChainLink *linkPtr;
    unsigned int linkSize;

    linkSize = ALIGN(sizeof(Rp_ChainLink));
    linkPtr = Rp_Calloc(1, linkSize + extraSize);
    assert(linkPtr);
    if (extraSize > 0) {
        /* Point clientData at the memory beyond the normal structure. */
        linkPtr->clientData = (ClientData)((char *)linkPtr + linkSize);
    }
    return linkPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Rp_ChainNewLink --
 *
 *  Creates a new link.
 *
 * Results:
 *  The return value is the pointer to the newly created link.
 *
 *----------------------------------------------------------------------
 */
Rp_ChainLink *
Rp_ChainNewLink()
{
    Rp_ChainLink *linkPtr;

    linkPtr = (Rp_ChainLink *) Rp_Malloc(sizeof(Rp_ChainLink));
    assert(linkPtr);
    linkPtr->clientData = NULL;
    linkPtr->nextPtr = linkPtr->prevPtr = NULL;
    return linkPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Rp_ChainReset --
 *
 *  Removes all the links from the chain, freeing the memory for
 *  each link.  Memory pointed to by the link (clientData) is not
 *  freed.  It's the caller's responsibility to deallocate it.
 *
 * Results:
 *  None.
 *
 *---------------------------------------------------------------------- 
 */
void
Rp_ChainReset(chainPtr)
    Rp_Chain *chainPtr;	/* Chain to clear */
{
    if (chainPtr != NULL) {
        Rp_ChainLink *oldPtr;
        Rp_ChainLink *linkPtr = chainPtr->headPtr;

        while (linkPtr != NULL) {
            oldPtr = linkPtr;
            linkPtr = linkPtr->nextPtr;
            Rp_Free(oldPtr);
        }
        Rp_ChainInit(chainPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Rp_ChainDestroy
 *
 *     Frees all the nodes from the chain and deallocates the memory
 *     allocated for the chain structure itself.  It's assumed that
 *     the chain was previous allocated by Rp_ChainCreate.
 *
 * Results:
 *      None.
 *
 *----------------------------------------------------------------------
 */
void
Rp_ChainDestroy(chainPtr)
    Rp_Chain *chainPtr;
{
    if (chainPtr != NULL) {
        Rp_ChainReset(chainPtr);
        Rp_Free(chainPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Rp_ChainInit --
 *
 *  Initializes a linked list.
 *
 * Results:
 *  None.
 *
 *----------------------------------------------------------------------
 */
void
Rp_ChainInit(chainPtr)
    Rp_Chain *chainPtr;
{
    chainPtr->nLinks = 0;
    chainPtr->headPtr = chainPtr->tailPtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Rp_ChainLinkAfter --
 *
 *  Inserts an entry following a given entry.
 *
 * Results:
 *  None.
 *
 *----------------------------------------------------------------------
 */
void
Rp_ChainLinkAfter(chainPtr, linkPtr, afterPtr)
    Rp_Chain *chainPtr;
    Rp_ChainLink *linkPtr, *afterPtr;
{
    if (chainPtr->headPtr == NULL) {
        chainPtr->tailPtr = chainPtr->headPtr = linkPtr;
    } else {
        if (afterPtr == NULL) {
            /* Prepend to the front of the chain */
            linkPtr->nextPtr = chainPtr->headPtr;
            linkPtr->prevPtr = NULL;
            chainPtr->headPtr->prevPtr = linkPtr;
            chainPtr->headPtr = linkPtr;
        } else {
            linkPtr->nextPtr = afterPtr->nextPtr;
            linkPtr->prevPtr = afterPtr;
            if (afterPtr == chainPtr->tailPtr) {
                chainPtr->tailPtr = linkPtr;
            } else {
                afterPtr->nextPtr->prevPtr = linkPtr;
            }
            afterPtr->nextPtr = linkPtr;
        }
    }
    chainPtr->nLinks++;
}

/*
 *----------------------------------------------------------------------
 *
 * Rp_ChainLinkBefore --
 *
 *  Inserts a link preceding a given link.
 *
 * Results:
 *  None.
 *
 *----------------------------------------------------------------------
 */
void
Rp_ChainLinkBefore(chainPtr, linkPtr, beforePtr)
    Rp_Chain *chainPtr;         /* Chain to contain new entry */
    Rp_ChainLink *linkPtr;      /* New entry to be inserted */
    Rp_ChainLink *beforePtr;    /* Entry to link before */
{
    if (chainPtr->headPtr == NULL) {
        chainPtr->tailPtr = chainPtr->headPtr = linkPtr;
    } else {
        if (beforePtr == NULL) {
            /* Append to the end of the chain. */
            linkPtr->nextPtr = NULL;
            linkPtr->prevPtr = chainPtr->tailPtr;
            chainPtr->tailPtr->nextPtr = linkPtr;
            chainPtr->tailPtr = linkPtr;
        } else {
            linkPtr->prevPtr = beforePtr->prevPtr;
            linkPtr->nextPtr = beforePtr;
            if (beforePtr == chainPtr->headPtr) {
                chainPtr->headPtr = linkPtr;
            } else {
                beforePtr->prevPtr->nextPtr = linkPtr;
            }
            beforePtr->prevPtr = linkPtr;
        }
    }
    chainPtr->nLinks++;
}

/*
 *----------------------------------------------------------------------
 *
 * Rp_ChainUnlinkLink --
 *
 *	Unlinks a link from the chain. The link is not deallocated, 
 *	but only removed from the chain.
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Rp_ChainUnlinkLink(chainPtr, linkPtr)
    Rp_Chain *chainPtr;
    Rp_ChainLink *linkPtr;
{
    int unlinked;       /* Indicates if the link is actually
                         * removed from the chain. */

    unlinked = FALSE;
    if (chainPtr->headPtr == linkPtr) {
        chainPtr->headPtr = linkPtr->nextPtr;
        unlinked = TRUE;
    }
    if (chainPtr->tailPtr == linkPtr) {
        chainPtr->tailPtr = linkPtr->prevPtr;
        unlinked = TRUE;
    }
    if (linkPtr->nextPtr != NULL) {
        linkPtr->nextPtr->prevPtr = linkPtr->prevPtr;
        unlinked = TRUE;
    }
    if (linkPtr->prevPtr != NULL) {
        linkPtr->prevPtr->nextPtr = linkPtr->nextPtr;
        unlinked = TRUE;
    }
    if (unlinked) {
        chainPtr->nLinks--;
    }
    linkPtr->prevPtr = linkPtr->nextPtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Rp_ChainDeleteLink --
 *
 *  Unlinks and also frees the given link.
 *
 * Results:
 *  None.
 *
 *----------------------------------------------------------------------
 */
void
Rp_ChainDeleteLink(chainPtr, linkPtr)
    Rp_Chain *chainPtr;
    Rp_ChainLink *linkPtr;
{
    Rp_ChainUnlinkLink(chainPtr, linkPtr);
    Rp_Free(linkPtr);
}

Rp_ChainLink *
Rp_ChainAppend(chainPtr, clientData)
    Rp_Chain *chainPtr;
    ClientData clientData;
{
    Rp_ChainLink *linkPtr;

    linkPtr = Rp_ChainNewLink();
    Rp_ChainLinkBefore(chainPtr, linkPtr, (Rp_ChainLink *)NULL);
    Rp_ChainSetValue(linkPtr, clientData);
    return linkPtr;
}

Rp_ChainLink *
Rp_ChainPrepend(chainPtr, clientData)
    Rp_Chain *chainPtr;
    ClientData clientData;
{
    Rp_ChainLink *linkPtr;

    linkPtr = Rp_ChainNewLink();
    Rp_ChainLinkAfter(chainPtr, linkPtr, (Rp_ChainLink *)NULL);
    Rp_ChainSetValue(linkPtr, clientData);
    return linkPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Rp_ChainGetNthLink --
 *
 *  Find the link at the given position in the chain.
 *
 * Results:
 *  Returns the pointer to the link, if that numbered link
 *  exists. Otherwise NULL.
 *
 *----------------------------------------------------------------------
 */
Rp_ChainLink *
Rp_ChainGetNthLink(chainPtr, position)
    Rp_Chain *chainPtr; /* Chain to traverse */
    int position;       /* Index of link to select from front
                         * or back of the chain. */
{
    Rp_ChainLink *linkPtr;

    if (chainPtr != NULL) {
        for (linkPtr = chainPtr->headPtr; linkPtr != NULL;
            linkPtr = linkPtr->nextPtr) {
            if (position == 0) {
                return linkPtr;
            }
            position--;
        }
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Rp_ChainSort --
 *
 *  Sorts the chain according to the given comparison routine.
 *
 * Results:
 *  None.
 *
 * Side Effects:
 *  The chain is reordered.
 *
 *----------------------------------------------------------------------
 */
void
Rp_ChainSort(chainPtr, proc)
    Rp_Chain *chainPtr;	/* Chain to traverse */
    Rp_ChainCompareProc *proc;
{
    Rp_ChainLink **linkArr;
    register Rp_ChainLink *linkPtr;
    register int i;

    if (chainPtr->nLinks < 2) {
        return;
    }
    linkArr = (Rp_ChainLink **) Rp_Malloc(sizeof(Rp_ChainLink *) * (chainPtr->nLinks + 1));
    if (linkArr == NULL) {
        return;         /* Out of memory. */
    }
    i = 0;
    for (linkPtr = chainPtr->headPtr; linkPtr != NULL;
         linkPtr = linkPtr->nextPtr) {
        linkArr[i++] = linkPtr;
    }
    qsort((char *)linkArr, chainPtr->nLinks, sizeof(Rp_ChainLink *),
        (QSortCompareProc *)proc);

    /* Rethread the chain. */
    linkPtr = linkArr[0];
    chainPtr->headPtr = linkPtr;
    linkPtr->prevPtr = NULL;
    for (i = 1; i < chainPtr->nLinks; i++) {
        linkPtr->nextPtr = linkArr[i];
        linkPtr->nextPtr->prevPtr = linkPtr;
        linkPtr = linkPtr->nextPtr;
    }
    chainPtr->tailPtr = linkPtr;
    linkPtr->nextPtr = NULL;
    Rp_Free(linkArr);
}
