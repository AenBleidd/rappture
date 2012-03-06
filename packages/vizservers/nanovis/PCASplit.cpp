/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdio.h>
#include "PCASplit.h"

#define WANT_STREAM                  // include.h will get stream fns
#define WANT_MATH                    // include.h will get math fns
                                     // newmatap.h will get include.h
#include <newmatap.h>                // need matrix applications
#include <newmatio.h>                // need matrix output routines
#include <newmatrc.h>

#ifdef use_namespace
using namespace NEWMAT;              // access NEWMAT namespace
#endif

namespace PCA {

PCASplit::PCASplit() : 
    _maxLevel(4), 
    _minDistance(0.5f), 
    _distanceScale(0.2f), 
    _indexCount(0), 
    _finalMaxLevel(0)
{
    _indices = new unsigned int[MAX_INDEX];
    _memClusterChunk1 = new ClusterListNode[MAX_INDEX];
    _memClusterChunk2 = new ClusterListNode[MAX_INDEX];
    _curMemClusterChunk = _memClusterChunk1;
    _memClusterChunkIndex = 0;
}

PCASplit::~PCASplit()
{
    delete [] _indices;
    delete [] _memClusterChunk1;
    delete [] _memClusterChunk2;
}

void 
PCASplit::computeCentroid(Point* data, int count, Vector3& mean)
{
    float sumx= 0, sumy = 0, sumz = 0;
    float size = 0;
    float sumsize = 0;
    for (int i = 0; i < count; ++i) {
	size = data[i].size;
	sumx += data[i].position.x * size;
	sumy += data[i].position.y * size;
	sumz += data[i].position.z * size;
	sumsize += size;
    }
    sumsize = 1.0f / sumsize;
    mean.set(sumx * sumsize, sumy * sumsize, sumz * sumsize);
}

void 
PCASplit::computeCovariant(Point* data, int count, const Vector3& mean, 
			   float* m)
{
    memset(m, 0, sizeof(float) * 9);

    for (int i = 0; i < count; ++i) {
	m[0] += (data[i].position.x - mean.x) * (data[i].position.x - mean.x);
	m[1] += (data[i].position.x - mean.x) * (data[i].position.y - mean.y);
	m[2] += (data[i].position.x - mean.x) * (data[i].position.z - mean.z);
	m[4] += (data[i].position.y - mean.y) * (data[i].position.y - mean.y);
	m[5] += (data[i].position.y - mean.y) * (data[i].position.z - mean.z);
	m[8] += (data[i].position.z - mean.z) * (data[i].position.z - mean.z);
    }
    
    float invCount = 1.0f / (count - 1);
    m[0] *= invCount;
    m[1] *= invCount;
    m[2] *= invCount;
    m[4] *= invCount;
    m[5] *= invCount;
    m[8] *= invCount;
    m[3] = m[1];
    m[6] = m[2];
    m[7] = m[5];
}

void 
PCASplit::computeDistortion(Point* data, int count, const Vector3& mean, 
			    float& distortion, float& finalSize)
{
    distortion = 0.0f;
    finalSize = 0.0f;
        
    float maxSize = 0.0f;
    float distance;
    for (int i = 0; i < count; ++i) {
	// sum 
	distance = mean.distanceSquare(data[i].position.x, data[i].position.y, data[i].position.z);
	distortion += distance;
	
	if (data[i].size > maxSize) {
	    maxSize = data[i].size;
	}
	
	/*
	  finalSize += data[i].size * sqrt(distance);
	*/
	if (distance > finalSize) {
	    finalSize = distance;
	}
    }
    // equation 2
    //finalSize = 0.5f * sqrt (finalSize) / (float) (count - 1) + maxSize;
    finalSize = sqrt (finalSize) + maxSize;
}
        
void 
PCASplit::init()
{
    _curClusterNode = 0;
    _curClusterCount = 0;
}

Cluster* 
PCASplit::createClusterBlock(ClusterListNode* clusterList, int count, int level)
{
    static int cc = 0;
    cc += count;
    Cluster* clusterBlock = new Cluster[count];

    _clusterHeader->numOfClusters[level - 1] = count;
    _clusterHeader->startPointerCluster[level - 1] = clusterBlock;

    TRACE("Cluster created %d [in level %d]:total %d\n", count, level, cc);
        
    int i = 0;
    ClusterListNode* clusterNode = clusterList;
    while (clusterNode) {
	clusterBlock[i].centroid = clusterList->data->centroid_t;
	clusterBlock[i].level = level;
	clusterBlock[i].scale = clusterList->data->scale_t;
	
	clusterNode = clusterNode->next;
	++i;
    }
    if (count != i) {
	TRACE("ERROR\n");
    }
    return clusterBlock;
}

ClusterAccel* PCASplit::doIt(Point* data, int count)
{
    init();

    _clusterHeader = new ClusterAccel(_maxLevel);
        
    Cluster* root = new Cluster;
    Cluster_t* cluster_t = new Cluster_t();
    cluster_t->points_t = data;
    cluster_t->numOfPoints_t = count;
    root->level = 1;
        
    _clusterHeader->root = root;
    _clusterHeader->numOfClusters[0] = 1;
    _clusterHeader->startPointerCluster[0] = root;
        
    Vector3 mean;
    float distortion, scale;
        
    computeCentroid(cluster_t->points_t, cluster_t->numOfPoints_t, mean);
    computeDistortion(cluster_t->points_t, cluster_t->numOfPoints_t, mean, 
		      distortion, scale);
        
    cluster_t->centroid_t = mean;
    cluster_t->scale_t = scale;
        
    float mindistance = _minDistance;
    int level = 2;

    ClusterListNode* clustNodeList = &(_memClusterChunk2[0]);
    clustNodeList->data = cluster_t;
    clustNodeList->next = 0;
        
    _curRoot = root;
    do {
        analyze(clustNodeList, _curRoot, level, mindistance);

        mindistance *= _distanceScale;
        ++level;

        // swap memory buffer & init
        _curMemClusterChunk = (_curMemClusterChunk == _memClusterChunk1)?
            _memClusterChunk2 : _memClusterChunk1;
        _memClusterChunkIndex = 0;

        clustNodeList = _curClusterNode;

    } while (level <= _maxLevel);

    return _clusterHeader;
}

void 
PCASplit::addLeafCluster(Cluster_t* cluster)
{
    ClusterListNode* clusterNode = 
	&_curMemClusterChunk[_memClusterChunkIndex++];
    clusterNode->data = cluster;
    clusterNode->next = _curClusterNode;
    _curClusterNode = clusterNode;
    ++_curClusterCount;
}

void 
PCASplit::split(Point* data, int count, float limit)
{
    Vector3 mean;
    float m[9];

    computeCentroid(data, count, mean);
    float scale, distortion;
    computeDistortion(data, count, mean, distortion, scale);
        
    //if (distortion < limit)
    if (scale < limit) {
	Cluster_t* cluster_t = new Cluster_t();
	cluster_t->centroid_t = mean;
	cluster_t->points_t = data;
	cluster_t->numOfPoints_t = count;
	cluster_t->scale_t = scale;
	addLeafCluster(cluster_t);
	return;
    }
    
    computeCovariant(data, count, mean, m);
    
    SymmetricMatrix A(3);
    for (int i = 1; i <= 3; ++i) {
        for (int j = 1; j <= i; ++j) {
	    A(i, j) = m[(i - 1) * 3 + j - 1];
	}
    }
        
    Matrix U; DiagonalMatrix D;
    eigenvalues(A,D,U);
    Vector3 emax(U(1, 3), U(2, 3), U(3, 3));
    
    int left = 0, right = count - 1;
    
    Point p;
    for (;left < right;) {
	while (left < count && emax.dot(data[left].position - mean) >= 0.0f) {
	    ++left;
	}
	while (right >= 0 && emax.dot(data[right].position - mean) < 0.0f) {
	    --right;
	}
	if (left > right) {
	    break;
	}
	
	p = data[left];
	data[left] = data[right];
	data[right] = p;
	++left, --right;
        
    }
    
    if (left == 0 || right == count - 1) {
	TRACE("error\n");
	exit(1);
    } else {
	split(data, left, limit);
	split(data + left, count - left, limit);
    }
}

void 
PCASplit::analyze(ClusterListNode* clusterNode, Cluster* parent, int level, 
		  float limit)
{
    if (level > _maxLevel) {
	return;
    } 
    if (level > _finalMaxLevel) {
	_finalMaxLevel = level;
    }
        
    init();

    ClusterListNode* clNode = clusterNode;
        
    // initialize the indexCount of indices
    _indexCount = 0;
    while (clNode) {
	if (clNode->data) {
	    split(clNode->data->points_t, clNode->data->numOfPoints_t, limit);
	    _indices[_indexCount++] = _curClusterCount;
	}
	clNode = clNode->next;
    }
        
    //Vector3 mean;
    //computeCentroid(cluster->points, cluster->numOfPoints, mean);

    // the process values of split are in _curClusterNode and _curClusterCount
    ClusterListNode* curClusterNode = _curClusterNode;
    unsigned int curClusterNodeCount = _curClusterCount;

    if (curClusterNodeCount) {
	
	// create and init centroid
	Cluster* retClusterBlock = 
	    createClusterBlock(curClusterNode, curClusterNodeCount, level);
        
	_curRoot = retClusterBlock;
	
	if (level == _maxLevel) {
	    ClusterListNode* node = curClusterNode;
	    if (_indexCount > 0) {
		// for parent
		Point* points = new Point[curClusterNodeCount];
		
		parent[0].setChildren(retClusterBlock, _indices[0]);
		parent[0].setPoints(points, _indices[0]);
		
		for (unsigned int i = 0, in = 0; i < curClusterNodeCount; ++i) {
		    if (i >= _indices[in]) {
			in++;
			parent[in].setChildren(retClusterBlock + _indices[in - 1], _indices[in] - _indices[in - 1]);
			parent[in].setPoints(points + _indices[in - 1], _indices[in] - _indices[in - 1]);
		    }
                                        
		    retClusterBlock[i].scale = node->data->scale_t;
		    retClusterBlock[i].centroid = node->data->centroid_t;
		    retClusterBlock[i].points = node->data->points_t;
		    retClusterBlock[i].numOfPoints = node->data->numOfPoints_t;
		    retClusterBlock[i].color.set(float(rand()) / RAND_MAX, 
						 float(rand()) / RAND_MAX, 
						 float(rand()) / RAND_MAX,
						 0.2);
		    
		    points[i].position = node->data->centroid_t;
		    points[i].color.set(1.0f, 1.0f, 1.0f, 0.2f);
		    points[i].size = node->data->scale_t;
		    
		    node = node->next;
		}
	    }
	} else {
	    Point* points = new Point[curClusterNodeCount];
	    ClusterListNode* node = curClusterNode;
            
	    if (_indexCount > 0) {
		parent[0].setPoints(points, _indices[0]);
		parent[0].setChildren(retClusterBlock, _indices[0]);
		for (int k = 1; k < _indexCount; ++k) {
		    parent[k].setPoints(points + _indices[k - 1], 
					_indices[k] - _indices[k - 1]);
		    parent[k].setChildren(retClusterBlock + _indices[k - 1], 
					  _indices[k] - _indices[k - 1]);
		}
                                
		// set points of sub-clusters
		for (unsigned int i = 0; i < curClusterNodeCount; ++i) {
		    points[i].position = node->data->centroid_t;
		    points[i].color.set(1.0f, 1.0f, 1.0f, 0.2f);
		    points[i].size = node->data->scale_t;
		    node = node->next;
		}
		
	    }
	}
    }
}

} /*namespace PCA */

