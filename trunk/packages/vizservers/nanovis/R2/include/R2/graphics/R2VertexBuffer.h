/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef R2_VERTEXBUFFER_H
#define R2_VERTEXBUFFER_H

class R2VertexBuffer
{
public:
    enum {
        POSITION3 = 0x01,
        NORMAL3 = 0x02,
        COLOR3 = 0x04,
        COLOR4 = 0x08
    };

    R2VertexBuffer(int type, int vertexCount,
                   int byteSize, void *data, bool copy = true);

    ~R2VertexBuffer();

    unsigned int getGraphicObjectID() const;

    void updateBuffer(void *data);

    int getVertexCount() const;

    void *_data;

private:
    unsigned int _graphicObjectID;
    int _byteSize;
    int _vertexCount;
};

inline int R2VertexBuffer::getVertexCount() const
{
    return _vertexCount;
}

inline unsigned int R2VertexBuffer::getGraphicObjectID() const
{
    return _graphicObjectID;
}

#endif
