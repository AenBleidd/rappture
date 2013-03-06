/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_GRAPHICS_VERTEXBUFFER_H
#define NV_GRAPHICS_VERTEXBUFFER_H

namespace nv {
namespace graphics {

class VertexBuffer
{
public:
    enum {
        POSITION3 = 0x01,
        NORMAL3 = 0x02,
        COLOR3 = 0x04,
        COLOR4 = 0x08
    };

    VertexBuffer(int type, int vertexCount,
                 int byteSize, void *data, bool copy = true);

    ~VertexBuffer();

    unsigned int getGraphicObjectID() const;

    void updateBuffer(void *data);

    int getVertexCount() const;

    void *_data;

private:
    unsigned int _graphicObjectID;
    int _byteSize;
    int _vertexCount;
};

inline int VertexBuffer::getVertexCount() const
{
    return _vertexCount;
}

inline unsigned int VertexBuffer::getGraphicObjectID() const
{
    return _graphicObjectID;
}

}
}

#endif
