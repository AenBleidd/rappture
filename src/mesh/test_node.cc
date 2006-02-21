#include "node.h"
#include <vector>

int main()
{
	vector<RpNode> list(10);
	int i;
	int tmp[3];

	for (i=0; i<10; i++) {
		tmp[0] = i;
		tmp[1] = i+1;
		tmp[2] = i+2;
		(list[i]).set(3, tmp, i);
	}
	printf("list size: %d\n", list.size());
	for (i=0; i<10; i++) {
		printf("<node %d> ", list[i].id());
		list[i].print();
	}

	return 0;
}
