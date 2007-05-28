#ifndef __NV_SHADER_H__
#define __NV_SHADER_H__

#include <Cg/cg.h>
#include <Cg/cgGL.h>

class NvShader {
protected :
    CGprogram _cgVP;
    CGprogram _cgFP;

protected :
    NvShader();

public :
    virtual ~NvShader();
};

extern CGcontext g_context;

#endif // 

