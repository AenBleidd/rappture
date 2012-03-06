/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#pragma once

#include <vrmath/vrLinmath.h>
#include <vrmath/vrVector3f.h>
#include <vrmath/vrVector4f.h>

class vrMatrix4x4f;

class LmExport vrVector4f {
public :
	float	x, y, z, w;

public :
	vrVector4f() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
	vrVector4f(const vrVector4f& v4) : x(v4.x), y(v4.y), z(v4.z) , w(v4.w) {}
    vrVector4f(const vrVector3f& v3, float w1) : x(v3.x), y(v3.y), z(v3.z) , w(w1) {}
	vrVector4f(float x1, float y1, float z1, float w1) : x(x1), y(y1), z(z1), w(w1) {}

	void set(float x1, float y1, float z1, float w1);
	void set(const vrVector3f& v, float w);
	void set(const vrVector4f& v4);
    void divideByW();
    float dot(const vrVector4f& vec);
    void mult(const vrMatrix4x4f& mat, const vrVector4f& vec);
    void mult(const vrMatrix4x4f& mat);

	void transform(const vrVector4f& v, const vrMatrix4x4f& m);
};

inline void vrVector4f::divideByW()
{
    if (w != 0)
	{
        x /= w; y /= w; z /= w;
        w = 1.0f;
    }
}

inline void vrVector4f::set(float x1, float y1, float z1, float w1)
{
	x = x1;
	y = y1;
	z = z1;
    w = w1;
}

inline void vrVector4f::set(const vrVector4f& v4)
{
	x = v4.x;
	y = v4.y;
	z = v4.z;
    w = v4.w;
}

inline void vrVector4f::set(const vrVector3f& v, float w1)
{
	x = v.x;
	y = v.y;
	z = v.z;
    w = w1;
}

inline float vrVector4f::dot(const vrVector4f& vec)
{
    return x * vec.x + y * vec.y + z * vec.z + w * vec.w;
}
