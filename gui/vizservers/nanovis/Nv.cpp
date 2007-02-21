#include "Nv.h"
#include "NvShader.h"
#include <R2/R2FilePath.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

extern void NvInitCG(); // in NvShader.cpp

//query opengl information
static void NvPrintSystemInfo()
{
    fprintf(stderr, "-----------------------------------------------------------\n");
    fprintf(stderr, "OpenGL driver: %s %s\n", glGetString(GL_VENDOR), glGetString(GL_VERSION));
    fprintf(stderr, "Graphics hardware: %s\n", glGetString(GL_RENDERER));
    fprintf(stderr, "-----------------------------------------------------------\n");
}

void NvCgErrorCallback(void)
{
    CGerror lastError = cgGetError();
    if(lastError) {
        const char *listing = cgGetLastListing(g_context);
        printf("\n---------------------------------------------------\n");
        printf("%s\n\n", cgGetErrorString(lastError));
        printf("%s\n", listing);
        printf("-----------------------------------------------------\n");
        printf("Cg error, exiting...\n");
        cgDestroyContext(g_context);
        exit(-1);
    }
}


void NvInitGLEW()
{
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        //glew init failed, exit.
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        getchar();
        assert(false);
    }
    fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
}

void NvInit()
{
    R2FilePath::getInstance()->setPath(".:./resources:/home/nanohub/vrinside/rappture/gui/vizservers/nanovis/resources");
    NvPrintSystemInfo();
    NvInitGLEW();
    NvInitCG();
}

void NvExit()
{
}
