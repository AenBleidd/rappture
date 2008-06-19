#include "Nv.h"
#include "NvShader.h"
#include "NvParticleRenderer.h"
#include "NvColorTableRenderer.h"
#include "NvEventLog.h"
#include "VolumeRenderer.h"
#include <R2/R2FilePath.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <BMPImageLoaderImpl.h>
#include <ImageLoaderFactory.h>
#include "PointSetRenderer.h"

extern void NvInitCG(); // in NvShader.cpp

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
        fflush(stdout);
        exit(-1);
    }
}

void NvInit(char* path)
{
    printf("Nanovis GL Initialized\n");
}

void NvExit()
{
#ifdef EVENTLOG
    NvExitEventLog();
#endif

#ifdef XINETD
    NvExitService();
#endif

}

