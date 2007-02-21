#include "Vector3.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

extern void load_volqd_volume(int index, int width, int height, int depth,
    int n_component, float* data, double vmin, double vmax, Vector3& cellSize);

extern void load_zinc_volume(int index, int width, int height, int depth,
    int n_component, float* data, double vmin, double vmax, Vector3& cellSize);

void NvLoadVolumeBinFile(int index, char* filename)
{
	//read file
	FILE* file = fopen(filename, "rb");
	assert(file!=NULL);

	int width, height, depth, n_components;
	unsigned int type;
	unsigned int interp;

	fread(&width, sizeof(int), 1, file);
	fread(&height, sizeof(int), 1, file);
	fread(&depth, sizeof(int), 1, file);
	fread(&n_components, sizeof(int), 1, file);
	fread(&type, sizeof(unsigned int), 1, file);
	fread(&interp, sizeof(unsigned int), 1, file);

    int size = n_components*width*depth*height;

	float* data;
	data = new float[size];

    fread(data, sizeof(float) * size, 1, file);
    fclose(file);

    float min = data[0], max = data[0];
    float non_zero_min = -1e27f;
	for (int i=0; i<n_components*width*depth*height; i++)
    {
		if(data[i]>max)
			max = data[i];
		if(data[i]<min)
			min = data[i];

		if(data[i]<non_zero_min && data[i] != 0)
        {
			non_zero_min = data[i];
		}
	}
	
 
    int i = 0;
    if (max != 0.0f)
    {
        for (i=0; i < size; ++i) {
            data[i] = data[i] / max;
        }
    }

    Vector3 cellsize;
    cellsize.x = width * 0.25;
    cellsize.y = height * 0.25;
    cellsize.z = depth * 0.25;

    load_volqd_volume(index, width, height, depth, 4, data, min, max, cellsize);

    delete [] data;
}

void NvLoadVolumeBinFile2(int index, char* filename)
{
	//read file
	FILE* file = fopen(filename, "rb");
	assert(file!=NULL);

	int width, height, depth, n_components;
	unsigned int type;
	unsigned int interp;

	fread(&width, sizeof(int), 1, file);
	fread(&height, sizeof(int), 1, file);
	fread(&depth, sizeof(int), 1, file);
	fread(&n_components, sizeof(int), 1, file);
	fread(&type, sizeof(unsigned int), 1, file);
	fread(&interp, sizeof(unsigned int), 1, file);

    int size = n_components*width*depth*height;

	float* data;
	data = new float[size];

    fread(data, sizeof(float) * size, 1, file);
    fclose(file);

    float min = data[0], max = data[0];
    float non_zero_min = -1e27f;
	for (int i=0; i<n_components*width*depth*height; i++)
    {
		if(data[i]>max)
			max = data[i];
		if(data[i]<min)
			min = data[i];

		if(data[i]<non_zero_min && data[i] != 0)
        {
			non_zero_min = data[i];
		}
	}
	
 
    int i = 0;
    if (max != 0.0f)
    {
        for (i=0; i < size; ++i) {
            data[i] = data[i] / max;
        }
    }

    Vector3 cellsize;
    cellsize.x = width * 0.25;
    cellsize.y = height * 0.25;
    cellsize.z = depth * 0.25;

    load_zinc_volume(index, width, height, depth, 4, data, min, max, cellsize);

    delete [] data;
}
