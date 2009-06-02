#pragma once

#include <Cg/cg.h>
#include <Cg/cgGL.h>
#include <vector>
#include "Vector3.h"

class VelocityArrowsSlice {

	unsigned int _vectorFieldGraphicsID;
	float _vfXscale;
	float _vfYscale;
	float _vfZscale;
	float _slicePos;
	int _axis;
	
	unsigned int _fbo; 	
    unsigned int _tex;
	
	CGcontext _context;
	CGprogram _queryVelocityFP;
	CGparameter _ipVectorFieldParam;

	int _renderTargetWidth;
	int _renderTargetHeight;
	Vector3* _velocities;
	std::vector<Vector3> _samplingPositions;
	Vector3 _projectionVector;

	int _tickCountForMinSizeAxis;
	int _tickCountX;
	int _tickCountY;
	int _tickCountZ;
	
	int _pointCount;

	Vector3 _maxVelocityScale;
	Vector3 _arrowColor;

	bool _enabled;	
	bool _dirty;
	bool _dirtySamplingPosition;
	bool _dirtyRenderTarget;
	
private :
	void createRenderTarget();
	void computeSamplingTicks();
public :
	VelocityArrowsSlice();

	void vectorField(unsigned int vfGraphicsID, float xScale, float yScale, float zScale);
	void axis(int axis);
	int axis() const;
	void slicePos(float pos);
	float slicePos() const;
	void queryVelocity();
	void render();
	void enabled(bool e);
	bool enabled() const;
	void tickCountForMinSizeAxis(int tickCount);
	int tickCountForMinSizeAxis() const;
        void arrowColor(const Vector3& color);
};

inline int VelocityArrowsSlice::axis() const
{
	return _axis;
}

inline float VelocityArrowsSlice::slicePos() const
{
	return _slicePos;
}


inline bool VelocityArrowsSlice::enabled() const
{
	return _enabled;
}

inline int VelocityArrowsSlice::tickCountForMinSizeAxis() const
{
	return _tickCountForMinSizeAxis;
}

inline void VelocityArrowsSlice::arrowColor(const Vector3& color)
{
    _arrowColor = color;
}

