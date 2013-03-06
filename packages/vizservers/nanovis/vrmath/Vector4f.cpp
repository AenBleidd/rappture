/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <vrmath/Vector4f.h>
#include <vrmath/Matrix4x4f.h>

void vrVector4f::mult( const vrMatrix4x4f& mat, const vrVector4f& vector)
{
    const float *m = mat.get();
    x = vector.x*m[0]+vector.y*m[4]+vector.z*m[8]+vector.w*m[12];
    y = vector.x*m[1]+vector.y*m[5]+vector.z*m[9]+vector.w*m[13];
    z = vector.x*m[2]+vector.y*m[6]+vector.z*m[10]+vector.w*m[14];
    w = vector.x*m[3]+vector.y*m[7]+vector.z*m[11]+vector.w*m[15];   
}

void vrVector4f::mult(const vrMatrix4x4f& mat)
{
    vrVector4f vector(x, y, z, w);
    const float *m = mat.get();

    x = vector.x*m[0]+vector.y*m[4]+vector.z*m[8]+vector.w*m[12];
    y = vector.x*m[1]+vector.y*m[5]+vector.z*m[9]+vector.w*m[13];
    z = vector.x*m[2]+vector.y*m[6]+vector.z*m[10]+vector.w*m[14];
    w = vector.x*m[3]+vector.y*m[7]+vector.z*m[11]+vector.w*m[15];   
}

void vrVector4f::transform(const vrVector4f& v, const vrMatrix4x4f& mat)
{
    vrVector4f vector(x, y, z, w);
    const float *m = mat.get();
    x = vector.x * m[0] + vector.y*m[4]+vector.z*m[8]+vector.w*m[12];
    y = vector.x * m[1] + vector.y*m[5]+vector.z*m[9]+vector.w*m[13];
    z = vector.x*m[2]+vector.y*m[6]+vector.z*m[10]+vector.w*m[14];
    w = vector.x*m[3]+vector.y*m[7]+vector.z*m[11]+vector.w*m[15];

}
