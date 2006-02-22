#include "node3d.h"
#include <vector>

int main()
{
	vector<RpNode3d> list(10);
	int i;
	int tmp[3];

	for (i=0; i<10; i++) {
		tmp[0] = i;
		tmp[1] = i+1;
		tmp[2] = i+2;
		list[i].set(tmp);
		list[i].id(i);
	}
	printf("list size: %d\n", list.size());
	for (i=0; i<10; i++) {
		list[i].print();
	}

	return 0;
}
