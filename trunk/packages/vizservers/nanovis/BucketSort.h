/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef BUCKETSORT_H
#define BUCKETSORT_H

#include <vector>
#include <list>

#include <vrmath/Matrix4x4d.h>

#include "PCASplit.h"

namespace PCA {

class ClusterList
{
public :
    ClusterList(Cluster *d, ClusterList *n) :
        data(d), next(n)
    {}

    ClusterList() :
        data(0), next(0)
    {}

    ~ClusterList()
    {
        //if (next) delete next;
    }

    Cluster *data;
    ClusterList *next;
};

class BucketSort
{
public:
    BucketSort(int maxSize)
    {
        _size = maxSize;
        _buffer = new ClusterList*[maxSize];
        memset(_buffer, 0, _size * sizeof(ClusterList*));

        _memChuckSize = 1000000;
        _memChunck = new ClusterList[_memChuckSize];

        _invMaxLength = 1.0f / 4.0f;
    }

    ~BucketSort()
    {
        delete [] _buffer;
        delete [] _memChunck;
    }

    int getSize() const
    { return _size; }

    int getSortedItemCount() const
    { return _count; }

    void init();

    void sort(ClusterAccel* cluster, const vrmath::Matrix4x4d& cameraMat, int level);

    void addBucket(Cluster* cluster, float ratio);

    ClusterList **getBucket()
    { return _buffer; }

private:
    void _sort(Cluster *cluster, const vrmath::Matrix4x4d& cameraMat, int level);

    ClusterList **_buffer;
    int _size;
    int _count;

    int _memChuckSize;
    bool _memChunckUsedFlag;
    ClusterList *_memChunck;

    float _invMaxLength;
};

}

#endif 
