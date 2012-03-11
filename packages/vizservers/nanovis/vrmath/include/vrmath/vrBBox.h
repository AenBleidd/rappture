/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef VRBBOX_H
#define VRBBOX_H

#include <vrmath/vrLinmath.h>
#include <vrmath/vrVector3f.h>
#include <vrmath/vrMatrix4x4f.h>

class LmExport vrBBox
{
public:
    /**
     * @brief constructor
     */
    vrBBox();

    /**
     * @brief constructor
     * @param bbox bounding box
     */
    vrBBox(const vrBBox& bbox);

    /**
     * @brief constructor
     * @param min minimum point of the bounding box
     * @param max maximum point of the bounding box
     */
    vrBBox(const vrVector3f& min, const vrVector3f& max);

    /**
     * @brief make an empty bounding box
     */
    void makeEmpty();

    /**
     * @brief make an bouning box
     * @param center the center of bounding box
     * @param size the size of bounding box
     */
    void make(const vrVector3f& center, const vrVector3f& size);

    /**
     * @brief check if the bounding box is empty
     */
    bool isEmpty();

    /**
     * @brief extend the bounding box by a point
     */
    void extend(const vrVector3f& point);

    /**
     * @brief extend the bounding box by a bbox
     */
    void extend(const vrBBox& bbox);

    /**
     * @brief transform a bounding box with an matrix and set the bounding box
     */
    void transform(const vrBBox& box, const vrMatrix4x4f& mat);

    /**
     * @brief check if the bounding box intersect with a box
     */
    bool intersect(const vrBBox& box);

    /**
     * @brief check if the bounding box intersect with a point
     */
    bool intersect(const vrVector3f& point);

    float getRadius() const;
    vrVector3f getCenter() const;
    vrVector3f getSize() const;

    vrVector3f min;
    vrVector3f max;
};

inline float vrBBox::getRadius() const
{
    return max.distance( min ) * 0.5f;
}

inline vrVector3f vrBBox::getCenter() const
{
    vrVector3f temp;
    temp.x = (max.x+ min.x) * 0.5f;
    temp.y = (max.y + min.y) * 0.5f;
    temp.z = (max.z + min.z) * 0.5f;
    return temp;
}

inline vrVector3f  vrBBox::getSize() const
{
    vrVector3f temp;
    temp.x = max.x - min.x;
    temp.y = max.y - min.y;
    temp.z = max.z - min.z;
    return temp;
}

#endif
