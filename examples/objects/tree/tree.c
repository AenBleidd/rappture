#include <stdio.h>
#include <string.h>
#include "RpTree.h"

int tree_0_0 ()
{
    const char *desc = "test creation of tree";
    const char *testname = "tree_0_0";

    const char *treeName = "mytree";
    const char *expected = "mytree";
    const char *received = NULL;

    Rp_Tree t;

    Rp_TreeCreate(treeName,t);
    received = Rp_TreeName(t);

    if (strcmp(expected,received) != 0) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", desc);
        printf("\texpected \"%s\"\n",expected);
        printf("\treceived \"%s\"\n",received);
        return 1;
    }

    return 0;
}

int main()
{
    tree_0_0();

    return 0;
}
