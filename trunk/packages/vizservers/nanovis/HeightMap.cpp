 /* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <memory.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <GL/gl.h>

#include "Grid.h"
#include "HeightMap.h"
#include "ContourLineFilter.h"
#include "Texture1D.h"
#include "R2/R2FilePath.h"
#include "RpField1D.h"
#include "RenderContext.h"

bool HeightMap::update_pending = false;
double HeightMap::valueMin = 0.0;
double HeightMap::valueMax = 1.0;

#define TOPCONTOUR	0
//#define TOPCONTOUR	1
HeightMap::HeightMap() : 
    _vertexBufferObjectID(0), 
    _textureBufferObjectID(0), 
    _vertexCount(0), 
    _contour(0), 
    _topContour(0), 
    _tfPtr(0), 
    _opacity(0.5f),
    _indexBuffer(0), 
    _indexCount(0), 
    _contourColor(1.0f, 0.0f, 0.0f), 
    _contourVisible(false), 
    _topContourVisible(true),
    _visible(false),
    _scale(1.0f, 1.0f, 1.0f), 
    _centerPoint(0.0f, 0.0f, 0.0f),
    _heights(NULL)
{
    _shader = new NvShader();
    _shader->loadFragmentProgram("heightcolor.cg", "main");
    _tfParam      = _shader->getNamedParameterFromFP("tf");
    _opacityParam = _shader->getNamedParameterFromFP("opacity");
}

HeightMap::~HeightMap()
{
    reset();

    if (_shader) {
        delete _shader;
    }
    if (_heights != NULL) {
	free(_heights);
    }
}

void 
HeightMap::render(graphics::RenderContext* renderContext)
{
    if (renderContext->getCullMode() == graphics::RenderContext::NO_CULL) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace((GLuint) renderContext->getCullMode());
    }
    glPolygonMode(GL_FRONT_AND_BACK, (GLuint) renderContext->getPolygonMode());
    glShadeModel((GLuint) renderContext->getShadingModel());

    glPushMatrix();

#ifndef notdef
    if (_scale.x != 0.0) {
        glScalef(1 / _scale.x, 1 / _scale.y , 1 / _scale.z);
    }
#endif
    glTranslatef(-_centerPoint.x, -_centerPoint.y, -_centerPoint.z);

    if (_contour != NULL) {
        glDepthRange (0.001, 1.0);
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
        
        if (_tfPtr) {
            // PUT vertex program here
            //
            //
            
            cgGLBindProgram(_shader->getFP());
            cgGLEnableProfile(CG_PROFILE_FP30);
            
            cgGLSetTextureParameter(_tfParam, _tfPtr->id());
            cgGLEnableTextureParameter(_tfParam);
            cgGLSetParameter1f(_opacityParam, _opacity);
            
            glEnable(GL_TEXTURE_1D);
            _tfPtr->getTexture()->activate();
            
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferObjectID);
        glVertexPointer(3, GL_FLOAT, 12, 0);
        
        glBindBuffer(GL_ARRAY_BUFFER, _textureBufferObjectID);
        ::glTexCoordPointer(3, GL_FLOAT, 12, 0);
        
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
        if (_tfPtr != NULL) {
            _tfPtr->getTexture()->deactivate();
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            
            cgGLDisableProfile(CG_PROFILE_FP30);
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
            glDepthRange (0.0, 1.0);
        }

#if TOPCONTOUR
        if (_topContourVisible) {
            glDisable(GL_BLEND);
            glDisable(GL_TEXTURE_2D);
            glColor4f(_contourColor.x, _contourColor.y, _contourColor.z, 
		_opacity /*1.0f*/);
            //glDepthRange (0.0, 0.999);
            _topContour->render();
            //glDepthRange (0.0, 1.0);
        }
#endif
    }
    glPopMatrix();
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
    if (_textureBufferObjectID) {
        glDeleteBuffers(1, &_textureBufferObjectID);
	_textureBufferObjectID = 0;
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

void 
HeightMap::setHeight(int xCount, int yCount, Vector3* heights)
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

    xAxis.SetRange(0.0, 1.0);
    yAxis.SetRange(0.0, 1.0);
    zAxis.SetRange(0.0, 1.0);
    wAxis.SetRange(min, max);
    update_pending = true;
    
    _centerPoint.set(_scale.x * 0.5, _scale.z * 0.5 + min, _scale.y * 0.5);

    Vector3* texcoord = new Vector3[count];
    for (int i = 0; i < count; ++i) {
        texcoord[i].set(0, 0, heights[i].y);
    }
    
    glGenBuffers(1, &_vertexBufferObjectID);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferObjectID);
    glBufferData(GL_ARRAY_BUFFER, _vertexCount * sizeof( Vector3 ), heights, 
	GL_STATIC_DRAW);
    glGenBuffers(1, &_textureBufferObjectID);
    glBindBuffer(GL_ARRAY_BUFFER, _textureBufferObjectID);
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

#if TOPCONTOUR
    ContourLineFilter topLineFilter;
    topLineFilter.setHeightTop(true);
    _topContour = topLineFilter.create(0.0f, 1.0f, 10, heights, xCount, yCount);
#endif

    //if (heightMap)
    //{
    //  VertexBuffer* vertexBuffer = new VertexBuffer(VertexBuffer::POSITION3, xCount * yCount, sizeof(Vector3) * xCount * yCount, heightMap, false);
    this->createIndexBuffer(xCount, yCount, 0);
    //}
    //else
    //{
    //ERROR("HeightMap::setHeight\n");
    //}
}

void 
HeightMap::setHeight(float xMin, float yMin, float xMax, float yMax, 
                     int xNum, int yNum, float* heights)
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

    wAxis.SetRange(min, max);
    yAxis.SetRange(min, max);
    xAxis.SetRange(xMin, xMax);
    zAxis.SetRange(yMin, yMax);
    
    
    min = 0.0, max = 1.0;
    xMin = yMin = min = 0.0; 
    xMax = yMax = max = 1.0;
    // Save the scales.
    _scale.x = _scale.y = _scale.z = 1.0;

    update_pending = true;

    _centerPoint.set(0.5, 0.5, 0.5);
    
#ifndef notdef
    Vector3* texcoord = new Vector3[_vertexCount];
    for (int i = 0; i < _vertexCount; ++i) {
        texcoord[i].set(0, 0, heights[i]);
    }
    
    Vector3* map = createHeightVertices(xMin, yMin, xMax, yMax, xNum, yNum, 
					heights);
    
    glGenBuffers(1, &_vertexBufferObjectID);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferObjectID);
    glBufferData(GL_ARRAY_BUFFER, _vertexCount * sizeof(Vector3), map, 
        GL_STATIC_DRAW);
    glGenBuffers(1, &_textureBufferObjectID);
    glBindBuffer(GL_ARRAY_BUFFER, _textureBufferObjectID);
    glBufferData(GL_ARRAY_BUFFER, _vertexCount * sizeof(float) * 3, texcoord, 
        GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    delete [] texcoord;
    
    
    if (_contour != NULL) {
        delete _contour;
	_contour = NULL;
    }
    ContourLineFilter lineFilter;
    //lineFilter.transferFunction(_tfPtr);
    _contour = lineFilter.create(0.0f, 1.0f, 10, map, xNum, yNum);
    
#if TOPCONTOUR
    ContourLineFilter topLineFilter;
    topLineFilter.setHeightTop(true);
    _topContour = topLineFilter.create(0.0f, 1.0f, 10, map, xNum, yNum);
#endif
    this->createIndexBuffer(xNum, yNum, heights);
    delete [] map;
#endif
}

Vector3* 
HeightMap::createHeightVertices(float xMin, float yMin, float xMax, 
				float yMax, int xNum, int yNum, float* height)
{
    Vector3* vertices = new Vector3[xNum * yNum];

    Vector3* dstDataPtr = vertices;
    float* srcDataPtr = height;
    
    for (int y = 0; y < yNum; ++y) {
        float yCoord;

        yCoord = yMin + ((yMax - yMin) * y) / (yNum - 1); 
        for (int x = 0; x < xNum; ++x) {
            float xCoord;

            xCoord = xMin + ((xMax - xMin) * x) / (xNum - 1); 
            dstDataPtr->set(xCoord, *srcDataPtr, yCoord);

            ++dstDataPtr;
            ++srcDataPtr;
        }
    }
    return vertices;
}

// Maps the data coordinates of the surface into the grid's axes.
void 
HeightMap::MapToGrid(Grid *gridPtr)
{
    int count = _xNum * _yNum;

    reset();

    // The range of the grid's y-axis 0..1 represents the distance between the
    // smallest and largest major ticks for all surfaces plotted.  Translate
    // this surface's y-values (heights) into the grid's axis coordinates.

    float yScale = 1.0 / (gridPtr->yAxis.max() - gridPtr->yAxis.min());
    float *p, *q, *pend;
    float *normHeights = new float[count];
    for (p = _heights, pend = p + count, q = normHeights; p < pend; p++, q++) {
	*q = (*p - gridPtr->yAxis.min()) * yScale;
    }
    Vector3 *t, *texcoord;
    texcoord = new Vector3[count];
    for (t = texcoord, p = normHeights, pend = p + count; p < pend; p++, t++) {
        t->set(0, 0, *p);
    }

    // Normalize the mesh coordinates (x and z min/max) the range of the major
    // ticks for the x and z grid axes as well.

    float xScale, zScale;
    float xMin, xMax, zMin, zMax;

    xScale = 1.0 / (gridPtr->xAxis.max() - gridPtr->xAxis.min());
    xMin = (xAxis.min() - gridPtr->xAxis.min()) * xScale;
    xMax = (xAxis.max() - gridPtr->xAxis.min()) * xScale;
    zScale = 1.0 / (gridPtr->zAxis.max() - gridPtr->zAxis.min());
    zMin = (zAxis.min() - gridPtr->zAxis.min()) * zScale;
    zMax = (zAxis.max() - gridPtr->zAxis.min()) * zScale;

    Vector3* vertices;
    vertices = createHeightVertices(xMin, zMin, xMax, zMax, _xNum, _yNum, 
	normHeights);
    
    glGenBuffers(1, &_vertexBufferObjectID);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferObjectID);
    glBufferData(GL_ARRAY_BUFFER, _vertexCount * sizeof(Vector3), vertices, 
        GL_STATIC_DRAW);
    glGenBuffers(1, &_textureBufferObjectID);
    glBindBuffer(GL_ARRAY_BUFFER, _textureBufferObjectID);
    glBufferData(GL_ARRAY_BUFFER, _vertexCount * sizeof(float) * 3, texcoord, 
        GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    delete [] texcoord;

    if (_contour != NULL) {
        delete _contour;
	_contour = NULL;
    }
    ContourLineFilter lineFilter;
    //lineFilter.transferFunction(_tfPtr);
    _contour = lineFilter.create(0.0f, 1.0f, 10, vertices, _xNum, _yNum);
    
#if TOPCONTOUR
    ContourLineFilter topLineFilter;
    topLineFilter.setHeightTop(true);
    _topContour = topLineFilter.create(0.0f, 1.0f, 10, vertices, _xNum, _yNum);
#endif
    this->createIndexBuffer(_xNum, _yNum, normHeights);
    delete [] normHeights;
    delete [] vertices;
}

void 
HeightMap::render_topview(graphics::RenderContext* renderContext, 
			  int render_width, int render_height)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, render_width, render_height);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    //gluOrtho2D(0, render_width, 0, render_height);
    glOrtho(-.5, .5, -.5, .5, -50, 50);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glTranslatef(0.0, 0.0, -10.0);

    // put camera rotation and translation
    //glScalef(1 / _scale.x, 1 / _scale.y , 1 / _scale.z);

    if (renderContext->getCullMode() == graphics::RenderContext::NO_CULL) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace((GLuint) renderContext->getCullMode());
    }

    glPolygonMode(GL_FRONT_AND_BACK, (GLuint) renderContext->getPolygonMode());
    glShadeModel((GLuint) renderContext->getShadingModel());

    glPushMatrix();

    //glTranslatef(-_centerPoint.x, -_centerPoint.y, -_centerPoint.z);

    //glScalef(0.01, 0.01, 0.01f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glTranslatef(-_centerPoint.x, -_centerPoint.y, -_centerPoint.z);
    if (_contour != NULL) {
        glDepthRange (0.001, 1.0);
    }
        
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    if (_vertexBufferObjectID) 
    {
        TRACE("TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT\n");
        glColor3f(1.0f, 1.0f, 1.0f);
        glShadeModel(GL_SMOOTH);
        glEnable(GL_BLEND);
        glEnableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_INDEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        
        if (_tfPtr != NULL) {
            cgGLBindProgram(_shader->getFP());
            cgGLEnableProfile(CG_PROFILE_FP30);
            
            cgGLSetTextureParameter(_tfParam, _tfPtr->id());
            cgGLEnableTextureParameter(_tfParam);
            
            glEnable(GL_TEXTURE_1D);
            _tfPtr->getTexture()->activate();
            
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        }
        else {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferObjectID);
        glVertexPointer(3, GL_FLOAT, 12, 0);
        
        glBindBuffer(GL_ARRAY_BUFFER, _textureBufferObjectID);
        ::glTexCoordPointer(3, GL_FLOAT, 12, 0);
        
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
        if (_tfPtr != NULL) {
            _tfPtr->getTexture()->deactivate();
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            
            cgGLDisableProfile(CG_PROFILE_FP30);
        }
    }
    
    glShadeModel(GL_FLAT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    if (_contour != NULL) {
        if (_contourVisible) {
            glDisable(GL_BLEND);
            glDisable(GL_TEXTURE_2D);
            glColor4f(_contourColor.x, _contourColor.y, _contourColor.z, 1.0f);
            glDepthRange (0.0, 0.999);
            _contour->render();
            glDepthRange (0.0, 1.0);
        }

#if TOPCONTOUR
        if (_topContourVisible) {
            glDisable(GL_BLEND);
            glDisable(GL_TEXTURE_2D);
            glColor4f(_contourColor.x, _contourColor.y, _contourColor.z, 1.0f);
            //glDepthRange (0.0, 0.999);
            _topContour->render();
            //glDepthRange (0.0, 1.0);
        }
#endif
    }
    
    glPopMatrix();
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);

    glPopAttrib();
}
