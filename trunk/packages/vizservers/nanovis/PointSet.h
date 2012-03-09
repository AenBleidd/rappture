/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef POINT_SET_H
#define POINT_SET_H

#include "PCASplit.h"
#include "Vector4.h"
#include "Vector3.h"

class PointSet
{
public :
    PointSet() :
        _sortLevel(4),
        _cluster(0),
        _max(1.0f),
        _min(0.0f), 
        _visible(false)
    {
    }

    ~PointSet() {
	if (_cluster) {
	    delete _cluster;
	}
    }

    void initialize(Vector4 *values, const unsigned int count, 
		    const Vector3& scale, const Vector3& origin, 
		    float min, float max);

    void updateColor(float *color, int  count);

    bool isVisible() const
    {
        return _visible;
    }

    void setVisible(bool visible)
    {
        _visible = visible;
    }

    unsigned int getSortLevel() const
    {
        return _sortLevel;
    }

    PCA::ClusterAccel* getCluster()
    {
        return _cluster;
    }

    Vector3& getScale()
    {
        return _scale;
    }

    const Vector3& getScale() const
    {
        return _scale;
    }

    Vector3& getOrigin()
    {
        return _origin;
    }

    const Vector3& getOrigin() const
    {
        return _origin;
    }

private:
    unsigned int _sortLevel;
    PCA::ClusterAccel* _cluster;

    Vector3 _scale;
    Vector3 _origin;
    float _max;
    float _min;
    bool _visible;
};

#endif
