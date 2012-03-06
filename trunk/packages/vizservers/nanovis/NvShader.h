/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __NV_SHADER_H__
#define __NV_SHADER_H__

#include <Cg/cg.h>
#include <Cg/cgGL.h>

typedef void NvCgCallbackFunction(void);

class NvShader {
protected :
    CGprogram _cgVP;
    CGprogram _cgFP;

public :
    NvShader();

public :
    virtual ~NvShader();

protected :
    void resetPrograms();

public :
    static void setErrorCallback(NvCgCallbackFunction callback);
public :
    /**
     * @brief create a Cg vertex program and load it
     * @param fileName the name of Cg program file
     * @param entryPoint a entry point of the Cg program
     */
    void loadVertexProgram(const char* fileName, const char* entryPoint);

    /**
     * @brief create a Cg fragment program and load it
     * @param fileName the name of Cg program file
     * @param entryPoint a entry point of the Cg program
     */
    void loadFragmentProgram(const char* fileName, const char* entryPoint);
    
    CGparameter getNamedParameterFromFP(const char* paramName);
    CGparameter getNamedParameterFromVP(const char* paramName);

    CGprogram getVP() const;
    CGprogram getFP() const;
};

extern CGcontext g_context;

inline CGparameter NvShader::getNamedParameterFromFP(const char* paramName)
{
    if (_cgFP)
    {
        return cgGetNamedParameter(_cgFP, paramName);
    }

    return 0;
}

inline CGparameter NvShader::getNamedParameterFromVP(const char* paramName)
{
    if (_cgVP)
    {
        return cgGetNamedParameter(_cgVP, paramName);
    }

    return 0;
}

inline CGprogram NvShader::getVP() const
{
    return _cgVP;
}

inline CGprogram NvShader::getFP() const
{
    return _cgFP;
}

#endif // 

