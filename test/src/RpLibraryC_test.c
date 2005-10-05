#include <stdio.h>
#include "RpLibraryCInterface.h"


int test_element (RpLibrary* lib, const char* path );
int test_get (RpLibrary* lib, const char* path );
int test_getString (RpLibrary* lib, const char* path );
int test_getDouble (RpLibrary* lib, const char* path );
int test_putString (RpLibrary* lib, 
                    const char* path, 
                    const char* value, 
                    int append );
int test_putDouble (RpLibrary* lib, 
                    const char* path, 
                    double value, 
                    int append );
int test_children (RpLibrary* lib, const char* path);
int test_childrenByType (RpLibrary* lib, const char* path, const char* type );

int test_element (RpLibrary* lib, const char* path )
{
    int retVal = 1;
    RpLibrary* searchEle = NULL;
    const char* type = NULL;
    const char* comp = NULL;
    const char* id = NULL;

    printf("TESTING ELEMENT: path = %s\n", path);

    searchEle = element(lib,path);
    type = elementAsType(lib,path);
    comp = elementAsComp(lib,path);
    id = elementAsId(lib,path);

    if (!searchEle) {
        printf("searchEle is NULL\n");
        retVal = 1;
    }
    else {
        printf("searchEle comp = :%s:\n", comp);
        printf("searchEle   id = :%s:\n", id);
        printf("searchEle type = :%s:\n", type);

        comp = nodeComp(searchEle);
        id   = nodeId(searchEle);
        type = nodeType(searchEle);
        
        printf("searchEle comp = :%s:\n", comp);
        printf("searchEle   id = :%s:\n", id);
        printf("searchEle type = :%s:\n", type);

        retVal = 0;
    }

    return retVal;
}

int test_getString (RpLibrary* lib, const char* path )
{
    int retVal = 1;
    const char* searchVal = NULL;

    printf("TESTING GET String: path = %s\n", path);

    searchVal = getString(lib,path);

    if (!searchVal || *searchVal == '\0') {
        printf("searchVal is EMPTY STRING\n");
        retVal = 1;
    }
    else {
        printf("searchVal = :%s:\n", searchVal);
        retVal = 0;
    }

    return retVal;
}

int test_getDouble (RpLibrary* lib, const char* path )
{
    int retVal = 1;
    double searchVal = 0.0;

    printf("TESTING GET Double: path = %s\n", path);

    searchVal = getDouble(lib,path);

    printf("searchVal = :%f:\n", searchVal);
    retVal = 0;

    return retVal;
}

int test_children (RpLibrary* lib, const char* path)
{
    int retVal = 1;
    RpLibrary* childEle = NULL;
    const char* id = NULL;
    const char* comp = NULL;
    const char* type = NULL;

    printf("TESTING CHILDREN: path = %s\n", path);

    while ( (childEle = children(lib,path,childEle)) ) {
        comp = nodeComp(childEle);
        id   = nodeId(childEle);
        type = nodeType(childEle);

        printf("childEle comp = :%s:\n",comp);
        printf("childEle   id = :%s:\n",id);
        printf("childEle type = :%s:\n",type);

    }

    retVal = 0;

    return retVal;
}

int test_childrenByType (RpLibrary* lib, const char* path, const char* searchType )
{
    int retVal = 1;
    RpLibrary* childEle = NULL;
    const char* id = NULL;
    const char* comp = NULL;
    const char* type = NULL;

    printf("TESTING CHILDREN: path = %s\n", path);

    while ( (childEle = childrenByType(lib,path,childEle,searchType)) ) {
        comp = nodeComp(childEle);
        id   = nodeId(childEle);
        type = nodeType(childEle);

        printf("childEle comp = :%s:\n",comp);
        printf("childEle   id = :%s:\n",id);
        printf("childEle type = :%s:\n",type);

    }

    retVal = 0;

    return retVal;
}

int test_putString (RpLibrary* lib, const char* path, const char* value, int append)
{
    int retVal = 1;
    const char* searchVal = NULL;

    printf("TESTING PUT String: path = %s\n", path);

    putString(lib,path,value,append);
    searchVal = getString(lib, path);

    if (!searchVal || *searchVal == '\0') {
        printf("searchVal is EMPTY STRING\n");
        retVal = 1;
    }
    else {
        printf("searchVal = :%s:\n", searchVal);
        retVal = 0;
    }

    return retVal;
}

int test_putDouble (RpLibrary* lib, const char* path, double value, int append)
{
    int retVal = 1;
    double searchVal = 0.0;

    printf("TESTING PUT String: path = %s\n", path);

    putDouble(lib,path,value,append);
    searchVal = getDouble(lib, path);
    printf("searchVal = :%f:\n", searchVal);
    retVal = 0;

    return retVal;
}

int
main(int argc, char** argv)
{
    RpLibrary* lib = NULL;

    if (argc < 3)
    {
        printf("usage: RpLibrary_test infile.xml outfile.xml\n");
        return -1;
    }

    lib = library(argv[1]);

    test_element(lib,"input.number(min)");
    test_element(lib,"input.number(max)");
    test_element(lib,"output.curve(result)");

    test_getString(lib, "input.number(min).default");
    test_getString(lib, "input.(min).current");
    test_getString(lib, "input.number(max).current");
    test_getString(lib, "output.curve.about.label");

    test_putString(lib,"input.number(test).default", "1000", 0);
    test_putDouble(lib, "input.number(test).preset", 1, 0);
    test_putDouble(lib, "input.number(test).current", 2000, 0);
    test_putDouble(lib, "input.number(test).preset(p1)", 100, 0);
    test_putDouble(lib, "input.number(test).preset(p2)", 200, 0);
    test_putDouble(lib, "input.number(test).preset(p3)", 300, 0);
    test_putDouble(lib, "input.number(test).preset(p4)", 400, 0);
    test_putDouble(lib, "input.number(test).preset(p5)", 500, 0);

    
    test_children(lib,"");
    test_children(lib,"input.number(test)");
    test_childrenByType(lib,"input.number(test)","preset");

    printf("XML = \n%s\n",xml(lib));
    
    freeLibrary(lib);

    return 0;
}
