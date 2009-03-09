#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct Vector2 {
	float x, y;
};

void storeInOpenDX(const char* filename, Vector2* vector, int width, int height, int depth)
{
	FILE* file = fopen(filename, "wb");
	
#if 0
        width = width >> 1;
        height = height >> 1;
#endif
	int count = width * height;
	int total_count = width * height * depth;
	fprintf(file, "object 1 class gridposition counts %d %d %d\n", width, height, depth);
	fprintf(file, "origin 0 0 0\n");
	fprintf(file, "delta 50 0 0\n");
	fprintf(file, "delta 0 50 0\n");
	fprintf(file, "delta 0 0 50\n");
	fprintf(file, "object 2 class gridconnections counts %d %d %d\n", width, height, depth);
	fprintf(file, "object 3 class array type double shape 3 rank 1 items %d data follows\n", total_count);
	

	for (int z = 0; z < depth; ++z)
	{
		for (int index = 0; index < count; ++index)
		{
			fprintf(file, "%f %f 0.0\n", vector[index].x, vector[index].y);
		}
	}

	fclose(file);
}

int main(int argc, char* argv[])
{
#if 1
	if (argc < 7)
	{
		printf("usage: mat2dx <Ix ascii filename> <Iy ascii filename> <width> <height> <depth> <outputfilename>\n");
		return 0;
	}
	FILE* xComp = fopen(argv[1], "rb");
	FILE* yComp = fopen(argv[2], "rb");
	int width, height, depth;
	width = atoi(argv[3]);
	height = atoi(argv[4]);
	depth = atoi(argv[5]);
#else
	int width = 610, height = 218;
	int depth = 50;
	FILE* xComp = fopen("Ix_data.txt", "rb");
	FILE* yComp = fopen("Iy_data.txt", "rb");
#endif
	Vector2* data = new Vector2[width * height];

	int index = 0;
	char buff[256];
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			fscanf(xComp, "%s", buff);
			if (strcmp(buff, "NaN") == 0)
			{
				data[index].x = 0;
			}
			else
			{
				data[index].x = atof(buff);
			}


			fscanf(xComp, "%s", buff);
			if (strcmp(buff, "NaN") == 0)
			{
				data[index].y = 0;
			}
			else
			{
				data[index].y = atof(buff);
			}

			++index;
		}

	}

	fclose(xComp);
	fclose(yComp);

	storeInOpenDX(argv[6], data, width, height, depth);
}
