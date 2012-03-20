/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef R2_INDEX_BUFFER_H
#define R2_INDEX_BUFFER_H

class R2IndexBuffer
{
public:
    R2IndexBuffer(int indexCount, int *data, bool copy = true);
    ~R2IndexBuffer();

    int getIndexCount() const;
    const int *getData() const;

private:
    int _indexCount;
    int *_data;
};

inline int R2IndexBuffer::getIndexCount() const
{
    return _indexCount;
}

inline const int* R2IndexBuffer::getData() const
{
    return _data;
}

#endif
