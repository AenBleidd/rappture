/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 */
#ifndef VRBBOX_H
#define VRBBOX_H

#include <vrmath/Vector3f.h>

namespace vrmath {

class Matrix4x4d;

class BBox
{
public:
    /**
     * @brief constructor
     */
    BBox();

    /**
     * @brief constructor
     * @param bbox bounding box
     */
    BBox(const BBox& bbox);

    /**
     * @brief constructor
     * @param min minimum point of the bounding box
     * @param max maximum point of the bounding box
     */
    BBox(const Vector3f& min, const Vector3f& max);

    BBox& operator=(const BBox& other)
    {
        if (&other != this) {
            min = other.min;
            max = other.max;
        }
        return *this;
    }

    /**
     * @brief make an empty bounding box
     */
    void makeEmpty();

    /**
     * @brief make an bouning box
     * @param center the center of bounding box
     * @param size the size of bounding box
     */
    void make(const Vector3f& center, const Vector3f& size);

    /**
     * @brief check if the bounding box is empty
     */
    bool isEmpty();
    bool isEmptyX();
    bool isEmptyY();
    bool isEmptyZ();

    /**
     * @brief extend the bounding box by a point
     */
    void extend(const Vector3f& point);

    /**
     * @brief extend the bounding box by a bbox
     */
    void extend(const BBox& bbox);

    /**
     * @brief transform a bounding box with a matrix and set the bounding box
     */
    void transform(const BBox& box, const Matrix4x4d& mat);

    /**
     * @brief check if the bounding box intersect with a box
     */
    bool intersect(const BBox& box);

    /**
     * @brief check if the bounding box contains a point
     */
    bool contains(const Vector3f& point);

    float getRadius() const;
    Vector3f getCenter() const;
    Vector3f getSize() const;

    Vector3f min;
    Vector3f max;
};

inline float BBox::getRadius() const
{
    return max.distance( min ) * 0.5f;
}

inline Vector3f BBox::getCenter() const
{
    Vector3f temp;
    temp.x = (max.x + min.x) * 0.5f;
    temp.y = (max.y + min.y) * 0.5f;
    temp.z = (max.z + min.z) * 0.5f;
    return temp;
}

inline Vector3f  BBox::getSize() const
{
    Vector3f temp;
    temp.x = max.x - min.x;
    temp.y = max.y - min.y;
    temp.z = max.z - min.z;
    return temp;
}

}

#endif
