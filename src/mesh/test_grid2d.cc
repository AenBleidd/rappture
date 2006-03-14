#include <stdio.h>
#include "grid2d.h"

#define Num_points 20

static double points[] = {
	0.309504615287, 0.844069129715,
	0.269863116215, 0.589394224151,
	0.948725301748, 0.226146486693,
	0.844001851438, 0.139117111982,
	0.141301087635, 0.847379885543,
	0.913736318664, 0.166307778641,
	0.134835615817, 0.182195034894,
	0.151951467689, 0.848317450773,
	0.671395143807, 0.138181958412,
	0.424175028887, 0.10971049923,
	0.904360553671, 0.587825545849,
	0.58394908746, 0.432312932998,
	0.88346489886, 0.394555137211,
	0.288191101648, 0.627845397977,
	0.197603800892, 0.127081587039,
	0.86023335618, 0.942017313532,
	0.484988525736, 0.202152039484,
	0.56932761081, 0.689154883236,
	0.62612255133, 0.241720210408,
	0.591576322723, 0.623256005171
};

int main()
{
	int i, j, err;

	printf("Testing RpGrid2d\n");

	RpGrid2d g1(20);

	printf("Testing addAllPoints\n");
	g1.addAllPoints(points, 20);
	g1.objectName("output.mesh(m2d)");

	printf("num points: %d\n", g1.numPoints());

	printf("Testing print\n");
	g1.print();

	printf("Testing serialize\n");

	int nbytes;
	char* buf = g1.serialize(nbytes);
	printf("nbytes=%d\n", nbytes);

	FILE* fp = fopen("out.g2d", "w");
	fwrite(buf, 1, nbytes, fp);
	fclose(fp);

	RpGrid2d g2;

	printf("Testing deserialize\n");
	g2.deserialize(buf);
	g2.print();

	/*
	double* ptr = g2.getDataCopy();
	for (i=0; i<g2.numPoints(); i++) {
		printf("%.12f  %.12f\n", ptr[i++], ptr[i]);
	}
	*/


	return 0;
}
