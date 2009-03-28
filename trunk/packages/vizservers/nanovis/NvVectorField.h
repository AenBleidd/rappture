#pragma once

#include "Volume.h"
#include "Vector3.h"
#include "NvParticleRenderer.h"

#include <string>
#include <map>

class NvDeviceShape {
public :
	Vector3 min;
	Vector3 max;
	Vector4 color;
	bool visible;
public :
	NvDeviceShape()
	: visible(true)
	{
	}
};

class NvVectorField {
	unsigned int _vectorFieldID;
	Volume* _volPtr;
	std::map<std::string, NvParticleRenderer*> _particleRendererMap;

	std::map<std::string, NvDeviceShape> _shapeMap;

	/**
         * @brief Specify the visibility
         */
	bool _activated;

	Vector3 _origin;
	Vector3 _physicalMin;
	Vector3 _physicalSize;
	float _scaleX;
	float _scaleY;
	float _scaleZ;
	float _max;
	
	bool _deviceVisible;
public :
	NvVectorField();
	~NvVectorField();

	void setVectorField(Volume* vol, const Vector3& ori, float scaleX, float scaleY, float scaleZ, float max);

	void activate();
	void deactivate();
	bool isActivated() const;

	/////////////////////////////
	// DEVICE
	void addDeviceShape(const std::string& name, const NvDeviceShape& shape);
	void removeDeviceShape(const std::string& name);
	void activateDeviceShape();
	void deactivateDeviceShape();
	void activateDeviceShape(const std::string& name);
	void deactivateDeviceShape(const std::string& name);

	void initialize();
	void reset();

	void addPlane(const std::string& name);
	void removePlane(const std::string& name);

	void advect();
	void render();

	void drawDeviceShape();
	void activatePlane(const std::string& name);
	void deactivatePlane(const std::string& name);
	void setPlaneAxis(const std::string& name, int axis);
	void setPlanePos(const std::string& name, float pos);
	void setParticleColor(const std::string& name, float r, float g, float b, float a);
	void setParticleColor(const std::string& name, const Vector4& color);
};

inline void NvVectorField::activate()
{
	_activated = true;
}

inline void NvVectorField::deactivate()
{
	_activated = false;
}

inline bool NvVectorField::isActivated() const
{
	return _activated;
}

inline void NvVectorField::activateDeviceShape()
{
	_deviceVisible = true;
}

inline void NvVectorField::deactivateDeviceShape()
{
	_deviceVisible = false;
}

