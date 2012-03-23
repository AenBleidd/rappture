/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef HEIGHTMAP_H
#define HEIGHTMAP_H

#include <Cg/cg.h>

#include <R2/graphics/R2Geometry.h>

#include "TransferFunction.h"
#include "NvShader.h"
#include "Vector3.h"
#include "RenderContext.h"
#include "AxisRange.h"

namespace graphics {
    class RenderContext;
}

class Grid;

/**
 *@brief Create a surface from height map and line contour of the generated surface
 */
class HeightMap
{
public:
    HeightMap();

    ~HeightMap();

    void render(graphics::RenderContext *renderContext);

    void render_topview(graphics::RenderContext *renderContext,
                        int render_width, int render_height);

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
    void setHeight(float startX, float startY, float endX, float endY, 
		   int xCount, int yCount, float *height);

    /**
     *@brief Create a height map with a set of points
     *@param xCount the number of columns of height values
     *@param yCount the number of rows of height values
     */
    void setHeight(int xCount, int yCount, Vector3 *heights);

    void MapToGrid(Grid *gridPtr);

    /**
     *@brief Define a color map for color shading of heightmap
     */
    void transferFunction(TransferFunction *tfPtr)
    {
        _tfPtr = tfPtr;
    }

    /**
     *@brief Get the color map defined for shading of this heightmap
     */
    TransferFunction *transferFunction()
    {
        return _tfPtr;
    }

    /**
     *@brief Set the visibility of the height map
     */
    void setVisible(bool visible)
    {
        _visible = visible;
    }

    /**
     *@brief Return the status of the visibility
     */
    bool isVisible() const
    {
        return _visible;
    }

    /**
     *@brief Set the visibility of the line contour
     */
    void setLineContourVisible(bool visible)
    {
        _contourVisible = visible;
    }

    void setTopLineContourVisible(bool visible)
    {
        _topContourVisible = visible;
    }

    void opacity(float opacity)
    {
        _opacity = opacity;
    }

    float opacity()
    {
	return _opacity;
    }

    /**
     *@brief Defind the color of the line contour
     */
    void setLineContourColor(float *rgb)
    {
	_contourColor.x = rgb[0];
	_contourColor.y = rgb[1];
	_contourColor.z = rgb[2];
    }

    AxisRange xAxis, yAxis, zAxis, wAxis;
    static bool updatePending;
    static double valueMin, valueMax;

private:
    void createIndexBuffer(int xCount, int zCount, float* heights);
    Vector3 *createHeightVertices(float startX, float startY,
                                  float endX, float endY,
                                  int xCount, int yCount, float  *height);
    void reset();

    unsigned int _vertexBufferObjectID;
    unsigned int _textureBufferObjectID;
    int _vertexCount;
    CGparameter _tfParam;
    CGparameter _opacityParam;
    R2Geometry *_contour;
    R2Geometry *_topContour;
    TransferFunction *_tfPtr;
    float _opacity;
    NvShader *_shader;
    int *_indexBuffer;
    int _indexCount;
    Vector3 _contourColor;
    
    bool _contourVisible;
    bool _topContourVisible;
    bool _visible;
    
    Vector3 _scale;
    Vector3 _centerPoint;
    int _xNum, _yNum;		// Number of elements x and y axes in grid.
    float *_heights;		// Array of original (unscaled) heights
				// (y-values)
};

#endif
