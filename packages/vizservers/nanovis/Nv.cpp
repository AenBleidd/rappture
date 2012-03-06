/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
        ERROR("\n---------------------------------------------------\n");
        ERROR("%s\n\n", cgGetErrorString(lastError));
        ERROR("%s\n", listing);
        ERROR("-----------------------------------------------------\n");
        ERROR("Cg error, exiting...\n");
        cgDestroyContext(g_context);
        exit(-1);
    }
}

void NvInit(char* path)
{
    TRACE("Nanovis GL Initialized\n");
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

