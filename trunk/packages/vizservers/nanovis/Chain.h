/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright 1993-2004 George A Howlett.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef NV_CHAIN_H
#define NV_CHAIN_H

namespace nv {

/**
 * \brief A ChainLink is the container structure for the Chain.
 */
class ChainLink
{
public:
    ChainLink()
    {
        _clientData = NULL;
        _next = _prev = NULL;
    }

    ChainLink(void *clientData)
    {
        _clientData = clientData;
         _next = _prev = NULL;
    }

    void *getValue()
    {
        return _clientData;
    }

    void setValue(void *clientData)
    {
        _clientData = clientData;
    }

    ChainLink *next()
    {
        return _next;
    }

    ChainLink *prev()
    {
        return _prev;
    }

    friend class Chain;

private:
    ChainLink *_prev;  ///< Link to the previous link
    ChainLink *_next;  ///< Link to the next link
    void *_clientData; ///< Pointer to the data object
};

typedef int (ChainCompareProc) (ChainLink *l1Ptr, ChainLink *l2Ptr);

/**
 * \brief A Chain is a doubly linked list structure.
 */
class Chain
{
public:
    Chain()
    {
        init();
    }
    ~Chain()
    {
        reset();
    }

    void init()
    {
        _nLinks = 0;
        _head = _tail = NULL;
    }

    ChainLink *append(void *clientData);

    ChainLink *prepend(void *clientData);

    ChainLink *getNthLink(long position);

    void linkBefore(ChainLink *linkPtr, ChainLink *beforePtr);

    void linkAfter(ChainLink *linkPtr, ChainLink *afterPtr);

    void deleteLink(ChainLink *linkPtr);

    void unlink(ChainLink *linkPtr);

    void reset();

    void sort(ChainCompareProc *proc);

    long getLength()
    {
        return _nLinks;
    }

    ChainLink *firstLink()
    {
        return _head;
    }

    ChainLink *lastLink()
    {
        return _tail;
    }

    void appendLink(ChainLink *linkPtr)
    {
        linkBefore(linkPtr, (ChainLink *)NULL);
    }

    void prependLink(ChainLink *linkPtr)
    {
        linkAfter(linkPtr, (ChainLink *)NULL);
    }

private:
    ChainLink *_head; ///< Pointer to first element in chain
    ChainLink *_tail; ///< Pointer to last element in chain
    long _nLinks;     ///< Number of elements in chain
};

}

#endif
