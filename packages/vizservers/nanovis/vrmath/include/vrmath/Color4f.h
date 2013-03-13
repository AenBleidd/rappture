/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 */
#ifndef VRCOLOR4F_H
#define VRCOLOR4F_H

namespace vrmath {

class Color4f
{
public:
    float r, g, b, a;

    Color4f();

    Color4f(float r1, float g1, float b1, float a1 = 0);

    Color4f(const Color4f& col) :
        r(col.r), g(col.g), b(col.b), a(col.a)
    {}

    friend bool operator==(const Color4f col1, const Color4f& col2);
    friend bool operator!=(const Color4f col1, const Color4f& col2);
};

inline bool operator==(const Color4f col1, const Color4f& col2)
{
    return ((col1.r == col2.r) && (col1.g == col2.g) && (col1.b == col2.b));
}

inline bool operator!=(const Color4f col1, const Color4f& col2)
{
    return ((col1.r != col2.r) || (col1.g != col2.g) || (col1.b != col2.b));
}

}

#endif
