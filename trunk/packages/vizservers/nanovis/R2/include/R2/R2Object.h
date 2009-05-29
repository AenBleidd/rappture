#ifndef __R2_OBJECT_H__
#define __R2_OBJECT_H__

#include <R2/R2.h>

class R2Object {
    /**
     * @brief reference count
     */
    R2int32 _referenceCount;

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
    return _referenceCount;
}

inline R2int32 R2Object::ref()
{
    return ++_referenceCount;
}

inline void R2Object::unref()
{
    if (_referenceCount > 1)
    {
        --_referenceCount;
    }

    if (_referenceCount <= 0) delete this;
}


#endif

