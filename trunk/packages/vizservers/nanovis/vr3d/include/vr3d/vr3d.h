/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#pragma once

#ifdef WIN32
#pragma pointers_to_members(full_generality, single_inheritance)
#endif

#include <list>

#ifndef IMAC
#include <GL/glew.h>
#endif 

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef IMAC
#include <GL/gl.h>
#else
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#endif

#ifdef _WIN32
//#include <windows.h>

#ifdef VR3DDLLEXPORT
#	define Vr3DExport __declspec(dllexport)
#   define EXPIMP_TEMPLATE
#else
#	define Vr3DExport __declspec(dllimport)
#   define EXPIMP_TEMPLATE extern
#endif 

#else
#	define Vr3DExport
#endif 


//#include <vr3d/vr3dClassIDs.h>

#ifdef __cplusplus
extern "C" {
#endif

void Vr3DExport vrInitializeTimeStamp();

float Vr3DExport vrGetTimeStamp();

void Vr3DExport vrUpdateTimeStamp();


void Vr3DExport vrInit();

void Vr3DExport vrExit();

#ifdef __cplusplus

class vrTransform;
typedef std::list<vrTransform*> vrPath;
}
#endif
