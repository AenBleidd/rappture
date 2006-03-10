#include <stdio.h>
#include "serializer.h"
#include "field.h"

//
// This example show how to create a grid1d object, put it into 
// a serializer object and ask the serializer for a serialized byte stream.
//

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

	const char* name1 = "output.field(f1d1)";
	const char* name2 = "output.field(f1d2)";

	printf("Testing RpField\n");

	RpField* f1 = new RpField(name1, 20);
	f1->addAllPoints(&(points[0]), 20);
	f1->setMesh("output.mesh(m2d)");
	//f1->print();

	//RpGrid1d* grid2 = new RpGrid1d(name2, 20);
	//grid2->addAllPoints(&(points[0]), 20);

	printf("Testing serializer\n");

	RpSerializer myvis;
	myvis.addObject(f1);
	//myvis.addObject(grid2);

	char* buf = myvis.serialize();

	// write the byte stream to a file for verification
	nbytes = myvis.numBytes();
	writeToFile(buf, nbytes, "out.g2");

	//myvis.print();
	
	printf("Testing serializer::deserializer\n");

	RpSerializer newvis;
	newvis.deserialize(buf);
	newvis.print();

	/*
	printf("Testing serializer::getObject\n");

	RpGrid1d* ptr1 = (RpGrid1d*) newvis.getObject(name1);
	if (ptr1 != NULL)
		ptr1->print();
	else
		printf("%s not found\n", name1);

	RpGrid1d* ptr2 = (RpGrid1d*) newvis.getObject(name2);
	if (ptr2 != NULL)
		ptr2->print();
	else
		printf("%s not found\n", name2);
	*/

	delete [] buf;

	return 0;
}


