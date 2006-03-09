#include <stdio.h>
#include "serializer.h"
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

void writeToFile(char* buf, int len, const char* filename)
{
	if (buf != NULL) {
		FILE* fp = fopen(filename, "w");
		fwrite(buf, 1, len, fp);
		fclose(fp);
	}
}

int main()
{
	int i, j, err, nbytes;

	printf("Testing RpGrid1d\n");

	RpGrid1d* grid1 = new RpGrid1d("output.grid(g1d)", 20);
	grid1->addAllPoints(&(points[0]), 20);

	char* buf = grid1->serialize(nbytes);
	writeToFile(buf, nbytes, "out.grid1");

	delete [] buf; 
	//buf = NULL;

	printf("Testing serializer\n");

	RpSerializer myvis;

	myvis.addObject(grid1);

	RpGrid1d* ptr = (RpGrid1d*) myvis.getObject(grid1->objectName());

	if (ptr == NULL)
		printf("%s not found\n", grid1->objectName());

	myvis.print();

	buf = myvis.serialize();

	nbytes = myvis.numBytes();
	writeToFile(buf, nbytes, "out.g1");


	return 0;
}
