/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include <stdio.h>

#include <GL/glew.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>

#include <util/FilePath.h>

#include "NvShader.h"
#include "Trace.h"

using namespace nv::util;

CGprofile NvShader::_defaultVertexProfile = CG_PROFILE_VP40;
CGprofile NvShader::_defaultFragmentProfile = CG_PROFILE_FP40;
CGcontext NvShader::_cgContext = NULL;

void NvShader::initCg(CGprofile defaultVertexProfile,
                      CGprofile defaultFragmentProfile)
{
    _defaultVertexProfile = defaultVertexProfile;
    _defaultFragmentProfile = defaultFragmentProfile;
    _cgContext = cgCreateContext();
}

void NvShader::exitCg()
{
    setErrorCallback(NULL);
    printErrorInfo();
    if (_cgContext != NULL) {
        TRACE("Before DestroyContext");
        cgDestroyContext(_cgContext);
        TRACE("After DestroyContext");
        _cgContext = NULL;
    }
}

bool NvShader::printErrorInfo()
{
    CGerror lastError = cgGetError();

    if (lastError) {
        TRACE("Cg Error: %s", cgGetErrorString(lastError));
        if (getCgContext())
            TRACE("%s", cgGetLastListing(getCgContext()));
        return false;
    }
    return true;
}

CGprogram
NvShader::loadCgSourceProgram(CGcontext context, const char *fileName,
                              CGprofile profile, const char *entryPoint)
{
    std::string path = FilePath::getInstance()->getPath(fileName);
    if (path.empty()) {
        ERROR("can't find program \"%s\"", fileName);
    }
    TRACE("cg program compiling: %s", path.c_str());
    CGprogram program;
    program = cgCreateProgramFromFile(context, CG_SOURCE, path.c_str(), profile, 
                                      entryPoint, NULL);
    cgGLLoadProgram(program);
    CGerror LastError = cgGetError();
    if (LastError) {
        ERROR("Error message: %s", cgGetLastListing(context));
    }
    TRACE("successfully compiled program: %s", path.c_str());
    return program;
}

NvShader::NvShader():
    _vertexProfile(_defaultVertexProfile),
    _fragmentProfile(_defaultFragmentProfile),
    _cgVP(NULL),
    _cgFP(NULL)
{
}

NvShader::~NvShader()
{
    TRACE("In ~NvShader");
    if (_cgContext == NULL) {
        TRACE("Lost Cg context: vp: %s, fp: %s", _vpFile.c_str(), _fpFile.c_str());
    } else {
        resetPrograms();
    }
}

void NvShader::loadVertexProgram(const char *fileName, const char *entryPoint)
{
    if (_cgVP != NULL) {
        cgDestroyProgram(_cgVP);
    }
    _cgVP = loadCgSourceProgram(_cgContext, fileName,
                                _vertexProfile, entryPoint);
    _vpFile = fileName;
}

void NvShader::loadFragmentProgram(const char *fileName, const char *entryPoint)
{
    if (_cgFP != NULL) {
        cgDestroyProgram(_cgFP);
    }
    _cgFP = loadCgSourceProgram(_cgContext, fileName,
                                _fragmentProfile, entryPoint);
    _fpFile = fileName;
}

void NvShader::resetPrograms()
{
    if (_cgVP != NULL) {
        TRACE("Destroying vertex program: %s", _vpFile.c_str());
        cgDestroyProgram(_cgVP);
    }

    if (_cgFP != NULL) {
        TRACE("Destroying fragment program: %s", _fpFile.c_str());
        cgDestroyProgram(_cgFP);
    }
}

void NvShader::setErrorCallback(NvCgCallbackFunction callback)
{
    TRACE("NvShader setting error callback to: %p", callback);
    cgSetErrorCallback(callback);
}
