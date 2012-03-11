/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <GL/glew.h>
#include <GL/gl.h>

#include <R2/graphics/R2Geometry.h>

R2Geometry::R2Geometry(int primitive, R2VertexBuffer* vertexBuffer, 
                       R2IndexBuffer* indexBuffer) : 
    _vertexBuffer(vertexBuffer), 
    _colorBuffer(0),
    _indexBuffer(indexBuffer), 
    _primitiveType(primitive)
{
}

R2Geometry::R2Geometry(int primitive, R2VertexBuffer* vertexBuffer, 
                       R2VertexBuffer* colorBuffer, R2IndexBuffer* indexBuffer) : 
    _vertexBuffer(vertexBuffer), 
    _colorBuffer(colorBuffer),
    _indexBuffer(indexBuffer), 
    _primitiveType(primitive) 
{
}


R2Geometry::~R2Geometry()
{
}

void 
R2Geometry::render()
{
    //glDisableClientState(GL_NORMAL_ARRAY);
    //glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer->getGraphicObjectID());
    glVertexPointer(3, GL_FLOAT, 0, 0);
    
    if (_colorBuffer) {
        glEnableClientState(GL_COLOR_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, _colorBuffer->getGraphicObjectID());
        glColorPointer(3, GL_FLOAT, 0, 0);
    } else {
        glDisableClientState(GL_COLOR_ARRAY);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    if (_indexBuffer) {
        glDrawElements(GL_QUADS, _indexBuffer->getIndexCount(), GL_UNSIGNED_INT, 
                       _indexBuffer->getData());
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(_primitiveType, 0, _vertexBuffer->getVertexCount());
    }
    
    glDisableClientState(GL_VERTEX_ARRAY);
    if (_colorBuffer) {
        glDisableClientState(GL_COLOR_ARRAY);
    }
}
