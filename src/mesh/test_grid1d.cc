#include <stdio.h>
#include "grid1d.h"

#define Num_points 20

static double points[20] = {
	0.261243291268,
	0.159055762008,
	0.214550893827,
	0.11741510008,
	0.119735699467,
	0.15196145742,
	0.0245663797288,
	0.128903081711,
	0.0927746958394,
	0.0465364541799,
	0.531606236106,
	0.252448742721,
	0.348575614391,
	0.180939456908,
	0.0251118046222,
	0.810354715199,
	0.0980414196039,
	0.392354903151,
	0.151346474849,
	0.368703495654
};

static double points2[20][2] = {
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

	printf("Testing RpGrid1d\n");

	printf("Testing constructor\n");
	RpGrid1d grid1(20);

	printf("vector size: %d\n", grid1.numPoints());
	printf("capacity: %d\n", grid1.capacity());

	printf("Testing addAllPoints\n");
	grid1.addAllPoints(&(points[0]), 20);

	printf("vector size: %d\n", grid1.numPoints());

	printf("Testing print\n");
	grid1.print();

	printf("Testing serialize\n");

	int nbytes;
	char* buf = grid1.serialize(nbytes);
	printf("nbytes=%d\n", nbytes);

	FILE* fp = fopen("out.grid", "w");
	fwrite(buf, 1, nbytes, fp);
	fclose(fp);

	printf("Testing deserialize\n");
	grid1.deserialize(buf);
	grid1.print();

	printf("Testing instantiation from byte stream\n");
	RpGrid1d * grid2;

	grid2 = new RpGrid1d(buf);

	grid2->print();

	printf("Testing addPoint\n");

	RpGrid1d grid3(20);
	grid3.addPoint(points[0]);
	grid3.addPoint(points[1]);
	printf("size=%d\n", grid3.numPoints());
	grid3.print();


	return 0;
}
