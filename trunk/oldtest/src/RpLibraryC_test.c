/*
 * ======================================================================
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <stdio.h>
#include "RpLibraryCInterface.h"

int test_lib(RpLibrary** lib, const char* path);
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

int test_lib (RpLibrary** lib, const char* path) {

    int retVal = 0;

    *lib = NULL;
    *lib = rpLibrary(path);

    printf("TESTING LIBRARY: path = %s\n", path);

    if (*lib != NULL) {
        printf("creation of library successful\n");
    }
    else {
        printf("creation of library failed\n");
        retVal += 1;
    }

    printf ("lib = %x\n",(unsigned int)(*lib));

    return retVal;
}

int test_element (RpLibrary* lib, const char* path )
{
    int retVal = 0;
    RpLibrary* searchEle = NULL;
    const char* type = NULL;
    const char* comp = NULL;
    const char* id = NULL;

    printf("TESTING ELEMENT: path = %s\n", path);

    searchEle = rpElement(lib,path);
    retVal += rpElementAsType(lib,path,&type);
    retVal += rpElementAsComp(lib,path,&comp);
    retVal += rpElementAsId(lib,path,&id);

    if (!searchEle) {
        printf("searchEle is NULL\n");
        retVal += 1;
    }
    else {
        printf("searchEle comp = :%s:\n", comp);
        printf("searchEle   id = :%s:\n", id);
        printf("searchEle type = :%s:\n", type);

        retVal += rpNodeComp(searchEle,&comp);
        retVal += rpNodeId(searchEle,&id);
        retVal += rpNodeType(searchEle,&type);

        printf("searchEle comp = :%s:\n", comp);
        printf("searchEle   id = :%s:\n", id);
        printf("searchEle type = :%s:\n", type);

    }

    return retVal;
}

int test_getString (RpLibrary* lib, const char* path )
{
    int retVal = 0;
    const char* searchVal = NULL;

    printf("TESTING GET String: path = %s\n", path);

    retVal += rpGetString(lib,path,&searchVal);

    if ( (retVal) || !searchVal || *searchVal == '\0') {
        printf("searchVal is EMPTY STRING\n");
        retVal += 1;
    }
    else {
        printf("searchVal = :%s:\n", searchVal);
        retVal = 0;
    }

    return retVal;
}

int test_getDouble (RpLibrary* lib, const char* path )
{
    int retVal = 0;
    double searchVal = 0.0;

    printf("TESTING GET Double: path = %s\n", path);

    retVal = rpGetDouble(lib,path,&searchVal);

    if (!retVal) {
        printf("searchVal = :%f:\n", searchVal);
    }
    else {
        printf("GET Double: FAILED\n");
    }

    return retVal;
}

int test_children (RpLibrary* lib, const char* path)
{
    int retVal = 0;
    RpLibrary* childEle = NULL;
    const char* id = NULL;
    const char* comp = NULL;
    const char* type = NULL;

    printf("TESTING CHILDREN: path = %s\n", path);

    while ( (childEle = rpChildren(lib,path,childEle)) ) {
        retVal += rpNodeComp(childEle,&comp);
        retVal += rpNodeId(childEle,&id);
        retVal += rpNodeType(childEle,&type);

        printf("childEle comp = :%s:\n",comp);
        printf("childEle   id = :%s:\n",id);
        printf("childEle type = :%s:\n",type);
    }

    return retVal;
}

int test_childrenByType (RpLibrary* lib, const char* path, const char* searchType )
{
    int retVal = 0;
    RpLibrary* childEle = NULL;
    const char* id = NULL;
    const char* comp = NULL;
    const char* type = NULL;

    printf("TESTING CHILDREN: path = %s\n", path);

    while ( (childEle = rpChildrenByType(lib,path,childEle,searchType)) ) {
        retVal += rpNodeComp(childEle,&comp);
        retVal += rpNodeId(childEle,&id);
        retVal += rpNodeType(childEle,&type);

        printf("childEle comp = :%s:\n",comp);
        printf("childEle   id = :%s:\n",id);
        printf("childEle type = :%s:\n",type);
    }

    return retVal;
}

int test_putString (RpLibrary* lib, const char* path, const char* value, int append)
{
    int retVal = 0;
    const char* searchVal = NULL;

    printf("TESTING PUT String: path = %s\n", path);

    retVal += rpPutString(lib,path,value,append);
    if (!retVal) {
        retVal += rpGetString(lib, path, &searchVal);
    }

    if (retVal || !searchVal || *searchVal == '\0') {
        printf("searchVal is EMPTY STRING, or there was an error\n");
    }
    else {
        printf("searchVal = :%s:\n", searchVal);
    }

    return retVal;
}

int test_putDouble (RpLibrary* lib, const char* path, double value, int append)
{
    int retVal = 0;
    double searchVal = 0.0;

    printf("TESTING PUT String: path = %s\n", path);

    retVal += rpPutDouble(lib,path,value,append);
    if (!retVal) {
        retVal += rpGetDouble(lib, path,&searchVal);
    }

    if ( retVal ){
        printf("ERROR while retrieving searchVal\n");
    }
    else {
        printf("searchVal = :%f:\n", searchVal);
    }

    return retVal;
}

int
main(int argc, char** argv)
{
    RpLibrary* lib = NULL;
    int err = 0;
    const char* retStr = NULL;

    if (argc < 2)
    {
        printf("usage: %s driver.xml\n", argv[0]);
        return -1;
    }

    test_lib(&lib,argv[1]);

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

    err = rpXml(lib,&retStr);

    if ( !err ) {
        printf("XML = \n%s\n",retStr);
    }
    else {
        printf("rpXML failed\n");
    }

    rpFreeLibrary(&lib);

    return 0;
}
