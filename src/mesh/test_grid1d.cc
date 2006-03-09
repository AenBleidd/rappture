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

int main()
{
	int i, j, err;

	printf("Testing RpGrid1d\n");

	printf("Testing constructor\n");
	RpGrid1d grid1("output.grid(g1d)", 20);

	printf("vector size: %d\n", grid1.size());
	printf("capacity: %d\n", grid1.capacity());

	printf("Testing addAllPoints\n");
	grid1.addAllPoints(&(points[0]), 20);

	printf("vector size: %d\n", grid1.size());

	printf("Testing print\n");
	grid1.print();

	printf("Testing serialize\n");

	int nbytes;
	char* buf = grid1.serialize(nbytes);
	printf("nbytes=%d\n", nbytes);

	FILE* fp = fopen("out.grid", "w");
	fwrite(buf, 1, nbytes, fp);
	fclose(fp);

	printf("Testing print\n");
	grid1.print();

	printf("Testing deserialize\n");

	RpGrid1d * grid2 = new RpGrid1d("test.path");
	grid2->deserialize(buf);

	printf("Testing getData\n");
	
//	double* ptr = grid2->getDataCopy();
	double* ptr = grid2->getData();
	for (i=0; i<grid2->size(); i++) {
		printf("%.12f\n", ptr[i]);
	}



	return 0;
}
