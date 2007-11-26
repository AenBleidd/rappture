#ifndef _HEIGHT_MAP_H_
#define _HEIGHT_MAP_H_

#include <Cg/cgGL.h>
#include <Cg/cg.h>
#include <R2/graphics/R2Geometry.h>
#include "TransferFunction.h"
#include "NvShader.h"
#include "Vector3.h"

/**
 *@class HeightMap
 *@brief Create a surface from height map and line contour of the generated surface
 */
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
    /**
     *@brief Constructor
     */
	HeightMap();
    /**
     *@brief Destructor
     */
	~HeightMap();

private :
	void createIndexBuffer(int xCount, int zCount, int*& indexBuffer, int& indexCount, float* heights);
	Vector3* createHeightVertices(float startX, float startY, float endX, float endY, int xCount, int yCount, float* height);
	void reset();
public :
	void render();
    /**
     *@brief Create a height map with heigh values
     *@param startX a x position of the first height value
     *@param startY a y position of the first height value
     *@param endX a x position of the last height value
     *@param endY a y position of the last height value
     *@param xCount the number of columns of height values
     *@param yCount the number of rows of height values
     *@param height a pointer value adrressing xCount * yCount values of heights
     */
	void setHeight(float startX, float startY, float endX, float endY, int xCount, int yCount, float* height);

    /**
     *@brief Create a height map with a set of points
     *@param xCount the number of columns of height values
     *@param yCount the number of rows of height values
     */
	void setHeight(int xCount, int yCount, Vector3* heights);

    /**
     *@brief Define a color map for color shading of heighmap
     */
	void setColorMap(TransferFunction* colorMap);

    /**
     *@brief Set the visibility of the height map
     */
	void setVisible(bool visible);

    /**
     *@brief Return the status of the visibility
     */
	bool isVisible() const;

    /**
     *@brief Set the visibility of the line contour
     */
	void setLineContourVisible(bool visible);

    /**
     *@brief Defind the color of the line contour
     */
    void setLineContourColor(float r, float g, float b);
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

inline void HeightMap::setLineContourColor(float r, float g, float b)
{
    _contourColor.x = r;
    _contourColor.y = g;
    _contourColor.z = b;
}
#endif
