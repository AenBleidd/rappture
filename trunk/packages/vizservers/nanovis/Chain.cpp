/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Chain.cpp --
 *
 * The module implements a generic linked list package.
 *
 *	Copyright 1991-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining
 *	a copy of this software and associated documentation files (the
 *	"Software"), to deal in the Software without restriction, including
 *	without limitation the rights to use, copy, modify, merge, publish,
 *	distribute, sublicense, and/or sell copies of the Software, and to
 *	permit persons to whom the Software is furnished to do so, subject to
 *	the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include "Chain.h"

typedef int (QSortCompareProc) (const void *, const void *);

/*
 *----------------------------------------------------------------------
 *
 * Reset --
 *
 *	Removes all the links from the chain, freeing the memory for each
 *	link.  Memory pointed to by the link (clientData) is not freed.  It's
 *	the caller's responsibility to deallocate it.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------- 
 */
void
Chain::Reset() /* Chain to clear */
{
    ChainLink *oldPtr;
    ChainLink *linkPtr = _head;

    while (linkPtr != NULL) {
	oldPtr = linkPtr;
	linkPtr = linkPtr->Next();
	delete oldPtr;
    }
    Init();
}

/*
 *----------------------------------------------------------------------
 *
 * LinkAfter --
 *
 *	Inserts a link after another link.  If afterPtr is NULL, then the new
 *	link is prepended to the beginning of the chain.
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Chain::LinkAfter(ChainLink *linkPtr, ChainLink *afterPtr)
{
    if (_head == NULL) {
	_tail = _head = linkPtr;
    } else {
	if (afterPtr == NULL) {
	    /* Prepend to the front of the chain */
	    linkPtr->_next = _head;
	    linkPtr->_prev = NULL;
	    _head->_prev = linkPtr;
	    _head = linkPtr;
	} else {
	    linkPtr->_next = afterPtr->Next();
	    linkPtr->_prev = afterPtr;
	    if (afterPtr == _tail) {
		_tail = linkPtr;
	    } else {
		afterPtr->Next()->_prev = linkPtr;
	    }
	    afterPtr->_next = linkPtr;
	}
    }
    _nLinks++;
}

/*
 *----------------------------------------------------------------------
 *
 * LinkBefore --
 *
 *	Inserts a new link preceding a given link in a chain.  If beforePtr is
 *	NULL, then the new link is placed at the beginning of the list.
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Chain::LinkBefore(
    ChainLink *linkPtr,		/* New entry to be inserted */
    ChainLink *beforePtr)	/* Entry to link before */
{
    if (_head == NULL) {
	_tail = _head = linkPtr;
    } else {
	if (beforePtr == NULL) {
	    /* Append to the end of the chain. */
	    linkPtr->_next = NULL;
	    linkPtr->_prev = _tail;
	    _tail->_next = linkPtr;
	    _tail = linkPtr;
	} else {
	    linkPtr->_prev = beforePtr->Prev();
	    linkPtr->_next = beforePtr;
	    if (beforePtr == _head) {
		_head = linkPtr;
	    } else {
		beforePtr->Prev()->_next = linkPtr;
	    }
	    beforePtr->_prev = linkPtr;
	}
    }
    _nLinks++;
}

/*
 *----------------------------------------------------------------------
 *
 * Unlink --
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
Chain::Unlink(ChainLink *linkPtr)
{
    bool unlinked;		/* Indicates if the link is actually removed
				 * from the chain. */

    unlinked = false;
    if (_head == linkPtr) {
	_head = linkPtr->Next();
	unlinked = true;
    }
    if (_tail == linkPtr) {
	_tail = linkPtr->Prev();
	unlinked = true;
    }
    if (linkPtr->Next() != NULL) {
	linkPtr->Next()->_prev = linkPtr->Prev();
	unlinked = true;
    }
    if (linkPtr->Prev() != NULL) {
	linkPtr->Prev()->_next = linkPtr->Next();
	unlinked = true;
    }
    if (unlinked) {
	_nLinks--;
    }
    linkPtr->_prev = linkPtr->_next = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteLink --
 *
 *	Unlinks and frees the given link from the chain.  It's assumed that
 *	the link belong to the chain. No error checking is performed to verify
 *	this.
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Chain::DeleteLink(ChainLink *linkPtr)
{
    Unlink(linkPtr);
    delete linkPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Append --
 *
 *	Creates and new link with the given data and appends it to the end of
 *	the chain.
 *
 * Results:
 *	Returns a pointer to the link created.
 *
 *----------------------------------------------------------------------
 */
ChainLink *
Chain::Append(void *clientData)
{
    ChainLink *linkPtr;

    linkPtr = new ChainLink(clientData);
    LinkBefore(linkPtr, (ChainLink *)NULL);
    return linkPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Prepend --
 *
 *	Creates and new link with the given data and prepends it to beginning
 *	of the chain.
 *
 * Results:
 *	Returns a pointer to the link created.
 *
 *----------------------------------------------------------------------
 */
ChainLink  *
Chain::Prepend(void *clientData)
{
    ChainLink *linkPtr;

    linkPtr = new ChainLink(clientData);
    LinkAfter(linkPtr, (ChainLink *)NULL);
    return linkPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * GetNthLink --
 *
 *	Find the link at the given position in the chain.
 *
 * Results:
 *	Returns the pointer to the link, if that numbered link
 *	exists. Otherwise NULL.
 *
 *----------------------------------------------------------------------
 */
ChainLink *
Chain::GetNthLink(long position) /* Index of link to select from front or back
				  * of the chain. */
{
    ChainLink *linkPtr;

    for (linkPtr = _head; linkPtr != NULL; linkPtr = linkPtr->Next()) {
	if (position == 0) {
	    return linkPtr;
	}
	position--;
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Sort --
 *
 *	Sorts the chain according to the given comparison routine.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The chain is reordered.
 *
 *----------------------------------------------------------------------
 */

void
Chain::Sort(ChainCompareProc *proc)
{
    ChainLink *linkPtr;
    long i;

    if (_nLinks < 2) {
	return;
    }
    ChainLink **linkArr = new ChainLink *[_nLinks + 1];
    if (linkArr == NULL) {
	return;			/* Out of memory. */
    }
    i = 0;
    for (linkPtr = _head; linkPtr != NULL; linkPtr = linkPtr->Next()) { 
	linkArr[i++] = linkPtr;
    }
    qsort((char *)linkArr, _nLinks, sizeof(ChainLink *), 
	  (QSortCompareProc *)proc);

    /* Rethread the chain. */
    linkPtr = linkArr[0];
    _head = linkPtr;
    linkPtr->_prev = NULL;
    for (i = 1; i < _nLinks; i++) {
	linkPtr->_next = linkArr[i];
	linkPtr->Next()->_prev = linkPtr;
	linkPtr = linkPtr->Next();
    }
    _tail = linkPtr;
    linkPtr->_next = NULL;
    delete [] linkArr;
}

