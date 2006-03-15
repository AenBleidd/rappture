#include <stdio.h>
#include "grid3d.h"

#define Num_points 20

static double points[] = {
	0.309504615287, 0.844069129715, 0.269863116215, 
	0.589394224151, 0.948725301748, 0.226146486693,
	0.844001851438, 0.139117111982, 0.141301087635, 
	0.847379885543, 0.913736318664, 0.166307778641,
	0.134835615817, 0.182195034894, 0.151951467689, 
	0.848317450773, 0.671395143807, 0.138181958412,
	0.424175028887, 0.10971049923, 0.904360553671, 
	0.587825545849, 0.58394908746, 0.432312932998,
	0.88346489886, 0.394555137211, 0.288191101648, 
	0.627845397977, 0.197603800892, 0.127081587039,
	0.86023335618, 0.942017313532, 0.484988525736, 
	0.202152039484, 0.56932761081, 0.689154883236,
	0.62612255133, 0.241720210408, 0.591576322723,
	0.241720210408, 0.591576322723, 0.623256005171,
	0.848317450773, 0.671395143807, 0.138181958412,
	0.424175028887, 0.10971049923, 0.904360553671, 
	0.587825545849, 0.58394908746, 0.432312932998,
	0.88346489886, 0.394555137211, 0.288191101648, 
	0.627845397977, 0.197603800892, 0.127081587039,
	0.86023335618, 0.942017313532, 0.484988525736
};

static double pts[20][3] = {
	{0.309504615287, 0.844069129715, 0.269863116215}, 
	{0.589394224151, 0.948725301748, 0.226146486693},
	{0.844001851438, 0.139117111982, 0.141301087635}, 
	{0.847379885543, 0.913736318664, 0.166307778641},
	{0.134835615817, 0.182195034894, 0.151951467689}, 
	{0.848317450773, 0.671395143807, 0.138181958412},
	{0.424175028887, 0.10971049923, 0.904360553671}, 
	{0.587825545849, 0.58394908746, 0.432312932998},
	{0.88346489886, 0.394555137211, 0.288191101648}, 
	{0.627845397977, 0.197603800892, 0.127081587039},
	{0.86023335618, 0.942017313532, 0.484988525736}, 
	{0.202152039484, 0.56932761081, 0.689154883236},
	{0.62612255133, 0.241720210408, 0.591576322723},
	{0.241720210408, 0.591576322723, 0.623256005171},
	{0.848317450773, 0.671395143807, 0.138181958412},
	{0.424175028887, 0.10971049923, 0.904360553671}, 
	{0.587825545849, 0.58394908746, 0.432312932998},
	{0.88346489886, 0.394555137211, 0.288191101648}, 
	{0.627845397977, 0.197603800892, 0.127081587039},
	{0.86023335618, 0.942017313532, 0.484988525736}
};

static double xval[] = {0.0, 0.2, 0.3, 1.5, 1.8};
static double yval[] = {0.2, 1.5, 1.8};
static double zval[] = {0.0};

int main()
{
	int i, j, err;

	printf("Testing RpGrid3d\n");

	RpGrid3d g1(20);

	printf("Testing addAllPoints\n");
	g1.addAllPoints(&(points[0]), 20);
	g1.objectName("output.mesh(m3d)");

	printf("num points: %d\n", g1.numPoints());

	printf("Testing print\n");
	g1.print();

	printf("Testing RpGrid3d(double* val[], int)\n");

	RpGrid3d gr2(&(pts[0][0]), 20);
	gr2.objectName("output.mesh(m3d2)");
	gr2.print();

	printf("Testing RpGrid3d uniformgrid3d constructor\n");
	RpGrid3d gr3(0.0, 5.0, 1.0, 0.0, 3.0, 1.0, 0.0, 0.0, 1.0);
	gr3.objectName("output.mesh(m3d3)");
	gr3.print();

	printf("Testing RpGrid3d rectilinear constructor\n");
	RpGrid3d gr4(xval, 5, yval, 3, zval, 1);
	gr4.objectName("output.mesh(m3d3)");
	gr4.print();

	printf("Testing serialize\n");

	int nbytes;
	char* buf = gr2.serialize(nbytes);
	printf("nbytes=%d\n", nbytes);

	FILE* fp = fopen("out.g3d", "w");
	fwrite(buf, 1, nbytes, fp);
	fclose(fp);

	RpGrid3d g2;

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
