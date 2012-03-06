/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <vr3d/vr3d.h>
#ifndef OPENGLES
#include <GL/glew.h>
#endif
#include <stdio.h>
#ifndef WIN32
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#endif




#ifdef USE_SOUND

/*
#pragma pack(1)
typedef struct {
uint32_t Chunk_ID;
uint32_t ChunkSize;
...
uint16_t AudioFormat;
...
} WAV_header_t;
#pragma pack()
*/



int is_big_endian(void) {
unsigned char test[4] = { 0x12, 0x34, 0x56, 0x78 };

return *((uint32_t *)test) == 0x12345678;
}


#include <al.h>
#include <alc.h>
// TBD..
#include <alut.h>
#endif


float _currentTime = 0;
float _startTime = 0;

void vrInitializeTimeStamp()
{
#ifdef WIN32
	_startTime = ::timeGetTime() * 0.001f;
#else
	_startTime = (float) clock() / (float) CLOCKS_PER_SEC;
#endif
}

float vrGetTimeStamp()
{
	//return (_currentTime - _startTime) * 10;
	return (_currentTime - _startTime);
}

void vrUpdateTimeStamp()
{
#ifdef WIN32
	_currentTime = ::timeGetTime() * 0.001f;
#else
	_currentTime = (float) clock() / (float) CLOCKS_PER_SEC;
#endif
}

int initOpenAL(void);
void vrInit()
{
#ifndef OPENGLES
	::glewInit();
#endif
/*
	if (! glewIsSupported( "GL_EXT_framebuffer_object GL_NV_depth_buffer_float")) {
        fprintf( stderr, "Error: This program requires the following capabilities\n");
        fprintf( stderr, "  OpenGL version 2.0 or later\n");
        fprintf( stderr, "  Support for the GL_EXT_framebuffer_object extension\n");
        fprintf( stderr, "  Support for the GL_NV_depth_buffer_float extension\n\n");
        exit(-1);
    }
	*/

	initOpenAL();
	
}

int initOpenAL(void)
{
#ifdef USE_SOUND
	alutInit(0, 0);
	//glGetError();
	/*
	ALCcontext *context;
	ALCdevice *device;
	const ALCchar *default_device;

	default_device = alcGetString(NULL,
	ALC_DEFAULT_DEVICE_SPECIFIER);

	printf("using default device: %s\n", default_device);

	if ((device = alcOpenDevice(default_device)) == NULL) {
		fprintf(stderr, "failed to open sound device\n");
		return -1;
	}
	context = alcCreateContext(device, NULL);
	alcMakeCurrentContext(context);
	alcProcessContext(context);

	atexit(atexit_openal);

	// allocate buffers and sources here using alGenBuffers() and alGenSources() 

	alGetError(); 
	*/

	// Set Listener attributes
        // Position ... 
	int error;
        ALfloat listenerPos[]={0.0,0.0,0.0}; // At the origin
        ALfloat listenerVel[]={0.0,0.0,0.0}; // The velocity (no doppler here)
        ALfloat listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0}; // LookAt then Up
        alListenerfv(AL_POSITION,listenerPos); 
        if ((error = alGetError()) != AL_NO_ERROR) 
        { 
                //DisplayOpenALError("alListenerfv :", error); 
                printf("alListenerfv : %d\n", error); 
                return 0;
        }
        // Velocity ... 
        alListenerfv(AL_VELOCITY,listenerVel); 
        if ((error = alGetError()) != AL_NO_ERROR) 
        { 
                //DisplayOpenALError("alListenerfv :", error); 
                printf("alListenerfv :%d\n", error); 
                return 0;
        }
        // Orientation ... 
        alListenerfv(AL_ORIENTATION,listenerOri); 
        if ((error = alGetError()) != AL_NO_ERROR) 
        { 
                //DisplayOpenALError("alListenerfv :", error); 
                printf("alListenerfv :%d\n", error); 
                return 0;
        }
#endif

	return 0;
}


void vrExit()
{
#ifdef USE_SOUND
	alutExit();
#endif
}
