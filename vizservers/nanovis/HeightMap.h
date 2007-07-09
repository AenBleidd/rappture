#ifndef _HEIGHT_MAP_H_
#define _HEIGHT_MAP_H_

#include <Cg/cgGL.h>
#include <Cg/cg.h>
#include <R2/graphics/R2Geometry.h>
#include "TransferFunction.h"
#include "NvShader.h"
#include "Vector3.h"

class HeightMap {
	unsigned int _vertexBufferObjectID;
	unsigned int _textureBufferObjectID;

	int _vertexCount;
	CGparameter _tf;
	R2Geometry* _contour;
	TransferFunction* _colorMap;
	NvShader* _shader;
	int* _indexBuffer;
	int _indexCount;
    Vector3 _contourColor;

	bool _contourVisible;
	bool _visible;

    Vector3 _scale;

public :
	HeightMap();
	~HeightMap();

private :
	void createIndexBuffer(int xCount, int zCount, int*& indexBuffer, int& indexCount);
	Vector3* createHeightVertices(float startX, float startY, float endX, float endY, int xCount, int yCount, float* height);
	void reset();
public :
	void render();
	void setHeight(float startX, float startY, float endX, float endY, int xCount, int yCount, float* height);
	void setHeight(int xCount, int yCount, Vector3* heights);
	void setColorMap(TransferFunction* colorMap);

	void setVisible(bool visible);
	bool isVisible() const;
	void setLineContourVisible(bool visible);
};

inline void HeightMap::setVisible(bool visible)
{
	_visible = visible;
}

inline bool HeightMap::isVisible() const
{
	return _visible;
}

inline void HeightMap::setLineContourVisible(bool visible)
{
	_contourVisible = visible;
}

#endif
