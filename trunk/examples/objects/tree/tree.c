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

    Rp_TreeCreate(treeName,&t);
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

int tree_1_0 ()
{
    const char *desc = "test creation of tree node";
    const char *testname = "tree_1_0";

    const char *treeName = "mytree";
    const char *nodeName = "mynode";
    const char *expected = "mynode";
    const char *received = NULL;

    Rp_Tree t;
    Rp_TreeNode n = NULL;

    Rp_TreeCreate(treeName,&t);
    n = Rp_TreeCreateNode(t, Rp_TreeRootNode(t), nodeName, 0);
    received = Rp_TreeNodeLabel(n);

    if (strcmp(expected,received) != 0) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", desc);
        printf("\texpected \"%s\"\n",expected);
        printf("\treceived \"%s\"\n",received);
        return 1;
    }

    return 0;
}

int tree_2_0 ()
{
    const char *desc = "test creation of tree node with single child";
    const char *testname = "tree_2_0";

    const char *treeName = "mytree";
    const char *parentNodeName = "myparentnode";
    const char *childNodeName = "mychildnode";
    const char *expected = "mychildnode";
    const char *received = NULL;

    Rp_Tree t;
    Rp_TreeNode parent = NULL;
    Rp_TreeNode child = NULL;

    Rp_TreeCreate(treeName,&t);
    parent = Rp_TreeCreateNode(t, Rp_TreeRootNode(t), parentNodeName, 0);
    child = Rp_TreeCreateNode(t, parent, childNodeName, 0);
    received = Rp_TreeNodeLabel(child);

    if (strcmp(expected,received) != 0) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", desc);
        printf("\texpected \"%s\"\n",expected);
        printf("\treceived \"%s\"\n",received);
        return 1;
    }

    return 0;
}

int tree_2_1 ()
{
    const char *desc = "test creation of tree node with multiple children";
    const char *testname = "tree_2_1";

    const char *treeName = "mytree";
    const char *parentNodeName = "myparentnode";
    size_t expected = 5;
    size_t received = 0;

    Rp_Tree t;
    Rp_TreeNode parent = NULL;

    Rp_TreeCreate(treeName,&t);
    parent = Rp_TreeCreateNode(t, Rp_TreeRootNode(t), parentNodeName, 0);

    Rp_TreeCreateNode(t, parent, "child1", 0);
    Rp_TreeCreateNode(t, parent, "child2", 0);
    Rp_TreeCreateNode(t, parent, "child3", 0);
    Rp_TreeCreateNode(t, parent, "child4", 0);
    Rp_TreeCreateNode(t, parent, "child5", 0);

    received = Rp_TreeNodeDegree(parent);

    if (expected != received) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", desc);
        printf("\texpected \"%zu\"\n",expected);
        printf("\treceived \"%zu\"\n",received);
        return 1;
    }

    return 0;
}

int tree_2_2 ()
{
    const char *desc = "test search for child node";
    const char *testname = "tree_2_2";

    const char *treeName = "mytree";
    const char *parentNodeName = "myparentnode";
    const char *childNodeName = "mychildnode";
    const char *expected = "mychildnode";
    const char *received = NULL;

    Rp_Tree t;
    Rp_TreeNode parent = NULL;
    Rp_TreeNode foundChild = NULL;

    Rp_TreeCreate(treeName,&t);
    parent = Rp_TreeCreateNode(t, Rp_TreeRootNode(t), parentNodeName, 0);

    Rp_TreeCreateNode(t, parent, "child1", 0);
    Rp_TreeCreateNode(t, parent, "child2", 0);
    Rp_TreeCreateNode(t, parent, "child3", 0);
    Rp_TreeCreateNode(t, parent, childNodeName, 0);
    Rp_TreeCreateNode(t, parent, "child4", 0);
    Rp_TreeCreateNode(t, parent, "child5", 0);

    foundChild = Rp_TreeFindChild(parent,childNodeName);
    received = Rp_TreeNodeLabel(foundChild);

    if (strcmp(expected,received) != 0) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", desc);
        printf("\texpected \"%s\"\n",expected);
        printf("\treceived \"%s\"\n",received);
        return 1;
    }

    return 0;
}

int tree_3_0 ()
{
    const char *desc = "test adding node with string value";
    const char *testname = "tree_3_0";

    const char *treeName = "mytree";
    const char *parentNodeName = "myparentnode";
    const char *childNodeName = "mychildnode";
    const char *childNodeValue = "quick brown fox";
    const char *expected = "quick brown fox1";
    const char *received = NULL;

    Rp_Tree t;
    Rp_TreeNode parent = NULL;
    Rp_TreeNode child = NULL;

    Rp_TreeCreate(treeName,&t);
    parent = Rp_TreeCreateNode(t, Rp_TreeRootNode(t), parentNodeName, 0);

    child = Rp_TreeCreateNode(t, parent, childNodeName, 0);
    Rp_TreeSetValue(t,child,"value",(void *)childNodeValue);

    Rp_TreeGetValue(t,child,"value",(void **)&received);

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
    tree_1_0();
    tree_2_0();
    tree_2_1();
    tree_2_2();
    tree_3_0();

    // add test case where two children have the same name but different ids

    return 0;
}
