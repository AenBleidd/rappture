/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include <stdlib.h>
#include <stdio.h>

#include <vrmath/Vector3f.h>
#include <vrmath/Vector4f.h>

#include "PointSet.h"
#include "PCASplit.h"

using namespace nv;

void 
PointSet::initialize(vrmath::Vector4f *values, const unsigned int count, 
		     const vrmath::Vector3f& scale, const vrmath::Vector3f& origin, 
		     float min, float max)
{
    PCA::PCASplit pcaSplit;

    _scale = scale;
    _origin = origin;
    _min = min;
    _max = max;

    PCA::Point *points = (PCA::Point *)malloc(sizeof(PCA::Point) * count);
    for (unsigned int i = 0; i < count; ++i) {
        points[i].position.set(values[i].x, values[i].y, values[i].z);
        points[i].color.set(1.0f, 1.0f, 1.0f, 0.2f);
        points[i].value = values[i].w;
        points[i].size = 0.001;
    }

    if (_cluster != 0) {
        delete _cluster;
    }
    
    pcaSplit.setMinDistance(scale.length() / 3.0f);
    _cluster = pcaSplit.doIt(points, count);
}

void 
PointSet::updateColor(float *color, int colorCount)
{
    if (_cluster == 0) {
	return;
    }
    int level = 4;

    PCA::Cluster *cluster = _cluster->startPointerCluster[level - 1];
    PCA::Cluster *c = &(cluster[0]);
    PCA::Cluster *end = &(cluster[_cluster->numOfClusters[level - 1] - 1]);

    vrmath::Vector3f pos;

    PCA::Point *points;
    vrmath::Vector4f *colors = (vrmath::Vector4f *)color;
    int numOfPoints;
    int index;
    float d = _max - _min;
    for (; c <= end; c = (PCA::Cluster *)((char *)c + sizeof(PCA::Cluster))) {
        points = c->points;
        numOfPoints = c->numOfPoints;

        for (int i = 0; i < numOfPoints; ++i) {
            index = (int)((points[i].value - _min) / d  * (colorCount - 1));
            if (index >= colorCount) {
		index = colorCount - 1;
	    }
            points[i].color = colors[index];
        }
    }
}
