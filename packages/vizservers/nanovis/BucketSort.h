/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __BUCKETSORT_H_
#define __BUCKETSORT_H_

#include <vector>
#include <list>
#include "Vector3.h"
#include "Mat4x4.h"
#include "PCASplit.h"

namespace PCA {

class ClusterList {
public :
	Cluster* data;
	ClusterList* next;
public :
	ClusterList(Cluster* d, ClusterList* n)
		: data(d), next(n)
	{}

	ClusterList()
		: data(0), next(0)
	{}

	~ClusterList()
	{
		//if (next) delete next;
	}
};

class BucketSort {
	ClusterList** _buffer;
	int _size;
	int _count;

	int _memChuckSize;
	bool _memChunckUsedFlag;
	ClusterList* _memChunck;
	
	float _invMaxLength;
private :
	//void _sort(Cluster* cluster, const Matrix4& cameraMat, int level);
	void _sort(Cluster* cluster, const Mat4x4& cameraMat, int level);
public :
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

	int getSize() const { return _size; }
	int getSortedItemCount() const { return _count; }
	void init();
	//void sort(ClusterAccel* cluster, const Matrix4& cameraMat, float maxLength);
	//void sort(ClusterAccel* cluster, const Matrix4& cameraMat, int level);
	void sort(ClusterAccel* cluster, const Mat4x4& cameraMat, int level);
	void addBucket(Cluster* cluster, float ratio);
	ClusterList** getBucket() { return _buffer; }
};

}

#endif 
