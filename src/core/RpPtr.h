/*
 * ======================================================================
 *  Rappture::Ptr<type>
 *
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef RAPPTURE_PTR_H
#define RAPPTURE_PTR_H

#include <assert.h>
#include <stddef.h>

namespace Rappture {

/**
 *  This is the core of a smart pointer, built to keep a reference
 *  count and do most of the work, so the template class can be lean
 *  and mean.
 */
class PtrCore {
public:
    explicit PtrCore(void* ptr=NULL);
    ~PtrCore();

    void* pointer() const;
    void attach();
    void* detach();

private:
    // copying the core is not allowed
    PtrCore(const PtrCore& pc) { assert(0); }
    PtrCore& operator=(const PtrCore& pc) { assert(0); return *this; }

    void *_ptr;
    int _refcount;
};

/**
 *  This is a smart pointer with reference counting.  Once one of
 *  these pointers is constructed with an object of the underlying
 *  type, that object becomes property of the pointer.  Other
 *  pointers can point to the same object.  When all such pointers
 *  have been destroyed, the underlying object goes away.
 */
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
    PtrCore *_pc;
};

template <class Type>
Ptr<Type>::Ptr(Type* ptr)
  : _pc(NULL)
{
    if (ptr) {
        _pc = new PtrCore(ptr);
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
    clear();
}

template <class Type>
Ptr<Type>&
Ptr<Type>::operator=(Type* ptr)
{
    clear();
    _pc = new PtrCore(ptr);
    return *this;
}

template <class Type>
Ptr<Type>&
Ptr<Type>::operator=(const Ptr<Type>& ptr)
{
    if (ptr._pc) {
        ptr._pc->attach();
    }
    clear();
    _pc = ptr._pc;
    return *this;
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
        return static_cast<Type*>(_pc->pointer());
    }
    return NULL;
}

template <class Type>
Type*
Ptr<Type>::pointer() const
{
    if (_pc) {
        return (Type*)_pc->pointer();
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
        Type* ptr = (Type*)_pc->detach();
        if (ptr) {
            // If we get back a pointer, then it is fully detached.
            // Clean it up.
            delete ptr;
            delete _pc;
        }
        _pc = NULL;
    }
}

} // namespace Rappture

#endif // RAPPTURE_PTR_H
