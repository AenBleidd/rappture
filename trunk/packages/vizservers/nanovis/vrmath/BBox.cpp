/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 */

#include <float.h>

#include <vrmath/BBox.h>
#include <vrmath/Matrix4x4d.h>
#include <vrmath/Vector3f.h>
#include <vrmath/Vector4f.h>

using namespace vrmath;

BBox::BBox()
{
    makeEmpty();
}

BBox::BBox(const BBox& other) :
    min(other.min),
    max(other.max)
{
}

BBox::BBox(const Vector3f& minv, const Vector3f& maxv) :
    min(minv),
    max(maxv)
{
}

void BBox::makeEmpty()
{
    min.set(FLT_MAX, FLT_MAX, FLT_MAX);
    max.set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
}

bool BBox::isEmpty()
{
    return (isEmptyX() && isEmptyY() && isEmptyZ());
}

bool BBox::isEmptyX()
{
    return (min.x > max.x);
}

bool BBox::isEmptyY()
{
    return (min.y > max.y);
}

bool BBox::isEmptyZ()
{
    return (min.z > max.z);
}

void BBox::make(const Vector3f& center, const Vector3f& size)
{
    float halfX = size.x * 0.5f, halfY = size.y * 0.5f, halfZ = size.z * 0.5f;

    min.x = center.x - halfX;
    max.x = center.x + halfX;

    min.y = center.y - halfY;
    max.y = center.y + halfY;

    min.z = center.z - halfZ;
    max.z = center.z + halfZ;
}

void BBox::extend(const Vector3f& point)
{
    if (min.x > point.x) {
        min.x = point.x;
    } else if (max.x < point.x)	{
        max.x = point.x;
    }

    if (min.y > point.y) {
        min.y = point.y;
    } else if (max.y < point.y)	{
        max.y = point.y;
    }

    if (min.z > point.z) {
        min.z = point.z;
    } else if (max.z < point.z)	{
        max.z = point.z;
    }
}

void BBox::extend(const BBox& box)
{
    if (min.x > box.min.x) min.x = box.min.x;
    if (min.y > box.min.y) min.y = box.min.y;
    if (min.z > box.min.z) min.z = box.min.z;

    if (max.x < box.max.x) max.x = box.max.x;
    if (max.y < box.max.y) max.y = box.max.y;
    if (max.z < box.max.z) max.z = box.max.z;
}

void BBox::transform(const BBox& box, const Matrix4x4d& mat)
{
    float x0 = box.min.x;
    float y0 = box.min.y;
    float z0 = box.min.z;
    float x1 = box.max.x;
    float y1 = box.max.y;
    float z1 = box.max.z;

    Vector4f points[8];

    points[0].set(x0, y0, z0, 1);
    points[1].set(x1, y0, z0, 1);
    points[2].set(x0, y1, z0, 1);
    points[3].set(x0, y0, z1, 1);
    points[4].set(x1, y1, z0, 1);
    points[5].set(x1, y0, z1, 1);
    points[6].set(x0, y1, z1, 1);
    points[7].set(x1, y1, z1, 1);

    float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
    float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;

    for (int i = 0; i < 8; i++) {
        points[i] = mat.transform(points[i]);

        if (points[i].x > maxX) maxX = points[i].x;
        if (points[i].x < minX) minX = points[i].x;
        if (points[i].y > maxY) maxY = points[i].y;
        if (points[i].y < minY) minY = points[i].y;
        if (points[i].z > maxZ) maxZ = points[i].z;
        if (points[i].z < minZ) minZ = points[i].z;
    }

    min.set(minX, minY, minZ);
    max.set(maxX, maxY, maxZ);
}

bool BBox::intersect(const BBox& box)
{
    if (min.x > box.max.x || max.x < box.min.x) return false;
    if (min.y > box.max.y || max.y < box.min.y) return false;
    if (min.z > box.max.z || max.z < box.min.z) return false;

    return true;
}

bool BBox::contains(const Vector3f& point)
{
    if ((point.x < min.x) || (point.x > max.x)) return false;
    if ((point.y < min.y) || (point.y > max.y)) return false;
    if ((point.z < min.z) || (point.z > max.z)) return false;

    return true;
}
