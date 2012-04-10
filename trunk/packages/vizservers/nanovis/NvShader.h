/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_SHADER_H
#define NV_SHADER_H

#include <string>
#include <tr1/unordered_map>

#include <GL/glew.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>

#include "Trace.h"

extern CGprogram
LoadCgSourceProgram(CGcontext context, const char *fileName,
                    CGprofile profile, const char *entryPoint);

class NvShader
{
public:
    enum NvGLMatrix {
        MODELVIEW_MATRIX = CG_GL_MODELVIEW_MATRIX,
        PROJECTION_MATRIX = CG_GL_PROJECTION_MATRIX,
        MODELVIEW_PROJECTION_MATRIX = CG_GL_MODELVIEW_PROJECTION_MATRIX
    };
    enum NvGLMatrixType {
        MATRIX_IDENTITY = CG_GL_MATRIX_IDENTITY,
        MATRIX_INVERSE = CG_GL_MATRIX_INVERSE
    };

    typedef void NvCgCallbackFunction(void);

    NvShader();

    virtual ~NvShader();

    /**
     * @brief create a Cg vertex program and load it
     * @param fileName the name of Cg program file
     * @param entryPoint a entry point of the Cg program
     */
    void loadVertexProgram(const char *fileName, const char *entryPoint);

    /**
     * @brief create a Cg fragment program and load it
     * @param fileName the name of Cg program file
     * @param entryPoint a entry point of the Cg program
     */
    void loadFragmentProgram(const char *fileName, const char *entryPoint);

    CGparameter getNamedParameterFromFP(const char *paramName)
    {
        if (_cgFP) {
            return cgGetNamedParameter(_cgFP, paramName);
        }
        ERROR("Unknown fragment program parameter: %s\n", paramName);
        return 0;
    }

    CGparameter getNamedParameterFromVP(const char *paramName)
    {
        if (_cgVP) {
            return cgGetNamedParameter(_cgVP, paramName);
        }
        ERROR("Unknown vertex program parameter: %s\n", paramName);
        return 0;
    }

    void setVPParameter1f(const char *name, float val)
    {
        CGparameter param = getVPParam(name);
        if (param == NULL)
            return;
        cgSetParameter1f(param, val);
    }

    void setFPParameter1f(const char *name, float val)
    {
        CGparameter param = getFPParam(name);
        if (param == NULL)
            return;
        cgSetParameter1f(param, val);
    }

    void setVPParameter2f(const char *name, float val1, float val2)
    {
        CGparameter param = getVPParam(name);
        if (param == NULL)
            return;
        cgSetParameter2f(param, val1, val2);
    }

    void setFPParameter2f(const char *name, float val1, float val2)
    {
        CGparameter param = getFPParam(name);
        if (param == NULL)
            return;
        cgSetParameter2f(param, val1, val2);
    }

    void setVPParameter3f(const char *name, float val1, float val2, float val3)
    {
        CGparameter param = getVPParam(name);
        if (param == NULL)
            return;
        cgSetParameter3f(param, val1, val2, val3);
    }

    void setFPParameter3f(const char *name, float val1, float val2, float val3)
    {
        CGparameter param = getFPParam(name);
        if (param == NULL)
            return;
        cgSetParameter3f(param, val1, val2, val3);
    }

    void setVPParameter4f(const char *name, float val1, float val2, float val3, float val4)
    {
        CGparameter param = getVPParam(name);
        if (param == NULL)
            return;
        cgSetParameter4f(param, val1, val2, val3, val4);
    }

    void setFPParameter4f(const char *name, float val1, float val2, float val3, float val4)
    {
        CGparameter param = getFPParam(name);
        if (param == NULL)
            return;
        cgSetParameter4f(param, val1, val2, val3, val4);
    }

    void setVPTextureParameter(const char *name, GLuint texobj, bool enable = true)
    {
        CGparameter param = getVPParam(name);
        if (param == NULL)
            return;
        cgGLSetTextureParameter(param, texobj);
        if (enable)
            cgGLEnableTextureParameter(param);
    }

    void setFPTextureParameter(const char *name, GLuint texobj, bool enable = true)
    {
        CGparameter param = getFPParam(name);
        if (param == NULL)
            return;
        ParameterHashmap::iterator itr = _fpParams.find(name);
        cgGLSetTextureParameter(param, texobj);
        if (enable)
            cgGLEnableTextureParameter(param);
    }

    void enableVPTextureParameter(const char *name)
    {
        CGparameter param = getVPParam(name);
        if (param == NULL)
            return;
        cgGLEnableTextureParameter(param);
    }

    void enaableFPTextureParameter(const char *name)
    {
        CGparameter param = getFPParam(name);
        if (param == NULL)
            return;
        cgGLEnableTextureParameter(param);
    }

    void disableVPTextureParameter(const char *name)
    {
        CGparameter param = getVPParam(name);
        if (param == NULL)
            return;
        cgGLDisableTextureParameter(param);
    }

    void disableFPTextureParameter(const char *name)
    {
        CGparameter param = getFPParam(name);
        if (param == NULL)
            return;
        cgGLDisableTextureParameter(param);
    }

    void setGLStateMatrixVPParameter(const char *name, NvGLMatrix matrix, NvGLMatrixType type)
    {
        CGparameter param = getVPParam(name);
        if (param == NULL)
            return;
        cgGLSetStateMatrixParameter(param, (CGGLenum)matrix, (CGGLenum)type);
    }

    void setGLStateMatrixFPParameter(const char *name, NvGLMatrix matrix, NvGLMatrixType type)
    {
        CGparameter param = getFPParam(name);
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

    static void initCg(CGprofile defaultVertexProfile = CG_PROFILE_VP40,
                       CGprofile defaultFragmentProfile = CG_PROFILE_FP40);

    static void exitCg();

    static bool printErrorInfo();

    static void setErrorCallback(NvCgCallbackFunction callback);

    static CGcontext getCgContext()
    {
        return _cgContext;
    }

    static CGprogram
    loadCgSourceProgram(CGcontext context, const char *filename, 
                        CGprofile profile, const char *entryPoint);

protected:
    typedef std::tr1::unordered_map<std::string, CGparameter> ParameterHashmap;

    CGprogram getVP()
    {
        return _cgVP;
    }

    CGprogram getFP()
    {
        return _cgFP;
    }

    CGparameter getVPParam(const char *name)
    {
        CGparameter param;
        ParameterHashmap::iterator itr = _vpParams.find(name);
        if (itr == _vpParams.end()) {
            param = getNamedParameterFromVP(name);
            if (param != NULL)
                _vpParams[name] = param;
            else
                ERROR("Unknown vertex program parameter: %s\n", name);
        } else {
            param = itr->second;
        }
        return param;
    }

    CGparameter getFPParam(const char *name)
    {
        CGparameter param;
        ParameterHashmap::iterator itr = _fpParams.find(name);
        if (itr == _fpParams.end()) {
            param = getNamedParameterFromFP(name);
            if (param != NULL)
                _fpParams[name] = param;
            else
                ERROR("Unknown fragment program parameter: %s\n", name);
        } else {
            param = itr->second;
        }
        return param;
    }

    void resetPrograms();

    CGprofile _vertexProfile;
    CGprofile _fragmentProfile;
    std::string _vpFile;
    CGprogram _cgVP;
    std::string _fpFile;
    CGprogram _cgFP;
    ParameterHashmap _vpParams;
    ParameterHashmap _fpParams;

    static CGprofile _defaultVertexProfile;
    static CGprofile _defaultFragmentProfile;
    static CGcontext _cgContext;
};

#endif
