/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef R2_OBJECT_H
#define R2_OBJECT_H

#include <R2/R2.h>

class R2Object
{
public:
    R2Object();

    int getRef();
    int ref();
    void unref();

protected:
    virtual ~R2Object();

private:
    /// reference count
    int _refCount;
};

inline int R2Object::getRef()
{
    return _refCount;
}

inline int R2Object::ref()
{
    return ++_refCount;
}

inline void R2Object::unref()
{
    _refCount--;
    if (_refCount <= 0) {
	delete this;
    }
}

#endif

