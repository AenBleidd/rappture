/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __R2_OBJECT_H__
#define __R2_OBJECT_H__

#include <R2/R2.h>

class R2Object {
    /**
     * @brief reference count
     */
    R2int32 _refCount;

public :
    R2Object();
protected :
    virtual ~R2Object();

public :
    R2int32 getRef();
    R2int32 ref();
    void unref();
};

inline R2int32 R2Object::getRef()
{
    return _refCount;
}

inline R2int32 R2Object::ref()
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

