/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <R2/R2Object.h>

using namespace nv::util;

Object::Object() :
    _refCount(0)
{
}

Object::~Object()
{
}
