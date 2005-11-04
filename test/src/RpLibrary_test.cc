/**
 *
 * RpParser_test.c
 *
 * test file for the RpParser.so library based off of the scew
 * libaray, a simple wrapper around the expat parser
 *
 * Copyright (c) 2004-2005  Purdue Research Foundation
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "RpLibrary.h"

int test_element (RpLibrary* lib, std::string path );
int test_get (RpLibrary* lib, std::string path );
int test_getString (RpLibrary* lib, std::string path );
int test_getDouble (RpLibrary* lib, std::string path );

int test_element (RpLibrary* lib, std::string path )
{
    int retVal = 1;
    RpLibrary* searchEle = lib->element(path);

    std::cout << "TESTING ELEMENT: path = " << path << std::endl;

    if (!searchEle) {
        std::cout << "searchEle is NULL" << std::endl;
        retVal = 1;
    }
    else {
        std::cout << "searchEle comp = :" << searchEle->nodeComp() << ":" << std::endl;
        std::cout << "searchEle   id = :" << searchEle->nodeId()   << ":" << std::endl;
        std::cout << "searchEle type = :" << searchEle->nodeType() << ":" << std::endl;
        retVal = 0;
    }

    return retVal;
}

int test_get (RpLibrary* lib, std::string path )
{
    int retVal = 1;
    std::string searchVal = lib->get(path);

    std::cout << "TESTING GET       : path = " << path << std::endl;

    if (searchVal.empty()) {
        std::cout << "searchVal is EMPTY STRING" << std::endl;
        retVal = 1;
    }
    else {
        std::cout << "searchVal = :" << searchVal << ":" << std::endl;
        retVal = 0;
    }

    return retVal;
}

int test_getString (RpLibrary* lib, std::string path )
{
    int retVal = 1;
    std::string searchVal = lib->getString(path);

    std::cout << "TESTING GET String: path = " << path << std::endl;

    if (searchVal.empty()) {
        std::cout << "searchVal is EMPTY STRING" << std::endl;
        retVal = 1;
    }
    else {
        std::cout << "searchVal = :" << searchVal << ":" << std::endl;
        retVal = 0;
    }

    return retVal;
}

int test_getDouble (RpLibrary* lib, std::string path )
{
    int retVal = 1;
    double searchVal = lib->getDouble(path);

    std::cout << "TESTING GET Double: path = " << path << std::endl;

    std::cout << "searchVal = :" << searchVal << ":" << std::endl;
    retVal = 0;

    return retVal;
}

/*
int test_children (RpLibrary* lib, std::string path, std::string type )
{
    int retVal = 1;
    int childNum = -1;

    std::cout << "TESTING CHILDREN: path = " << path << std::endl;
    RpLibrary** searchEle = lib->children(path,"",type);

    if (!searchEle || !*searchEle) {
        std::cout << "searchEle is NULL -> NO CHILDREN" << std::endl;
        retVal = 1;
    }
    else {

        while (searchEle[++childNum]) {
            std::cout << "searchEle comp = :" << searchEle[childNum]->nodeComp() 
                << ":" << std::endl;
            std::cout << "searchEle   id = :" << searchEle[childNum]->nodeId()   
                << ":" << std::endl;
            std::cout << "searchEle type = :" << searchEle[childNum]->nodeType() 
                << ":" << std::endl;
            delete (searchEle[childNum]);
            searchEle[childNum] = NULL;
        }

        retVal = 0;

    }

    delete[] searchEle;

    return retVal;
}
*/

int test_children (RpLibrary* lib, std::string path, std::string type )
{
    int retVal = 1;
    RpLibrary* childEle = NULL;

    std::cout << "TESTING CHILDREN: path = " << path << std::endl;

    while ( (childEle = lib->children(path,childEle,type)) ) {

        std::cout << "childEle comp = :" << childEle->nodeComp() 
            << ":" << std::endl;
        std::cout << "childEle   id = :" << childEle->nodeId()   
            << ":" << std::endl;
        std::cout << "childEle type = :" << childEle->nodeType() 
            << ":" << std::endl;

        retVal = 0;

    }

    return retVal;
}

int
main(int argc, char** argv)
{
    RpLibrary* lib = NULL;
    RpLibrary lib2;

    if (argc < 2)
    {
        printf("usage: RpLibrary_test infile.xml\n");
        return EXIT_FAILURE;
    }

    lib = new RpLibrary(std::string(argv[1]));

    test_element(lib,"input.number(min)");
    test_element(lib,"input.number(max)");
    test_element(lib,"output.curve(result)");

    test_getString(lib, "input.number(min).default");
    test_getString(lib, "input.(min).current");
    test_getString(lib, "input.number(max).current");
    test_getString(lib, "output.curve.about.label");

    lib->put("input.number(test_one).default", "3000");
    test_get(lib, "input.number(test_one).default");
    lib->put("input.number(test).default", "1000");
    test_getString(lib, "input.number(test).default");
    lib->put("input.number(test).current", 2000);
    test_getDouble(lib, "input.number(test).current");

    
    test_children(lib,"","");
    test_children(lib,"input.number(test)","");

    std::cout << lib->xml() << std::endl;
    
    // test copy assignment operator
    lib2 = *lib;

    delete lib;

    lib2.put("input.output.curve(curve1).xy.default", "1e-4 0.0\n");
    lib2.put("input.output.curve(curve1).xy.default", "1e-2 1.0\n","",1);
    lib2.put("input.output.curve(curve1).xy.default", "1e-30 2.0\n","",1);
    lib2.put("input.output.curve(curve1).xy.default", "1e+4 3.0\n","",1);
    lib2.put("input.output.curve(curve1).xy.default", "1.7e-4 4.0\n","",1);
    lib2.put("input.output.curve(curve1).xy.default", "15e-4 5.0\n","",1);


    // test copy constructor
    RpLibrary lib3 = lib2;

    lib2.put("input.output.curve(curve2).xy.current", "1e-4 0.0\n");
    lib2.put("input.output.curve(curve2).xy.current", "1e-2 1.0\n","",1);
    lib2.put("input.output.curve(curve2).xy.current", "1e-30 2.0\n","",1);
    lib2.put("input.output.curve(curve2).xy.current", "1e+4 3.0\n","",1);
    lib2.put("input.output.curve(curve2).xy.current", "1.7e-4 4.0\n","",1);
    lib2.put("input.output.curve(curve2).xy.current", "15e-4 5.0\n","",1);


    std::cout << "//////////////////// LIB 2 ////////////////////" << std::endl;
    std::cout << lib2.xml() << std::endl;
    std::cout << "//////////////////// LIB 3 ////////////////////" << std::endl;
    std::cout << lib3.xml() << std::endl;

    lib2.result();

    return 0;
}

    /*
    put(lib, "element3.sub_element(value3)", "put_element3", NULL, 1);
    
    search_element = _find(lib->element,"element3.sub_element(value3)",0);
    printf("search_element name = %s\n", scew_element_name(search_element));

    // search_element2 = element(root->element, "element3.sub_element(value3)", "object");

    search_element2 = element(lib, "element3.sub_element(value3)", OBJECT);
    printf("search_element2 name = %s\n", scew_element_name(search_element2->element));
    freeRpLibrary(&search_element2);

    search_element2 = element(lib, "element3.sub_element(value3)", COMPONENT);
    printf("search_element2 comp = %s\n", search_element2->stringVal);
    freeRpLibrary(&search_element2);
    
    search_element2 = element(lib, "element3.sub_element(value3)", ID);
    printf("search_element2 id = %s\n", search_element2->stringVal);
    freeRpLibrary(&search_element2);
    
    search_element2 = element(lib, "element3.sub_element(value3)", TYPE);
    printf("search_element2 type = %s\n", search_element2->xml_char);
    freeRpLibrary(&search_element2);
    
    
    get_element = get(lib, "element");
    printf("get element's contents = %s\n", get_element->xml_char);
    freeRpLibrary(&get_element);


        
    put(lib, "element4.sub_element(value)", "put1_element", NULL, 0);
    put(lib, "element4.sub_element(value2)", "put2_element", NULL, 0);
    put(lib, "element4.sub_element(value3)", "put3_element", NULL, 0);
    put(lib, "element4.sub_element(value4)", "put4_element", NULL, 0);
    get_element = get(lib, "element4.sub_element(value)");
    printf("put1 element's contents = %s\n", get_element->xml_char);
    get_element = get(lib, "element4.sub_element(value2)");
    printf("put2 element's contents = %s\n", get_element->xml_char);
    get_element = get(lib, "element4.sub_element(value3)");
    printf("put3 element's contents = %s\n", get_element->xml_char);
    get_element = get(lib, "element4.sub_element(value4)");
    printf("put4 element's contents = %s\n", get_element->xml_char);

    */
    /**
     * Save an XML tree to a file.
     */
    /*
    if (!scew_writer_tree_file(lib->tree, argv[2]))
    {
        printf("Unable to create %s\n", argv[2]);
        return EXIT_FAILURE;
    }
    */


    /*
    
    freeRpLibrary(&search_element2);
    freeRpLibrary(&get_element);
    freeRpLibrary(&lib);

    if (tagName) { free (tagName); }
    if (id) { free(id); }
    */
