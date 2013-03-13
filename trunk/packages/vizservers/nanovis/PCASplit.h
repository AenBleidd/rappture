/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef PCA_SPLIT_H
#define PCA_SPLIT_H

#include <memory.h>

#include <vrmath/Vector3f.h>
#include <vrmath/Vector4f.h>

namespace PCA {

class Point
{
public:
    Point() :
        size(1.0f),
        value(0.0f)
    {}

    vrmath::Vector3f position;
    vrmath::Vector4f color;
    float size;
    float value;
};

class Cluster
{
public:
    Cluster() :
        centroid(0.0f, 0.0f, 0.0f),
        color(1.0f, 1.0f, 1.0f, 1.0f),
        scale(0.0f),
        numOfChildren(0),
        numOfPoints(0),
        points(NULL),
        children(NULL),
        vbo(0),
        level(0)
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

    vrmath::Vector3f centroid;
    vrmath::Vector4f color;
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
    ClusterAccel(int maxlevel) :
        root(NULL),
        maxLevel(maxlevel)
    {
        startPointerCluster = new Cluster *[maxLevel];
        numOfClusters = new int[maxLevel];
        vbo = new unsigned int[maxLevel];
        memset(vbo, 0, sizeof(unsigned int) * maxLevel);
    }

    Cluster *root;
    int maxLevel;
    Cluster **startPointerCluster;
    int *numOfClusters;
    unsigned int *vbo;
};

class Cluster_t
{
public:
    Cluster_t() :
        numOfPoints_t(0),
        scale_t(0.0f),
        points_t(0)
    {}

    vrmath::Vector3f centroid_t;
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

    static void computeDistortion(Point *data, int count, const vrmath::Vector3f& mean, float& distortion, float& maxSize);

    static void computeCentroid(Point *data, int count, vrmath::Vector3f& mean);

    static void computeCovariant(Point *data, int count, const vrmath::Vector3f& mean, float *m);

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

