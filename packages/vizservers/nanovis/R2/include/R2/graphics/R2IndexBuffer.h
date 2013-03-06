/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_GRAPHICS_INDEX_BUFFER_H
#define NV_GRAPHICS_INDEX_BUFFER_H

namespace nv {
namespace graphics {

class IndexBuffer
{
public:
    IndexBuffer(int indexCount, int *data, bool copy = true);
    ~IndexBuffer();

    int getIndexCount() const;
    const int *getData() const;

private:
    int _indexCount;
    int *_data;
};

inline int IndexBuffer::getIndexCount() const
{
    return _indexCount;
}

inline const int *IndexBuffer::getData() const
{
    return _data;
}

}
}

#endif
