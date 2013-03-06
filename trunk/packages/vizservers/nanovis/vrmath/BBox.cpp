/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <vrmath/BBox.h>
#include <vrmath/Vector3f.h>

vrBBox::vrBBox()
{
    makeEmpty();
}

vrBBox::vrBBox(const vrBBox& bbox)
{
    min = bbox.min;
    max = bbox.max;	
}

vrBBox::vrBBox(const vrVector3f& minv, const vrVector3f& maxv)
{
    min = minv;
    max = maxv;	
}

void vrBBox::makeEmpty()
{
    float big = 1e27f;
    min.set(big, big, big);
    max.set(-big, -big, -big);
}

bool vrBBox::isEmpty()
{
    if ((min.x > max.x) || (min.y > max.y) || (min.z > max.z)) {
        return true;
    }

    return false;
}

void vrBBox::make(const vrVector3f& center, const vrVector3f& size)
{
    float halfX = size.x * 0.5f, halfY = size.y * 0.5f, halfZ = size.z * 0.5f;

    min.x = center.x - halfX;
    max.x = center.x + halfX;

    min.y = center.y - halfY;
    max.y = center.y + halfY;

    min.z = center.z - halfZ;
    max.z = center.z + halfZ;
}

void vrBBox::extend(const vrVector3f& point)
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

void vrBBox::extend(const vrBBox& box)
{
    if (min.x > box.min.x) min.x = box.min.x;
    if (min.y > box.min.y) min.y = box.min.y;
    if (min.z > box.min.z) min.z = box.min.z;

    if (max.x < box.max.x) max.x = box.max.x;
    if (max.y < box.max.y) max.y = box.max.y;
    if (max.z < box.max.z) max.z = box.max.z;
}

void vrBBox::transform(const vrBBox& box, const vrMatrix4x4f& mat)
{
    float halfSizeX = (box.max.x - box.min.x) * 0.5f;
    float halfSizeY = (box.max.y - box.min.y) * 0.5f;
    float halfSizeZ = (box.max.z - box.min.z) * 0.5f;

    float centerX = (box.max.x + box.min.x) * 0.5f;
    float centerY = (box.max.y + box.min.y) * 0.5f;
    float centerZ = (box.max.z + box.min.z) * 0.5f;

    vrVector3f points[8];

    points[0].set(centerX + halfSizeX, centerY + halfSizeY, centerZ + halfSizeZ);
    points[1].set(centerX + halfSizeX, centerY + halfSizeY, centerZ - halfSizeZ);
    points[2].set(centerX - halfSizeX, centerY + halfSizeY, centerZ - halfSizeZ);
    points[3].set(centerX - halfSizeX, centerY + halfSizeY, centerZ + halfSizeZ);
    points[4].set(centerX - halfSizeX, centerY - halfSizeY, centerZ + halfSizeZ);
    points[5].set(centerX - halfSizeX, centerY - halfSizeY, centerZ - halfSizeZ);
    points[6].set(centerX + halfSizeX, centerY - halfSizeY, centerZ - halfSizeZ);
    points[7].set(centerX + halfSizeX, centerY - halfSizeY, centerZ + halfSizeZ);

    float minX, minY, minZ;
    float maxX, maxY, maxZ;

    points[0].transform(mat, points[0]);

    minX = maxX = points[0].x;
    minY = maxY = points[0].y;
    minZ = maxZ = points[0].z;

    for (int i = 1; i < 8; i++) {
        points[i].transform(mat, points[i]);

        if (points[i].x > maxX) maxX = points[i].x;
        else if (points[i].x < minX) minX = points[i].x;

        if (points[i].y > maxY) maxY = points[i].y;
        else if (points[i].y < minY) minY = points[i].y;

        if (points[i].z > maxZ) maxZ = points[i].z;
        else if (points[i].z < minZ) minZ = points[i].z;
    }

    min.set(minX, minY, minZ);
    max.set(maxX, maxY, maxZ);
}

bool vrBBox::intersect(const vrBBox& box)
{
    if (min.x > box.max.x || max.x < box.min.x) return false;
    if (min.y > box.max.y || max.y < box.min.y) return false;
    if (min.z > box.max.z || max.z < box.min.z) return false;

    return true;
}

bool vrBBox::intersect(const vrVector3f& point)
{
    if ((point.x < min.x) || (point.x > max.x)) return false;
    if ((point.y < min.y) || (point.y > max.y)) return false;
    if ((point.z < min.z) || (point.z > max.z)) return false;

    return true;
}
