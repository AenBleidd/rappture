/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_SHADER_H
#define NV_SHADER_H

#include <GL/glew.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>

#include "Trace.h"

extern CGprogram
LoadCgSourceProgram(CGcontext context, const char *fileName,
                    CGprofile profile, const char *entryPoint)

class NvShader
{
public:
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
        return 0;
    }

    CGparameter getNamedParameterFromVP(const char *paramName)
    {
        if (_cgVP) {
            return cgGetNamedParameter(_cgVP, paramName);
        }
        return 0;
    }

    void setTextureParameter(CGparameter param, GLuint texobj)
    {
        cgGLSetTextureParameter(param, texobj);
    }

    CGprogram getVP() const
    {
        return _cgVP;
    }

    CGprogram getFP() const
    {
        return _cgFP;
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

protected:
    void resetPrograms();

    CGprofile _vertexProfile;
    CGprofile _fragmentProfile;
    CGprogram _cgVP;
    CGprogram _cgFP;

    static CGprofile _defaultVertexProfile;
    static CGprofile _defaultFragmentProfile;
    static CGcontext _cgContext;

private:
    static CGprogram
    loadCgSourceProgram(CGcontext context, const char *filename, 
                        CGprofile profile, const char *entryPoint);
};

#endif
