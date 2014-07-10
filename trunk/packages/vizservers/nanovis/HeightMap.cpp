 /* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include <float.h>

#include <GL/glew.h>

#include <graphics/RenderContext.h>

#include "Grid.h"
#include "HeightMap.h"
#include "ContourLineFilter.h"
#include "Texture1D.h"

using namespace nv;
using namespace nv::graphics;
using namespace vrmath;

bool HeightMap::updatePending = false;
double HeightMap::valueMin = 0.0;
double HeightMap::valueMax = 1.0;

HeightMap::HeightMap() : 
    _vertexBufferObjectID(0),
    _texcoordBufferObjectID(0),
    _vertexCount(0),
    _contour(NULL),
    _transferFunc(NULL),
    _opacity(0.5f),
    _indexBuffer(NULL),
    _indexCount(0),
    _contourColor(1.0f, 0.0f, 0.0f),
    _contourVisible(false),
    _visible(false),
    _scale(1.0f, 1.0f, 1.0f),
    _centerPoint(0.0f, 0.0f, 0.0f),
    _heights(NULL)
{
    _shader = new Shader();
    _shader->loadFragmentProgram("heightcolor.cg");
}

HeightMap::~HeightMap()
{
    reset();

    if (_shader != NULL) {
        delete _shader;
    }
    if (_heights != NULL) {
	delete [] _heights;
    }
}

void 
HeightMap::render(RenderContext *renderContext)
{
    if (!isVisible())
        return;

    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_LIGHTING_BIT);

    if (renderContext->getCullMode() == RenderContext::NO_CULL) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace((GLuint)renderContext->getCullMode());
    }
    glPolygonMode(GL_FRONT_AND_BACK, (GLuint)renderContext->getPolygonMode());
    glShadeModel((GLuint)renderContext->getShadingModel());

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

#ifndef notdef
    if (_scale.x != 0.0) {
        glScalef(1 / _scale.x, 1 / _scale.y , 1 / _scale.z);
    }
#endif
    glTranslatef(-_centerPoint.x, -_centerPoint.y, -_centerPoint.z);

    if (_contour != NULL) {
        glDepthRange(0.001, 1.0);
    }
        
    glEnable(GL_DEPTH_TEST);

    if (_vertexBufferObjectID) {
        glColor3f(1.0f, 1.0f, 1.0f);
        glShadeModel(GL_SMOOTH);
        glEnable(GL_BLEND);
        glEnableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_INDEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);

        if (_transferFunc) {
            // PUT vertex program here
            //
            //
            _shader->bind();
            _shader->setFPTextureParameter("tf", _transferFunc->id());
            _shader->setFPParameter1f("opacity", _opacity);

            glEnable(GL_TEXTURE_1D);
            _transferFunc->getTexture()->activate();
 
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        }

        glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferObjectID);
        glVertexPointer(3, GL_FLOAT, 12, 0);

        glBindBuffer(GL_ARRAY_BUFFER, _texcoordBufferObjectID);
        glTexCoordPointer(3, GL_FLOAT, 12, 0);

#define _TRIANGLES_
#ifdef _TRIANGLES_
        glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_INT, 
                       _indexBuffer);
#else
        glDrawElements(GL_QUADS, _indexCount, GL_UNSIGNED_INT, 
                       _indexBuffer);
#endif

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDisableClientState(GL_VERTEX_ARRAY);
        if (_transferFunc != NULL) {
            _transferFunc->getTexture()->deactivate();
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);

            _shader->disableFPTextureParameter("tf");
            _shader->unbind();
        }
    }
    glShadeModel(GL_FLAT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (_contour != NULL) {
        if (_contourVisible) {
            glDisable(GL_BLEND);
            glDisable(GL_TEXTURE_2D);
            glColor4f(_contourColor.x, _contourColor.y, _contourColor.z, 
                      _opacity /*1.0f*/);
            glDepthRange (0.0, 0.999);
            _contour->render();
        }

        glDepthRange (0.0, 1.0);
    }

    glPopMatrix();
    glPopAttrib();
}

void 
HeightMap::createIndexBuffer(int xCount, int zCount, float* heights)
{
    if (_indexBuffer != NULL) {
        delete [] _indexBuffer;
	_indexBuffer = NULL;
    }
    _indexCount = (xCount - 1) * (zCount - 1) * 6;
    _indexBuffer = new int[_indexCount];
   
    int i, j;
    int boundaryWidth = xCount - 1;
    int boundaryHeight = zCount - 1;
    int* ptr = _indexBuffer;
    int index1, index2, index3, index4;
    bool index1Valid, index2Valid, index3Valid, index4Valid;
    index1Valid = index2Valid = index3Valid = index4Valid = true;

    if (heights) {
        int ic = 0;
        for (i = 0; i < boundaryHeight; ++i) {
            for (j = 0; j < boundaryWidth; ++j) {
                index1 = i * xCount +j;
                if (isnan(heights[index1])) index1Valid = false;
                index2 = (i + 1) * xCount + j;
                if (isnan(heights[index2])) index2Valid = false;
                index3 = (i + 1) * xCount + j + 1;
                if (isnan(heights[index3])) index3Valid = false;
                index4 = i * xCount + j + 1;
                if (isnan(heights[index4])) index4Valid = false;
            
#ifdef _TRIANGLES_
                if (index1Valid && index2Valid && index3Valid) {
                    *ptr = index1; ++ptr;
                    *ptr = index2; ++ptr;
                    *ptr = index3; ++ptr;
                    ++ic;
                }
                if (index1Valid && index3Valid && index4Valid) {
                    *ptr = index1; ++ptr;
                    *ptr = index3; ++ptr;
                    *ptr = index4; ++ptr;
                    ++ic;
                }
#else
                if (index1Valid && index2Valid && index3Valid && index4Valid) {
                    *ptr = index1; ++ptr;
                    *ptr = index2; ++ptr;
                    *ptr = index3; ++ptr;
                    *ptr = index4; ++ptr;
                    ++ic;
                }
#endif
            }
        }
    } else {
        for (i = 0; i < boundaryHeight; ++i) {
            for (j = 0; j < boundaryWidth; ++j) {
                *ptr = i * xCount + j; ++ptr;
                *ptr = (i + 1) * xCount + j; ++ptr;
                *ptr = (i + 1) * xCount + j + 1; ++ptr;
                *ptr = i * xCount + j; ++ptr;
                *ptr = (i + 1) * xCount + j + 1; ++ptr;
                *ptr = i * xCount + j + 1; ++ptr;
            }
        }
    }
}

void 
HeightMap::reset()
{
    if (_vertexBufferObjectID) {
        glDeleteBuffers(1, &_vertexBufferObjectID);
	_vertexBufferObjectID = 0;
    }
    if (_texcoordBufferObjectID) {
        glDeleteBuffers(1, &_texcoordBufferObjectID);
	_texcoordBufferObjectID = 0;
    }
    if (_contour != NULL) {
        delete _contour;
	_contour = NULL;
    }
    if (_indexBuffer != NULL) {
        delete [] _indexBuffer;
	_indexBuffer = NULL;
    }
}
#if 0
void 
HeightMap::setHeight(int xCount, int yCount, Vector3f *heights)
{
    _vertexCount = xCount * yCount;
    reset();

    _heights = (float *)heights;
    float min, max;
    min = heights[0].y, max = heights[0].y;

    int count = xCount * yCount;
    for (int i = 0; i < count; ++i) {
        if (min > heights[i].y) {
            min = heights[i].y;
        } 
        if (max < heights[i].y) {
            max = heights[i].y;
        }
    }

    _scale.x = 1.0f;
    _scale.z = max - min;
    _scale.y = 1.0f;

    xAxis.setRange(0.0, 1.0);
    yAxis.setRange(0.0, 1.0);
    zAxis.setRange(0.0, 1.0);
    wAxis.setRange(min, max);
    updatePending = true;

    _centerPoint.set(_scale.x * 0.5, _scale.z * 0.5 + min, _scale.y * 0.5);

    Vector3f* texcoord = new Vector3f[count];
    for (int i = 0; i < count; ++i) {
        texcoord[i].set(0, 0, heights[i].y);
    }
    
    glGenBuffers(1, &_vertexBufferObjectID);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferObjectID);
    glBufferData(GL_ARRAY_BUFFER, _vertexCount * sizeof( Vector3f ), heights, 
	GL_STATIC_DRAW);
    glGenBuffers(1, &_texcoordBufferObjectID);
    glBindBuffer(GL_ARRAY_BUFFER, _texcoordBufferObjectID);
    glBufferData(GL_ARRAY_BUFFER, _vertexCount * sizeof(float) * 3, texcoord, 
	GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    delete [] texcoord;
    
    if (_contour != NULL) {
        delete _contour;
	_contour = NULL;
    }
    ContourLineFilter lineFilter;
    _contour = lineFilter.create(0.0f, 1.0f, 10, heights, xCount, yCount);

    //if (heightMap)
    //{
    //  VertexBuffer* vertexBuffer = new VertexBuffer(VertexBuffer::POSITION3, xCount * yCount, sizeof(Vector3f) * xCount * yCount, heightMap, false);
    this->createIndexBuffer(xCount, yCount, 0);
    //}
    //else
    //{
    //ERROR("HeightMap::setHeight");
    //}
}
#endif
void 
HeightMap::setHeight(float xMin, float yMin, float xMax, float yMax, 
                     int xNum, int yNum, float *heights)
{
    _vertexCount = xNum * yNum;
    _xNum = xNum, _yNum = yNum;
    _heights = heights; 
    reset();
    
    // Get the min/max of the heights. */
    float min, max;
    min = max = heights[0];
    for (int i = 0; i < _vertexCount; ++i) {
        if (min > heights[i]) {
            min = heights[i];
        } else if (max < heights[i]) {
            max = heights[i];
        }
    }
#ifdef notdef
    if (retainScale_) {
	// Check the units of each axis.  If they are the same, we want to
	// retain the surface's aspect ratio when transforming coordinates to
	// the grid. Use the range of the longest axis when the units are the
	// same.  
	if (xAxis.units() != NULL) && (xAxis.units() == yAxis.units()) {
	}
	if (yAxis.units() != NULL) && (yAxis.units() == zAxis.units()) {
	}
    }
#endif

    wAxis.setRange(min, max);
    yAxis.setRange(min, max);
    xAxis.setRange(xMin, xMax);
    zAxis.setRange(yMin, yMax);

    min = 0.0, max = 1.0;
    xMin = yMin = min = 0.0; 
    xMax = yMax = max = 1.0;
    // Save the scales.
    _scale.x = _scale.y = _scale.z = 1.0;

    updatePending = true;

    _centerPoint.set(0.5, 0.5, 0.5);
    
#ifndef notdef
    Vector3f* texcoord = new Vector3f[_vertexCount];
    for (int i = 0; i < _vertexCount; ++i) {
        texcoord[i].set(0, 0, heights[i]);
    }
    
    Vector3f* map = createHeightVertices(xMin, yMin, xMax, yMax, xNum, yNum, 
					heights);
    
    glGenBuffers(1, &_vertexBufferObjectID);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferObjectID);
    glBufferData(GL_ARRAY_BUFFER, _vertexCount * sizeof(Vector3f), map, 
        GL_STATIC_DRAW);
    glGenBuffers(1, &_texcoordBufferObjectID);
    glBindBuffer(GL_ARRAY_BUFFER, _texcoordBufferObjectID);
    glBufferData(GL_ARRAY_BUFFER, _vertexCount * sizeof(float) * 3, texcoord, 
        GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    delete [] texcoord;
    
    
    if (_contour != NULL) {
        delete _contour;
	_contour = NULL;
    }
    ContourLineFilter lineFilter;
    //lineFilter.transferFunction(_transferFunc);
    _contour = lineFilter.create(0.0f, 1.0f, 10, map, xNum, yNum);

    this->createIndexBuffer(xNum, yNum, heights);
    delete [] map;
#endif
}

Vector3f *
HeightMap::createHeightVertices(float xMin, float yMin, float xMax, 
				float yMax, int xNum, int yNum, float *height)
{
    Vector3f* vertices = new Vector3f[xNum * yNum];

    Vector3f* dstData = vertices;
    float* srcData = height;
    
    for (int y = 0; y < yNum; ++y) {
        float yCoord;

        yCoord = yMin + ((yMax - yMin) * y) / (yNum - 1); 
        for (int x = 0; x < xNum; ++x) {
            float xCoord;

            xCoord = xMin + ((xMax - xMin) * x) / (xNum - 1); 
            dstData->set(xCoord, *srcData, yCoord);

            ++dstData;
            ++srcData;
        }
    }
    return vertices;
}

/**
 * \brief Maps the data coordinates of the surface into the grid's axes.
 */
void 
HeightMap::mapToGrid(Grid *grid)
{
    int count = _xNum * _yNum;

    reset();

    // The range of the grid's y-axis 0..1 represents the distance between the
    // smallest and largest major ticks for all surfaces plotted.  Translate
    // this surface's y-values (heights) into the grid's axis coordinates.

    float yScale = 1.0 / (grid->yAxis.max() - grid->yAxis.min());
    float *p, *q, *pend;
    float *normHeights = new float[count];
    for (p = _heights, pend = p + count, q = normHeights; p < pend; p++, q++) {
	*q = (*p - grid->yAxis.min()) * yScale;
    }
    Vector3f *t, *texcoord;
    texcoord = new Vector3f[count];
    for (t = texcoord, p = normHeights, pend = p + count; p < pend; p++, t++) {
        t->set(0, 0, *p);
    }

    // Normalize the mesh coordinates (x and z min/max) the range of the major
    // ticks for the x and z grid axes as well.

    float xScale, zScale;
    float xMin, xMax, zMin, zMax;

    xScale = 1.0 / (grid->xAxis.max() - grid->xAxis.min());
    xMin = (xAxis.min() - grid->xAxis.min()) * xScale;
    xMax = (xAxis.max() - grid->xAxis.min()) * xScale;
    zScale = 1.0 / (grid->zAxis.max() - grid->zAxis.min());
    zMin = (zAxis.min() - grid->zAxis.min()) * zScale;
    zMax = (zAxis.max() - grid->zAxis.min()) * zScale;

    Vector3f* vertices;
    vertices = createHeightVertices(xMin, zMin, xMax, zMax, _xNum, _yNum, 
	normHeights);
    
    glGenBuffers(1, &_vertexBufferObjectID);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferObjectID);
    glBufferData(GL_ARRAY_BUFFER, _vertexCount * sizeof(Vector3f), vertices, 
        GL_STATIC_DRAW);
    glGenBuffers(1, &_texcoordBufferObjectID);
    glBindBuffer(GL_ARRAY_BUFFER, _texcoordBufferObjectID);
    glBufferData(GL_ARRAY_BUFFER, _vertexCount * sizeof(float) * 3, texcoord, 
        GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    delete [] texcoord;

    if (_contour != NULL) {
        delete _contour;
	_contour = NULL;
    }
    ContourLineFilter lineFilter;
    //lineFilter.transferFunction(_transferFunc);
    _contour = lineFilter.create(0.0f, 1.0f, 10, vertices, _xNum, _yNum);
    
    this->createIndexBuffer(_xNum, _yNum, normHeights);
    delete [] normHeights;
    delete [] vertices;
}

void
HeightMap::getBounds(Vector3f& bboxMin,
                     Vector3f& bboxMax) const
{
    bboxMin.set(xAxis.min(), yAxis.min(), zAxis.min());
    bboxMax.set(xAxis.max(), yAxis.max(), zAxis.max());
}