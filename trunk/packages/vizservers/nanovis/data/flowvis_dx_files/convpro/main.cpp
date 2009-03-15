#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct Vector2 {
    double x, y;
};

void storeInOpenDX2(const char* filename, Vector2* vector, int width, int height, int depth = 1)
{
    FILE* file = fopen(filename, "wb");

    int ori_width = width;
    int ori_height = height;

    int step = 4;
    width = width / step;
    height = height / step;

    int total_count = width * height * depth;
    fprintf(file, "object 1 class gridpositions counts %d %d %d\n", width, height, depth);
    fprintf(file, "origin 0 0 0\n");
    fprintf(file, "delta 50 0 0\n");
    fprintf(file, "delta 0 50 0\n");
    fprintf(file, "delta 0 0 50\n");
    fprintf(file, "object 2 class gridconnections counts %d %d %d\n", width, height, depth);
    fprintf(file, "object 3 class array type double shape 3 rank 1 items %d data follows\n", total_count);


    Vector2* ptr;
    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < ori_height; y += step) {
            for (int x = 0; x < ori_width; x += step) {
		double vx = 0, vy = 0;
		int count = 0;
		for (int yi = 0; yi < step; ++yi)
		{
			for (int xi = 0; xi < step; ++xi)
			{
				if (((x + xi) < ori_width) && ((y+yi) < ori_height))
				{
                			ptr = vector + ((y + yi) * ori_width + (x+xi));
					vx += ptr->x;
					vy += ptr->y;
					++count;
				}
			}
		}
                fprintf(file, "%lf %lf 0.0\n", ptr->x / count, ptr->y / count);
            }
        }
    } 

    fprintf(file, "attribute \"dep\" string \"positions\"\n");
    fprintf(file, "object \"regular positions regular connections\" class field\n");
    fprintf(file, "component \"positions\" value 1\n");
    fprintf(file, "component \"connections\" value 2\n");
    fprintf(file, "component \"data\" value 3\n");

    fclose(file);
}
void storeInOpenDX(const char* filename, Vector2* vector, int width, int height, int depth = 1)
{
    FILE* file = fopen(filename, "wb");

    int ori_width = width;
    int ori_height = height;

    if (ori_width > 512) {
        width = 512;
    }
    if (ori_height > 512) {
        height = 512;
    }

    int total_count = width * height * depth;
    fprintf(file, "object 1 class gridpositions counts %d %d %d\n", height, width, depth);
    fprintf(file, "origin -0.5 -22 0\n");
    fprintf(file, "delta 50 0 0\n");
    fprintf(file, "delta 0 50 0\n");
    fprintf(file, "delta 0 0 50\n");
    fprintf(file, "object 2 class gridconnections counts %d %d %d\n", height, width, depth);
    fprintf(file, "object 3 class array type double shape 3 rank 1 items %d data follows\n", total_count);


    if (ori_width <= 512 && ori_height <= 512) {
        Vector2* ptr;
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
        for (int z = 0; z < depth; ++z) {
                    ptr = vector + ((y * ori_width) +  x);
                    fprintf(file, "%f %f 0.0\n", ptr->x, ptr->y);
                }
            }
        }
    } else {
        int offsetx = 0, offsety = 0;
        if (ori_width > 512) {
            width = 512;
            offsetx = (ori_width - width) / 2;
        }
        if (ori_height > 512) {
            height = 512;
            offsety = (ori_height - height) / 2;
        }

        Vector2* ptr;
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
        	for (int z = 0; z < depth; ++z) {
                    ptr = vector + ((y + offsety) * ori_width +  x + offsetx);
                    fprintf(file, "%lf %lf 0.0\n", ptr->x, ptr->y);
                }
            }
        }
    }

    fprintf(file, "attribute \"dep\" string \"positions\"\n");
    fprintf(file, "object \"regular positions regular connections\" class field\n");
    fprintf(file, "component \"positions\" value 1\n");
    fprintf(file, "component \"connections\" value 2\n");
    fprintf(file, "component \"data\" value 3\n");

    fclose(file);
}

int main(int argc, char* argv[])
{
    // int width = 22;
    // int width = 77;
    // int width = 218;
    // int width = 610;
    int width = 109;

    // int height = 61;
    // int height = 28;
    // int height = 610;
    // int height = 218;
    int height = 305;

    //int depth = 2;
    int depth = 25;

    FILE* xComp = fopen("Ix_data.txt", "rb");
    FILE* yComp = fopen("Iy_data.txt", "rb");
    Vector2* data = new Vector2[width * height];

    int index = 0;
    char buff[256];
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            fscanf(xComp, "%s", buff);
            if (strcmp(buff, "NaN") == 0) {
                data[index].x = 0;
            } else {
                data[index].x = atof(buff);
            }


            fscanf(yComp, "%s", buff);
            if (strcmp(buff, "NaN") == 0) {
                data[index].y = 0;
            } else {
                data[index].y = atof(buff);
            }

            ++index;
        }

    }

    fclose(xComp);
    fclose(yComp);

    // storeInOpenDX2("flow2d.dx", data, width, height, depth);
    storeInOpenDX("flow2d.dx", data, width, height, depth);
}
