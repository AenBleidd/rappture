#include "node2d.h"
#include <vector>

int main()
{
	vector<RpNode2d> list(10);
	int i;

	for (i=0; i<10; i++) {
		// set id, x, y 
		list[i].set(i, i, i+1);
	}
	printf("list size: %d\n", list.size());
	for (i=0; i<10; i++) {
		list[i].print();
	}

	return 0;
}
