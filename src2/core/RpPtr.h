/*
 * ----------------------------------------------------------------------
 *  Rappture::Ptr<type>
 *    This is a smart pointer with reference counting.  Once one of
 *    these pointers is constructed with an object of the underlying
 *    type, that object becomes property of the pointer.  Other
 *    pointers can point to the same object.  When all such pointers
 *    have been destroyed, the underlying object goes away.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef RAPPTURE_POINTER_H
#define RAPPTURE_POINTER_H

#include <assert.h>

#ifndef NULL
#  define NULL 0
#endif

namespace Rappture {

template <class Type>
class PtrCore {
public:
    explicit PtrCore(Type* ptr=NULL);
    ~PtrCore();

    // copying the core is not allowed
    PtrCore(const PtrCore& pc) { assert(0); }
    PtrCore& operator=(const PtrCore& pc) { assert(0); }

    Type* pointer() const;
    void attach();
    void detach();

private:
    Type *_ptr;
    int _refcount;
};

template <class Type>
PtrCore<Type>::PtrCore(Type* ptr)
  : _ptr(ptr),
    _refcount(1)
{
}

template <class Type>
PtrCore<Type>::~PtrCore()
{
    assert(_refcount <= 0);
}

template <class Type>
Type*
PtrCore<Type>::pointer() const
{
    return _ptr;
}

template <class Type>
void
PtrCore<Type>::attach()
{
    _refcount++;
}

template <class Type>
void
PtrCore<Type>::detach()
{
    if (--_refcount <= 0 && _ptr != NULL) {
        delete _ptr;
        delete this;
    }
}


template <class Type>
class Ptr {
public:
    explicit Ptr(Type* ptr=NULL);
    Ptr(const Ptr& ptr);
    Ptr& operator=(Type* ptr);
    Ptr& operator=(const Ptr& ptr);
    ~Ptr();

    int isNull() const;
    Type* operator->() const;
    Type* pointer() const;
    Type& operator*() const;
    void clear();

private:
    PtrCore<Type> *_pc;
};

template <class Type>
Ptr<Type>::Ptr(Type* ptr)
  : _pc(NULL)
{
    if (ptr) {
        _pc = new PtrCore<Type>(ptr);
    }
}

template <class Type>
Ptr<Type>::Ptr(const Ptr<Type>& ptr)
  : _pc(NULL)
{
    if (ptr._pc) {
        ptr._pc->attach();
        _pc = ptr._pc;
    }
}

template <class Type>
Ptr<Type>::~Ptr()
{
    if (_pc) {
        _pc->detach();
    }
}

template <class Type>
Ptr<Type>&
Ptr<Type>::operator=(Type* ptr)
{
    if (_pc) {
        _pc->detach();
    }
    _pc = new PtrCore<Type>(ptr);
}

template <class Type>
Ptr<Type>&
Ptr<Type>::operator=(const Ptr<Type>& ptr)
{
    if (ptr._pc) {
        ptr._pc->attach();
    }
    if (_pc) {
        _pc->detach();
    }
    _pc = ptr._pc;
}

template <class Type>
int
Ptr<Type>::isNull() const
{
    if (_pc) {
        return (_pc->pointer() == NULL);
    }
    return 1;
}

template <class Type>
Type*
Ptr<Type>::operator->() const
{
    if (_pc) {
        return _pc->pointer();
    }
    return NULL;
}

template <class Type>
Type*
Ptr<Type>::pointer() const
{
    if (_pc) {
        return _pc->pointer();
    }
    return NULL;
}

template <class Type>
Type&
Ptr<Type>::operator*() const
{
    assert(_pc != NULL);
    return *(Type*)_pc->pointer();
}

template <class Type>
void
Ptr<Type>::clear()
{
    if (_pc) {
        _pc->detach();
        _pc = NULL;
    }
}

} // namespace Rappture

#endif
