/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_UTIL_OBJECT_H
#define NV_UTIL_OBJECT_H

namespace nv {
namespace util {

class Object
{
public:
    Object();

    int getRef();
    int ref();
    void unref();

protected:
    virtual ~Object();

private:
    /// reference count
    int _refCount;
};

inline int Object::getRef()
{
    return _refCount;
}

inline int Object::ref()
{
    return ++_refCount;
}

inline void Object::unref()
{
    _refCount--;
    if (_refCount <= 0) {
	delete this;
    }
}

}
}

#endif

