/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_SHADER_H
#define NV_SHADER_H

#include <Cg/cg.h>
#include <Cg/cgGL.h>

typedef void NvCgCallbackFunction(void);

extern CGcontext g_context;

class NvShader
{
public:
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

    CGprogram getVP() const
    {
        return _cgVP;
    }

    CGprogram getFP() const
    {
        return _cgFP;
    }

    static void setErrorCallback(NvCgCallbackFunction callback);

protected:
    CGprogram _cgVP;
    CGprogram _cgFP;

    void resetPrograms();
};

#endif
