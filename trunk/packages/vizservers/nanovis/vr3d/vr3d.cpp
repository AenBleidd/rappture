/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>

#include <vr3d/vr3d.h>

#include <GL/glew.h>

float _currentTime = 0;
float _startTime = 0;

void vrInitializeTimeStamp()
{
    _startTime = (float)clock() / (float)CLOCKS_PER_SEC;
}

float vrGetTimeStamp()
{
    return (_currentTime - _startTime);
}

void vrUpdateTimeStamp()
{
    _currentTime = (float)clock() / (float)CLOCKS_PER_SEC;
}

void vrInit()
{
    ::glewInit();
}

void vrExit()
{
}
