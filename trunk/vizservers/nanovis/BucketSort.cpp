#include "BucketSort.h"

using namespace PCA;

void BucketSort::init()
{
	_count = 0;
	memset(_buffer, 0, sizeof(ClusterList*) * _size);
}

void BucketSort::sort(ClusterAccel* clusterAccel, const Mat4x4& cameraMat, int level)
{
	if (clusterAccel == 0) return;

	Cluster* cluster = clusterAccel->startPointerCluster[level - 1];
	Cluster* c = &(cluster[0]);
	Cluster* end = &(cluster[clusterAccel->numOfClusters[level - 1] - 1]);

	Vector3 pos;
	int count = clusterAccel->numOfClusters[level - 1];
	
	for (; c <= end; c = (Cluster*) ((int) c + sizeof(Cluster)))
	{
		pos.transform(c->centroid, cameraMat);
		addBucket(c, pos.length()*_invMaxLength);
	}
}	

void BucketSort::addBucket(Cluster* cluster, float ratio)
{
	int index = (int) (ratio * _size);
	ClusterList* c = &(_memChunck[_count++]);
	c->data = cluster;
	c->next =_buffer[index];
	_buffer[index] = c;
}

