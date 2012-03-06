/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#pragma once
#include <vrmath/vrLinmath.h>

class LmExport vrColor4f {
public :
	float r, g, b, a;

public :
	vrColor4f();
	vrColor4f(float r1, float g1, float b1, float a1 = 0);
	vrColor4f(const vrColor4f& col)
	: r(col.r), g(col.g), b(col.b), a(col.a)
	{
	}

public :
	friend bool operator==(const vrColor4f col1, const vrColor4f& col2);
	friend bool operator!=(const vrColor4f col1, const vrColor4f& col2);
};

inline bool operator==(const vrColor4f col1, const vrColor4f& col2)
{
	return ((col1.r == col2.r) && (col1.g == col2.g) && (col1.b == col2.b));
}

inline bool operator!=(const vrColor4f col1, const vrColor4f& col2)
{
	return ((col1.r != col2.r) || (col1.g != col2.g) || (col1.b != col2.b));
}
