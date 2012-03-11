/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <vrmath/vrVector3f.h>
#include <vrmath/vrMatrix4x4f.h>

void vrVector3f::transform(const vrMatrix4x4f& m, const vrVector3f& v)
{
    const float *mat = m.get();

    set(mat[0] * v.x + mat[4] * v.y + mat[8] * v.z + mat[12],
        mat[1] * v.x + mat[5] * v.y + mat[9] * v.z + mat[13],
        mat[2] * v.x + mat[6] * v.y + mat[10] * v.z + mat[14]);
}

void vrVector3f::transformVec(const vrMatrix4x4f& m, const vrVector3f& v)
{
    const float *mat = m.get();

    set(mat[0] * v.x + mat[4] * v.y + mat[8] * v.z,
        mat[1] * v.x + mat[5] * v.y + mat[9] * v.z,
        mat[2] * v.x + mat[6] * v.y + mat[10] * v.z);
}
