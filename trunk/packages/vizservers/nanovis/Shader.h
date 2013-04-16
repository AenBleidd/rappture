/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef NV_SHADER_H
#define NV_SHADER_H

#include <string>
#include <tr1/unordered_map>

#include <GL/glew.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>

#include "Trace.h"

namespace nv {

class Shader
{
public:
    enum GLMatrix {
        MODELVIEW_MATRIX = CG_GL_MODELVIEW_MATRIX,
        PROJECTION_MATRIX = CG_GL_PROJECTION_MATRIX,
        MODELVIEW_PROJECTION_MATRIX = CG_GL_MODELVIEW_PROJECTION_MATRIX
    };
    enum GLMatrixType {
        MATRIX_IDENTITY = CG_GL_MATRIX_IDENTITY,
        MATRIX_INVERSE = CG_GL_MATRIX_INVERSE
    };

    typedef void (*ShaderCallbackFunction)(void);

    Shader();

    virtual ~Shader();

    /**
     * \brief Load and compile a vertex shader
     * \param fileName the name of the shader source file
     */
    void loadVertexProgram(const char *fileName);

    /**
     * \brief Load and compile a fragment shader
     * \param fileName the name of the shader source file
     */
    void loadFragmentProgram(const char *fileName);

    void setVPParameter1f(const char *name, float val)
    {
        Parameter param = getVPParam(name);
        if (param == NULL)
            return;
        cgSetParameter1f(param, val);
    }

    void setFPParameter1f(const char *name, float val)
    {
        Parameter param = getFPParam(name);
        if (param == NULL)
            return;
        cgSetParameter1f(param, val);
    }

    void setVPParameter2f(const char *name, float val1, float val2)
    {
        Parameter param = getVPParam(name);
        if (param == NULL)
            return;
        cgSetParameter2f(param, val1, val2);
    }

    void setFPParameter2f(const char *name, float val1, float val2)
    {
        Parameter param = getFPParam(name);
        if (param == NULL)
            return;
        cgSetParameter2f(param, val1, val2);
    }

    void setVPParameter3f(const char *name, float val1, float val2, float val3)
    {
        Parameter param = getVPParam(name);
        if (param == NULL)
            return;
        cgSetParameter3f(param, val1, val2, val3);
    }

    void setFPParameter3f(const char *name, float val1, float val2, float val3)
    {
        Parameter param = getFPParam(name);
        if (param == NULL)
            return;
        cgSetParameter3f(param, val1, val2, val3);
    }

    void setVPParameter4f(const char *name, float val1, float val2, float val3, float val4)
    {
        Parameter param = getVPParam(name);
        if (param == NULL)
            return;
        cgSetParameter4f(param, val1, val2, val3, val4);
    }

    void setFPParameter4f(const char *name, float val1, float val2, float val3, float val4)
    {
        Parameter param = getFPParam(name);
        if (param == NULL)
            return;
        cgSetParameter4f(param, val1, val2, val3, val4);
    }

    void setVPMatrixParameterf(const char *name, float *mat)
    {
        Parameter param = getVPParam(name);
        if (param == NULL)
            return;
        cgSetMatrixParameterfc(param, mat);
    }

    void setFPMatrixParameterf(const char *name, float *mat)
    {
        Parameter param = getFPParam(name);
        if (param == NULL)
            return;
        cgSetMatrixParameterfc(param, mat);
    }

    void setVPTextureParameter(const char *name, GLuint texobj, bool enable = true)
    {
        Parameter param = getVPParam(name);
        if (param == NULL)
            return;
        cgGLSetTextureParameter(param, texobj);
        if (enable)
            cgGLEnableTextureParameter(param);
    }

    void setFPTextureParameter(const char *name, GLuint texobj, bool enable = true)
    {
        Parameter param = getFPParam(name);
        if (param == NULL)
            return;
        cgGLSetTextureParameter(param, texobj);
        if (enable)
            cgGLEnableTextureParameter(param);
    }

    void enableVPTextureParameter(const char *name)
    {
        Parameter param = getVPParam(name);
        if (param == NULL)
            return;
        cgGLEnableTextureParameter(param);
    }

    void enaableFPTextureParameter(const char *name)
    {
        Parameter param = getFPParam(name);
        if (param == NULL)
            return;
        cgGLEnableTextureParameter(param);
    }

    void disableVPTextureParameter(const char *name)
    {
        Parameter param = getVPParam(name);
        if (param == NULL)
            return;
        cgGLDisableTextureParameter(param);
    }

    void disableFPTextureParameter(const char *name)
    {
        Parameter param = getFPParam(name);
        if (param == NULL)
            return;
        cgGLDisableTextureParameter(param);
    }

    void setGLStateMatrixVPParameter(const char *name, GLMatrix matrix,
                                     GLMatrixType type = MATRIX_IDENTITY)
    {
        Parameter param = getVPParam(name);
        if (param == NULL)
            return;
        cgGLSetStateMatrixParameter(param, (CGGLenum)matrix, (CGGLenum)type);
    }

    void setGLStateMatrixFPParameter(const char *name, GLMatrix matrix,
                                     GLMatrixType type = MATRIX_IDENTITY)
    {
        Parameter param = getFPParam(name);
        if (param == NULL)
            return;
        cgGLSetStateMatrixParameter(param, (CGGLenum)matrix, (CGGLenum)type);
    }

    virtual void bind()
    {
        if (_cgVP) {
            cgGLBindProgram(_cgVP);
            enableVertexProfile();
        }
        if (_cgFP) {
            cgGLBindProgram(_cgFP);
            enableFragmentProfile();
        }
    }

    virtual void unbind()
    {
        if (_cgVP)
            disableVertexProfile();
        if (_cgFP)
            disableFragmentProfile();
    }

    static void init();

    static void exit();

    static bool printErrorInfo();

    static void setErrorCallback(ShaderCallbackFunction callback);

private:
    typedef CGparameter Parameter;
    typedef std::tr1::unordered_map<std::string, Parameter> ParameterHashmap;

    Parameter getNamedParameterFromFP(const char *paramName)
    {
        if (_cgFP) {
            return cgGetNamedParameter(_cgFP, paramName);
        }
        ERROR("Unknown fragment program parameter: %s", paramName);
        return 0;
    }

    Parameter getNamedParameterFromVP(const char *paramName)
    {
        if (_cgVP) {
            return cgGetNamedParameter(_cgVP, paramName);
        }
        ERROR("Unknown vertex program parameter: %s", paramName);
        return 0;
    }

    Parameter getVPParam(const char *name)
    {
        Parameter param;
        ParameterHashmap::iterator itr = _vpParams.find(name);
        if (itr == _vpParams.end()) {
            param = getNamedParameterFromVP(name);
            if (param != NULL)
                _vpParams[name] = param;
            else
                ERROR("Unknown vertex program parameter: %s", name);
        } else {
            param = itr->second;
        }
        return param;
    }

    Parameter getFPParam(const char *name)
    {
        Parameter param;
        ParameterHashmap::iterator itr = _fpParams.find(name);
        if (itr == _fpParams.end()) {
            param = getNamedParameterFromFP(name);
            if (param != NULL)
                _fpParams[name] = param;
            else
                ERROR("Unknown fragment program parameter: %s", name);
        } else {
            param = itr->second;
        }
        return param;
    }


    void resetPrograms();


    CGprogram getVP()
    {
        return _cgVP;
    }

    CGprogram getFP()
    {
        return _cgFP;
    }

    void enableVertexProfile()
    {
        cgGLEnableProfile(_vertexProfile);
    }

    void disableVertexProfile()
    {
        cgGLDisableProfile(_vertexProfile);
    }

    void enableFragmentProfile()
    {
        cgGLEnableProfile(_fragmentProfile);
    }

    void disableFragmentProfile()
    {
        cgGLDisableProfile(_fragmentProfile);
    }

    static CGcontext getCgContext()
    {
        return _cgContext;
    }

    std::string _vpFile;
    std::string _fpFile;

    ParameterHashmap _vpParams;
    ParameterHashmap _fpParams;

    CGprofile _vertexProfile;
    CGprofile _fragmentProfile;

    CGprogram _cgVP;
    CGprogram _cgFP;

    static CGprofile _defaultVertexProfile;
    static CGprofile _defaultFragmentProfile;
    static CGcontext _cgContext;

    static CGprogram
    loadCgSourceProgram(CGcontext context, const char *filename, 
                        CGprofile profile, const char *entryPoint);
};

}

#endif
