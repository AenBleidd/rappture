/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Chain.h --
 *
 *	Copyright 1993-2004 George A Howlett.
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
#ifndef _CHAIN_H
#define _CHAIN_H

/*
 * A ChainLink is the container structure for the Chain.
 */
class ChainLink {
    friend class Chain;
private:
    ChainLink *_prev;		/* Link to the previous link */
    ChainLink *_next;		/* Link to the next link */
    void *_clientData;		/* Pointer to the data object */
public:
    ChainLink(void) {
	_clientData = NULL;
	_next = _prev = NULL;
    }
    ChainLink(void *clientData) {
	_clientData = clientData;
 	_next = _prev = NULL;
    }
    void *GetValue(void) {
	return _clientData;
    }
    void SetValue(void *clientData) {
	_clientData = clientData;
    }
    ChainLink *Next(void) {
	return _next;
    }
    ChainLink *Prev(void) {
	return _prev;
    }
};

typedef int (ChainCompareProc) (ChainLink *l1Ptr, ChainLink *l2Ptr);

/*
 * A Chain is a doubly linked list structure.
 */
class Chain {
private:
    ChainLink *_head;		/* Pointer to first element in chain */
    ChainLink *_tail;		/* Pointer to last element in chain */
    long _nLinks;		/* Number of elements in chain */
public:
    Chain(void) {
	Init();
    }
    ~Chain(void) {
	Reset();
    }
    void Init(void) {
	_nLinks = 0;
	_head = _tail = NULL;
    }
    ChainLink *Append(void *clientData);
    ChainLink *Prepend(void *clientData);
    ChainLink *GetNthLink(long position);
    void LinkBefore(ChainLink *linkPtr, ChainLink *beforePtr);
    void LinkAfter(ChainLink *linkPtr, ChainLink *afterPtr);
    void DeleteLink(ChainLink *linkPtr);
    void Unlink(ChainLink *linkPtr);
    void Reset(void);
    void Sort(ChainCompareProc *proc);
    long GetLength(void)   {
	return _nLinks;
    }
    ChainLink *FirstLink(void) {
	return _head;
    }
    ChainLink *LastLink(void) {
	return _tail;
    }
    void AppendLink(ChainLink *linkPtr) {
	LinkBefore(linkPtr, (ChainLink *)NULL);
    }
    void PrependLink(ChainLink *linkPtr) {
	LinkAfter(linkPtr, (ChainLink *)NULL);
    }
};

#endif /* _CHAIN_H */
