#include <stdio.h>
#include "serializer.h"
#include "field.h"
#include "grid2d.h"

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

static double xy_pts[] = {
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
	const char* gname1 = "output.mesh(m1d)";
	const char* gname2 = "output.mesh(m2d)";

	printf("Testing field\n");

	RpField* f1 = new RpField(fname1, 20);
	f1->addAllValues(&(points[0]), 20);
	f1->setMeshId("output.mesh(m2d)");

	RpField* f2 = new RpField(fname2, 20);
	f2->addAllPoints(&(points2[0]), 20);
	f2->setMeshId("output.mesh(m1d)");

	printf("Testing grid1d\n");

	// use a regular 1d grid
	RpGrid1d* grid = new RpGrid1d(0, 1, 20);
	grid->objectName(gname1);

	printf("Testing grid2d\n");

	RpGrid2d* mesh = new RpGrid2d(0.0, 6.0, 1.0, 0.0, 3.0, 1.0);
	mesh->objectName(gname2);

	printf("Testing serializer\n");

	RpSerializer myvis;
	myvis.addObject(f1);
	myvis.addObject(f2);
	myvis.addObject(grid);
	myvis.addObject(mesh);

	char* buf = myvis.serialize();

	// write the byte stream to a file for verification
	nbytes = myvis.numBytes();
	writeToFile(buf, nbytes, "out.f");
	
	printf("Testing serializer::deserializer\n");

	RpSerializer newvis;
	newvis.deserialize(buf);

	newvis.print();

	printf("Testing serializer::getObject\n");

	RpGrid1d* gptr = (RpGrid1d*) newvis.getObject(gname1);
	gptr->print();

	RpGrid2d* g2ptr = (RpGrid2d*) newvis.getObject(gname2);
	g2ptr->print();

	RpField* fptr = (RpField*) newvis.getObject(fname1);
	fptr->print();

	RpField* f2ptr = (RpField*) newvis.getObject(fname2);
	f2ptr->print();

	newvis.print();

	printf("after clear()\n");
	newvis.clear();
	newvis.print();

	return 0;
}


