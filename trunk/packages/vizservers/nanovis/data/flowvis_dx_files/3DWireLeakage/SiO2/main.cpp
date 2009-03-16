#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct Vector2 {
    double x, y;
};

void storeInOpenDX(const char* filename, const char* dstfilename)
{
    FILE* srcFile = fopen(filename, "rb");
    FILE* dstfile = fopen(dstfilename, "wb");

    char buff[256];
    fscanf(srcFile, "%s", buff);
    double orix = atof(buff);
    fscanf(srcFile, "%s", buff);
    double dx = atof(buff);
    fscanf(srcFile, "%s", buff);
    int nx = (int) atof(buff);

    fscanf(srcFile, "%s", buff);
    double oriy = atof(buff);
    fscanf(srcFile, "%s", buff);
    double dy = atof(buff);
    fscanf(srcFile, "%s", buff);
    int ny = (int) atof(buff);
    fscanf(srcFile, "%s", buff);
    double oriz = atof(buff);
    fscanf(srcFile, "%s", buff);
    double dz = atof(buff);
    fscanf(srcFile, "%s", buff);
    int nz = (int) atof(buff);

    int total_count =  nx * ny * nz;
    fprintf(dstfile, "object 1 class gridpositions counts %d %d %d\n",  nx,  ny,  nz);
    fprintf(dstfile, "origin %lf %lf %lf\n", orix, oriy, oriz);
    fprintf(dstfile, "delta %lf 0 0\n", dx);
    fprintf(dstfile, "delta 0 %lf 0\n", dy);
    fprintf(dstfile, "delta 0 0 %lf\n", dz);
    fprintf(dstfile, "object 2 class gridconnections counts %d %d %d\n",  nx,  ny,  nz);
    fprintf(dstfile, "object 3 class array type double shape 3 rank 1 items %d data follows\n", nx * ny * nz);

    double x, y, z;
    char buff2[256], buff3[256];
    for (int i = 0; i < total_count; ++i)
    {
        fscanf(srcFile, "%s", buff);
	if (!strcmp(buff, "NaN")) { buff[0] = '0'; buff[1] = '\0'; }

        fscanf(srcFile, "%s", buff2);
	if (!strcmp(buff2, "NaN")) { buff2[0] = '0'; buff2[1] = '\0'; }

        fscanf(srcFile, "%s", buff3);
	if (!strcmp(buff3, "NaN")) { buff3[0] = '0'; buff3[1] = '\0'; }

	fprintf(dstfile, "%s %s %s\n", buff, buff2, buff3);
    }


    fprintf(dstfile, "attribute \"dep\" string \"positions\"\n");
    fprintf(dstfile, "object \"regular positions regular connections\" class field\n");
    fprintf(dstfile, "component \"positions\" value 1\n");
    fprintf(dstfile, "component \"connections\" value 2\n");
    fprintf(dstfile, "component \"data\" value 3\n");

    fclose(srcFile);
    fclose(dstfile);
}

int main(int argc, char* argv[])
{
    storeInOpenDX(argv[1], argv[2]);
}
