/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef POINT_SET_H
#define POINT_SET_H

#include <vrmath/Vector3f.h>
#include <vrmath/Vector4f.h>

#include "PCASplit.h"

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

    void initialize(vrmath::Vector4f *values, const unsigned int count, 
		    const vrmath::Vector3f& scale, const vrmath::Vector3f& origin, 
		    float min, float max);

    void updateColor(float *color, int count);

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

    vrmath::Vector3f& getScale()
    {
        return _scale;
    }

    const vrmath::Vector3f& getScale() const
    {
        return _scale;
    }

    vrmath::Vector3f& getOrigin()
    {
        return _origin;
    }

    const vrmath::Vector3f& getOrigin() const
    {
        return _origin;
    }

private:
    unsigned int _sortLevel;
    PCA::ClusterAccel *_cluster;

    vrmath::Vector3f _scale;
    vrmath::Vector3f _origin;
    float _max;
    float _min;
    bool _visible;
};

#endif
