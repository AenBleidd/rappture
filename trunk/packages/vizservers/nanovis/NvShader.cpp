/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdio.h>

#include <GL/glew.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>

#include <R2/R2FilePath.h>

#include "NvShader.h"
#include "Trace.h"

CGcontext NvShader::_cgContext = NULL;

void NvShader::initCg()
{
    _cgContext = cgCreateContext();
}

void NvShader::exitCg()
{
    setErrorCallback(NULL);
    printErrorInfo();
    if (_cgContext != NULL) {
        cgDestroyContext(_cgContext);
        _cgContext = NULL;
    }
}

bool NvShader::printErrorInfo()
{
    CGerror lastError = cgGetError();

    if (lastError) {
        TRACE("Cg Error: %s\n", cgGetErrorString(lastError));
        if (getCgContext())
            TRACE("%s\n", cgGetLastListing(getCgContext()));
        return false;
    }
    return true;
}

CGprogram
LoadCgSourceProgram(CGcontext context, const char *fileName, CGprofile profile, 
                    const char *entryPoint)
{
    const char *path = R2FilePath::getInstance()->getPath(fileName);
    if (path == NULL) {
        ERROR("can't find program \"%s\"\n", fileName);
    }
    TRACE("cg program compiling: %s\n", path);
    CGprogram program;
    program = cgCreateProgramFromFile(context, CG_SOURCE, path, profile, 
                                      entryPoint, NULL);
    cgGLLoadProgram(program);
    CGerror LastError = cgGetError();
    if (LastError) {
        ERROR("Error message: %s\n", cgGetLastListing(context));
    }
    TRACE("successfully compiled program: %s\n", path);
    delete [] path;
    return program;
}

NvShader::NvShader():
    _cgVP(NULL),
    _cgFP(NULL)
{
}

NvShader::~NvShader()
{
    resetPrograms();
}

void NvShader::loadVertexProgram(const char *fileName, const char *entryPoint)
{
    resetPrograms();

    _cgVP = LoadCgSourceProgram(_cgContext, fileName, CG_PROFILE_VP40, entryPoint);
}

void NvShader::loadFragmentProgram(const char *fileName, const char *entryPoint)
{
    _cgFP = LoadCgSourceProgram(_cgContext, fileName, CG_PROFILE_FP40, entryPoint);
}

void NvShader::resetPrograms()
{
    if (_cgVP != NULL) {
        cgDestroyProgram(_cgVP);
    }

    if (_cgFP != NULL) {
        cgDestroyProgram(_cgFP);
    }
}

void NvShader::setErrorCallback(NvCgCallbackFunction callback)
{
    TRACE("NvShader error callback: %p\n", callback);
    cgSetErrorCallback(callback);
}
