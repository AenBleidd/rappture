 
#include <GL/glew.h>
#include <GL/gl.h>
#include "HeightMap.h"
#include "ContourLineFilter.h"
#include "TypeDefs.h"
#include <Texture1D.h>
#include <stdlib.h>
#include <memory.h>
#include <R2/R2FilePath.h>
#include "RpField1D.h"
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <RenderContext.h>

HeightMap::HeightMap() : 
    _vertexBufferObjectID(0), 
    _textureBufferObjectID(0), 
    _vertexCount(0), 
    _contour(0), 
    _colorMap(0), 
    _indexBuffer(0), 
    _indexCount(0), 
    _contourColor(1.0f, 0.0f, 0.0f), 
    _contourVisible(true), 
    _visible(false),
    _scale(1.0f, 1.0f, 1.0f), 
    _centerPoint(0.0f, 0.0f, 0.0f)
{
    _shader = new NvShader();

    R2string path = R2FilePath::getInstance()->getPath("heightcolor.cg");
    if (path.getLength() == 0) {
        printf("ERROR : file not found %s\n", "heightcolor.cg");
    }
    _shader->loadFragmentProgram(path, "main");
    _tf = _shader->getNamedParameterFromFP("tf");
}

HeightMap::~HeightMap()
{
    reset();

    if (_shader) {
        delete _shader;
    }

    // TMP
    //if (_colorMap) delete _colorMap;
}

void HeightMap::render(graphics::RenderContext* renderContext)
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

    if (_scale.x != 0) {
        glScalef(1 / _scale.x, 1 / _scale.x , 1 / _scale.x);
    }

    glTranslatef(-_centerPoint.x, -_centerPoint.y, -_centerPoint.z);

    if (_contour) {
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
        
        if (_colorMap) {
            cgGLBindProgram(_shader->getFP());
            cgGLEnableProfile(CG_PROFILE_FP30);
            
            cgGLSetTextureParameter(_tf, _colorMap->id);
            cgGLEnableTextureParameter(_tf);
            
            glEnable(GL_TEXTURE_1D);
            _colorMap->getTexture()->activate();
            
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferObjectID);
        glVertexPointer(3, GL_FLOAT, 12, 0);
        
        glBindBuffer(GL_ARRAY_BUFFER, _textureBufferObjectID);
        ::glTexCoordPointer(3, GL_FLOAT, 12, 0);
        
        glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_INT, 
                       _indexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        glDisableClientState(GL_VERTEX_ARRAY);
        if (_colorMap) {
            _colorMap->getTexture()->deactivate();
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            
            cgGLDisableProfile(CG_PROFILE_FP30);
        }
    }
    
    glShadeModel(GL_FLAT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    if (_contour && _contourVisible) {
        glDisable(GL_BLEND);
        glDisable(GL_TEXTURE_2D);
        glColor4f(_contourColor.x, _contourColor.y, _contourColor.z, 1.0f);
        glDepthRange (0.0, 0.999);
        _contour->render();
        glDepthRange (0.0, 1.0);
    }
    glPopMatrix();
}

void HeightMap::createIndexBuffer(int xCount, int zCount, int*& indexBuffer, int& indexCount, float* heights)
{
    indexCount = (xCount - 1) * (zCount - 1) * 6;

    indexBuffer = new int[indexCount];
   
    int i, j;
    int boundaryWidth = xCount - 1;
    int boundaryHeight = zCount - 1;
    int* ptr = indexBuffer;
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

void HeightMap::reset()
{
    if (_vertexBufferObjectID) {
        glDeleteBuffers(1, &_vertexBufferObjectID);
    }

    if (_vertexBufferObjectID) {
        glDeleteBuffers(1, &_vertexBufferObjectID);
    }

    //if (_contour) _contour->unrefDelete();
    if (_contour) {
        delete _contour;
    }
    if (_indexBuffer) {
        free(_indexBuffer);
    }
}

void HeightMap::setHeight(int xCount, int yCount, Vector3* heights)
{
    _vertexCount = xCount * yCount;
    reset();
    
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

    SetRange(AxisRange::VALUES, min, max);
    SetRange(AxisRange::X, 0.0, 1.0);
    SetRange(AxisRange::Y, 0.0, 1.0);
    SetRange(AxisRange::Z, min, max);

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
    
    
    ContourLineFilter lineFilter;
    _contour = lineFilter.create(0.0f, 1.0f, 10, heights, xCount, yCount);

    //if (heightMap)
    //{
    //  VertexBuffer* vertexBuffer = new VertexBuffer(VertexBuffer::POSITION3, xCount * yCount, sizeof(Vector3) * xCount * yCount, heightMap, false);
    if (_indexBuffer) {
        free(_indexBuffer);
    }

    this->createIndexBuffer(xCount, yCount, _indexBuffer, _indexCount, 0);
    //}
    //else
    //{
    //printf("ERROR - HeightMap::setHeight\n");
    //}
}

void 
HeightMap::setHeight(float startX, float startY, float endX, float endY, 
                     int xCount, int yCount, float* heights)
{
    _vertexCount = xCount * yCount;
    
    reset();

    float min, max;
    min = heights[0], max = heights[0];
    int count = xCount * yCount;
    for (int i = 0; i < count; ++i) {
        if (min > heights[i]) {
            min = heights[i];
        } else if (max < heights[i]) {
            max = heights[i];
        }
    }
    _scale.x = endX - startX;
    _scale.y = max - min;
    _scale.z = endY - startY;

    SetRange(AxisRange::VALUES, min, max);
    SetRange(AxisRange::X, startX, endX);
    SetRange(AxisRange::Y, min, max);
    SetRange(AxisRange::Z, startY, endY);

    _centerPoint.set(_scale.x * 0.5 + startX, _scale.y * 0.5 + min, 
	_scale.z * 0.5 + startY);
    
    Vector3* texcoord = new Vector3[count];
    for (int i = 0; i < count; ++i) {
        texcoord[i].set(0, 0, heights[i]);
    }
    
    Vector3* heightMap = createHeightVertices(startX, startY, endX, endY, xCount, yCount, heights);
    
    glGenBuffers(1, &_vertexBufferObjectID);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferObjectID);
    glBufferData(GL_ARRAY_BUFFER, _vertexCount * sizeof( Vector3 ), heightMap, 
        GL_STATIC_DRAW);
    glGenBuffers(1, &_textureBufferObjectID);
    glBindBuffer(GL_ARRAY_BUFFER, _textureBufferObjectID);
    glBufferData(GL_ARRAY_BUFFER, _vertexCount * sizeof(float) * 3, texcoord, 
        GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    delete [] texcoord;
    
    
    ContourLineFilter lineFilter;
    //lineFilter.setColorMap(_colorMap);
    _contour = lineFilter.create(0.0f, 1.0f, 10, heightMap, xCount, yCount);
    
    //if (heightMap)
    //{
    //  VertexBuffer* vertexBuffer = new VertexBuffer(VertexBuffer::POSITION3, xCount * yCount, 
    // sizeof(Vector3) * xCount * yCount, heightMap, false);
    if (_indexBuffer) {
        free(_indexBuffer);
    }
    this->createIndexBuffer(xCount, yCount, _indexBuffer, _indexCount, heights);
    //}
    //else
    //{
    //printf("ERROR - HeightMap::setHeight\n");
    //}
}

Vector3* HeightMap::createHeightVertices(float startX, float startY, float endX, float endY, int xCount, int yCount, float* height)
{
    Vector3* vertices = (Vector3*) malloc(sizeof(Vector3) * xCount * yCount);

    Vector3* dstDataPtr = vertices;
    float* srcDataPtr = height;
    
    for (int y = 0; y < yCount; ++y) {
        float yCoord;

        yCoord = startY + ((endY - startY) * y) / (yCount - 1); 
        for (int x = 0; x < xCount; ++x) {
            float xCoord;

            xCoord = startX + ((endX - startX) * x) / (xCount - 1); 
            dstDataPtr->set(xCoord, *srcDataPtr, yCoord);

            ++dstDataPtr;
            ++srcDataPtr;
        }
    }
    return vertices;
}

void HeightMap::setColorMap(TransferFunction* colorMap)
{
    //if (colorMap) colorMap->addRef();
    //if (_colorMap) _colorMap->unrefDelete();
    _colorMap = colorMap;
}

