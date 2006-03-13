#include <stdio.h>
#include "serializer.h"
#include "field.h"

//
// This example show how to create field & grid1d objects, put into 
// a serializer and ask for a serialized byte stream.
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

static double points2[20] = {
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
	0.368703495654,
	0.261243291268,
	0.159055762008,
	0.214550893827,
	0.11741510008,
	0.119735699467,
	0.15196145742
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

	const char* fname1 = "output.field(f1d1)";
	const char* fname2 = "output.field(f1d2)";
	const char* gname = "output.mesh(m1d)";

	printf("Testing RpField\n");

	// create field object of 20 points
	RpField* f1 = new RpField(fname1, 20);
	f1->addAllPoints(&(points[0]), 20);
	f1->setMeshId("output.mesh(m1d)");

	// create another field object of 20 points
	RpField* f2 = new RpField(fname2, 20);
	f2->addAllPoints(&(points2[0]), 20);
	f2->setMeshId("output.mesh(m1d)");

	// create a regular 1d grid
	RpGrid1d* grid = new RpGrid1d(0, 1, 20);
	grid->objectName(gname);

	printf("Testing serializer\n");

	// add the objects to serializer
	RpSerializer myvis;
	myvis.addObject(f1);
	myvis.addObject(f2);
	myvis.addObject(grid);

	char* buf = myvis.serialize();

	// write the byte stream to a file for verification
	nbytes = myvis.numBytes();
	writeToFile(buf, nbytes, "out.g2");
	
	// simulating receiving end
	printf("Testing serializer::deserializer\n");

	RpSerializer newvis;
	newvis.deserialize(buf);

	newvis.print();

	printf("Testing serializer::getObject\n");

	RpGrid1d* gptr = (RpGrid1d*) newvis.getObject(gname);
	gptr->print();

	RpField* fptr = (RpField*) newvis.getObject(fname1);
	fptr->print();

	RpField* f2ptr = (RpField*) newvis.getObject(fname2);
	f2ptr->print();

	// delete all objects and buffer
	newvis.clear();

	return 0;
}


