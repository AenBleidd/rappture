/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __POINT_SET_H__
#define __POINT_SET_H__

#include <PCASplit.h>
#include <Vector4.h>
#include <Vector3.h>

class PointSet {
    unsigned int _sortLevel;
    PCA::ClusterAccel* _cluster;

    Vector3 _scale;
    Vector3 _origin;
    float _max;
    float _min;
    bool _visible;
public :
    void initialize(Vector4* values, const unsigned int count, 
		    const Vector3& scale, const Vector3& origin, 
		    float min, float max);
    void updateColor(float* color, int  count);

    PointSet() :
	_sortLevel(4), 
	_cluster(0), 
	_max(1.0f),
	_min(0.0f), 
	_visible(false)
    {
	/*empty*/
    }
    ~PointSet() {
	if (_cluster) {
	    delete _cluster;
	}
    }

    bool isVisible() const {
	return _visible;
    }
    void setVisible(bool visible) {
	_visible = visible;
    }
    unsigned int getSortLevel(void) const {
	return _sortLevel;
    }
    PCA::ClusterAccel* getCluster(void) {
	return _cluster;
    }
    Vector3& getScale(void) {
	return _scale;
    }
    const Vector3& getScale(void) const {
	return _scale;
    }
    Vector3& getOrigin(void) {
	return _origin;
    }
    const Vector3& getOrigin(void) const {
	return _origin;
    }
};

#endif /*__POINT_SET_H__*/
