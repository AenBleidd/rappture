/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "PointSet.h"
#include "PCASplit.h"
#include <stdlib.h>
#include <stdio.h>

void 
PointSet::initialize(Vector4* values, const unsigned int count, 
		     const Vector3& scale, const Vector3& origin, 
		     float min, float max)
{
    PCA::PCASplit pcaSplit;

    _scale = scale;
    _origin = origin;
    _min = min;
    _max = max;

    PCA::Point* points = (PCA::Point*) malloc(sizeof(PCA::Point) * count);
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
PointSet::updateColor(float* color,  int colorCount)
{
    if (_cluster == 0) {
	return;
    }
    int level = 4;

    PCA::Cluster* cluster = _cluster->startPointerCluster[level - 1];
    PCA::Cluster* c = &(cluster[0]);
    PCA::Cluster* end = &(cluster[_cluster->numOfClusters[level - 1] - 1]);
    
    Vector3 pos;
    
    PCA::Point* points;
    Vector4* colors = (Vector4*) color;
    int numOfPoints;
    int index;
    float d = _max - _min;
    for (/*empty*/; c <= end; c = (PCA::Cluster*)((char *)c+sizeof(PCA::Cluster))){
        points = c->points;
        numOfPoints = c->numOfPoints;
	
        for (int i = 0; i < numOfPoints; ++i) {
            index = (points[i].value - _min) / d  * (colorCount - 1);
            if (index >= colorCount) {
		index = colorCount - 1;
	    }
            points[i].color = colors[index];
        }
    }
}

