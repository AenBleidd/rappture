/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */

#include <vrmath/Vector3f.h>
#include <vrmath/Matrix4x4d.h>

#include "BucketSort.h"

using namespace PCA;
using namespace vrmath;

void 
BucketSort::init()
{
    _count = 0;
    memset(_buffer, 0, sizeof(ClusterList*) * _size);
}

void 
BucketSort::sort(ClusterAccel* clusterAccel, const Matrix4x4d& cameraMat, int level)
{
    if (clusterAccel == 0) {
        return;
    }
    Cluster* cluster = clusterAccel->startPointerCluster[level - 1];
    Cluster* c = &(cluster[0]);
    Cluster* end = &(cluster[clusterAccel->numOfClusters[level - 1] - 1]);

    for (; c <= end; c = (Cluster*) ((char *)c + sizeof(Cluster))) {
        Vector4f pt = cameraMat.transform(Vector4f(c->centroid, 1));
        Vector3f pos(pt.x, pt.y, pt.z);
        addBucket(c, pos.length()*_invMaxLength);
    }
}       

void 
BucketSort::addBucket(Cluster* cluster, float ratio)
{
    int index = (int) (ratio * _size);
    ClusterList* c = &(_memChunck[_count++]);
    c->data = cluster;
    c->next =_buffer[index];
    _buffer[index] = c;
}

