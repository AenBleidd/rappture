/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#pragma once

#ifdef WIN32
#pragma pointers_to_members(full_generality, single_inheritance)
#endif

#ifdef _WIN32
//#include <windows.h>

#ifdef VRUTILDLLEXPORT
#	define VrUtilExport __declspec(dllexport)
#else
#	define VrUtilExport __declspec(dllimport)
#endif 

#else
#	define VrUtilExport
#endif 

