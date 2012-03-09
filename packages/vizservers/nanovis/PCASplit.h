/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef PCA_SPLIT_H
#define PCA_SPLIT_H

#include <memory.h>

#include "Vector3.h"
#include "Vector4.h"

namespace PCA {

class Point
{
public:
    Point() :
        size(1.0f),
        value(0.0f)
    {}

    Vector3 position;
    // histogram Index;
    //unsigned char color;
    Vector4 color;
    float size;
    float value;
};

class Cluster
{
public:
    Cluster() :
        color(1.0f, 1.0f, 1.0f, 1.0f), 
        scale(0.0f), 
        numOfChildren(0), 
        numOfPoints(0),
        points(0), 
        children(0), 
        vbo(0)/*, minValue(0.0f), maxValue(0.0f)*/
        , level(0)
    {
    }

    void setChildren(Cluster *children, int count)
    {
        this->children = children;
        numOfChildren = count;
    }
    void setPoints(Point *points, int count)
    {
        this->points = points;
        numOfPoints = count;
    }

    Vector3 centroid;
    Vector4 color;
    float scale;

    int numOfChildren;
    int numOfPoints;
    Point *points;
    Cluster *children;
    unsigned int vbo;
    int level;
};

class ClusterAccel
{
public:
    ClusterAccel(int maxlevel)
        : root(0),
          maxLevel(maxlevel)
    {
        startPointerCluster = new Cluster *[maxLevel];
        numOfClusters = new int[maxLevel];
        _vbo = new unsigned int[maxLevel];
        memset(_vbo, 0, sizeof(unsigned int) * maxLevel);
    }

    Cluster *root;
    int maxLevel;
    Cluster **startPointerCluster;
    int *numOfClusters;
    unsigned int *_vbo;
};

class Cluster_t
{
public:
    Cluster_t() :
        numOfPoints_t(0),
        scale_t(0.0f),
        points_t(0)
    {}

    Vector3 centroid_t;
    int numOfPoints_t;
    float scale_t;
    Point *points_t;
};

class ClusterListNode
{
public:
    ClusterListNode(Cluster_t *d, ClusterListNode *n) :
        data(d),
        next(n)
    {}

    ClusterListNode() :
        data(NULL),
        next(NULL)
    {}

    Cluster_t *data;
    ClusterListNode *next;
};

class PCASplit
{
public:
    enum {
        MAX_INDEX = 8000000
    };

    PCASplit();

    ~PCASplit();

    ClusterAccel *doIt(Point *data, int count);

    void setMinDistance(float minDistance)
    {
        _minDistance = minDistance;
    }

    void setMaxLevel(int maxLevel)
    {
        _maxLevel = maxLevel;
    }

    static void computeDistortion(Point *data, int count, const Vector3& mean, float& distortion, float& maxSize);

    static void computeCentroid(Point *data, int count, Vector3& mean);

    static void computeCovariant(Point *data, int count, const Vector3& mean, float *m);

private:
    void init();

    void analyze(ClusterListNode *cluster, Cluster *parent, int level, float limit);

    void addLeafCluster(Cluster_t *cluster);

    Cluster *createClusterBlock(ClusterListNode *clusterList, int count, int level);

    void split(Point *data, int count, float maxDistortion);

    float getMaxDistortionSquare(ClusterListNode *clusterList, int count);

    ClusterAccel *_clusterHeader;

    ClusterListNode *_curClusterNode;
    ClusterListNode *_memClusterChunk1;
    ClusterListNode *_memClusterChunk2;
    ClusterListNode *_curMemClusterChunk;

    int _memClusterChunkIndex;

    Cluster *_curRoot;

    int _curClusterCount;
    int _maxLevel;
    float _minDistance;
    float _distanceScale;
    unsigned int *_indices;
    int _indexCount;
    int _finalMaxLevel;
};	

}

#endif 

