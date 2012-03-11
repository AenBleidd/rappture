/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef VRCOLORBREWER_H
#define VRCOLORBREWER_H

#include <string>

#include <vrutil/vrUtil.h>

class CBColor
{
public :
    float r, g, b;
};

class vrColorBrewer
{
public:
    vrColorBrewer();

    vrColorBrewer(int size, char label[20]);

    ~vrColorBrewer();

    void setColor(int index, float r, float g, float b);

    CBColor *colorScheme;
    float* colorKey;
    float* defaultOpacity;
    int size;
    std::string label;
};

#endif

