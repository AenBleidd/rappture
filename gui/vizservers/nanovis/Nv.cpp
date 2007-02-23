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

extern void NvInitCG(); // in NvShader.cpp

NvParticleRenderer* g_particleRenderer;
NvColorTableRenderer* g_color_table_renderer;
VolumeRenderer* g_vol_render;
R2Fonts* g_fonts;

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

//init line integral convolution
void NvInitLIC()
{
/*
    g_lic = new Lic(NMESH, win_width, win_height, 0.3, g_context, volume[1]->id, 
        volume[1]->aspect_ratio_width,
        volume[1]->aspect_ratio_height,
        volume[1]->aspect_ratio_depth);
        */
}

void NvInitParticle()
{
/*
    //random placement on a slice
    float* data = new float[g_particleRenderer->psys_width * g_particleRenderer->psys_height * 4];
    bzero(data, sizeof(float)*4* g_particleRenderer->psys_width * g_particleRenderer->psys_height);

    int index;
    bool particle;
    for (int i=0; i<g_particleRenderer->psys_width; i++) { 
        for (int j=0; j<g_particleRenderer->psys_height; j++) { 
            index = i + g_particleRenderer->psys_height*j;
            particle = rand() % 256 > 150; 
            if(particle) 
            {
                //assign any location (x,y,z) in range [0,1]
                data[4*index] = lic_slice_x;
                data[4*index+1]= j/float(g_particleRenderer->psys_height);
                data[4*index+2]= i/float(g_particleRenderer->psys_width);
                data[4*index+3]= 30; //shorter life span, quicker iterations	
            }
            else
            {
                data[4*index] = 0;
                data[4*index+1]= 0;
                data[4*index+2]= 0;
                data[4*index+3]= 0;	
            }
        }
    }

    g_particleRenderer->initialize((Particle*)data);
    delete[] data;
    */
}

//init the particle system using vector field volume->[1]
void NvInitParticleRenderer()
{
/*
    g_particleRenderer = new NvParticleRenderer(NMESH, NMESH, g_context, 
        volume[1]->id,
        1./volume[1]->aspect_ratio_width,
        1./volume[1]->aspect_ratio_height,
        1./volume[1]->aspect_ratio_depth);

    NvInitParticle();
*/
}

void NvInitVolumeRenderer()
{
}

void NvInit()
{
    char* filepath = "/home/nanohub/vrinside/rappture/gui/vizservers/nanovis/shaders:" \
                    "/home/nanohub/vrinside/rappture/gui/vizservers/nanovis/resources:" \
                    "/opt/nanovis/lib/shaders:/opt/nanovis/lib/resources";

    R2FilePath::getInstance()->setPath(filepath);
    NvPrintSystemInfo();
    NvInitGLEW();
    NvInitCG();

    g_fonts = new R2Fonts();
    g_fonts->addFont("verdana", "verdana.fnt");
    g_fonts->setFont("verdana");

    g_color_table_renderer = new NvColorTableRenderer();
    g_color_table_renderer->setFonts(g_fonts);

    NvInitVolumeRenderer();
    NvInitParticleRenderer();
   
    printf("Nanovis GL Initialized\n");
}

void NvExit()
{
    if (g_particleRenderer)
    {
        delete g_particleRenderer;
        g_particleRenderer;
    }

#ifdef EVENTLOG
    NvExitEventLog();
#endif

#ifdef XINETD
    NvExitService();
#endif

}

