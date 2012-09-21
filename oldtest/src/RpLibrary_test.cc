/**
 *
 * RpParser_test.c
 *
 * test file for the RpParser.so library based off of the scew
 * libaray, a simple wrapper around the expat parser
 *
 * Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "RpLibrary.h"
#include <list>
#include <string>
#include <iostream>

using namespace std;

int test_element (RpLibrary* lib, string path);
int test_parent (RpLibrary* lib, string path);
int test_get (RpLibrary* lib, string path);
int test_getString (RpLibrary* lib, string path);
int test_getDouble (RpLibrary* lib, string path);
int test_getInt (RpLibrary* lib, string path);
int test_getBool (RpLibrary* lib, string path);
int test_putObj (RpLibrary* fromLib, string fromPath,
                 RpLibrary* toLib, string toPath);
int test_copy (RpLibrary* fromLib, string fromPath,
               RpLibrary* toLib, string toPath);
int test_remove (RpLibrary* lib, string path);

int test_element (RpLibrary* lib, string path)
{
    int retVal = 1;
    RpLibrary* searchEle = lib->element(path);

    cout << "TESTING ELEMENT: path = " << path << endl;

    if (!searchEle) {
        cout << "searchEle is NULL" << endl;
        retVal = 1;
    }
    else {
        cout << "searchEle path = :" << searchEle->nodePath() << ":" << endl;
        cout << "searchEle comp = :" << searchEle->nodeComp() << ":" << endl;
        cout << "searchEle   id = :" << searchEle->nodeId()   << ":" << endl;
        cout << "searchEle type = :" << searchEle->nodeType() << ":" << endl;
        retVal = 0;
    }

    return retVal;
}

int test_parent (RpLibrary* lib, string path)
{
    int retVal = 1;
    RpLibrary* searchEle = lib->parent(path);

    cout << "TESTING PARENT: path = " << path << endl;

    if (!searchEle) {
        cout << "searchEle is NULL" << endl;
        retVal = 1;
    } else {
        cout << "searchParent path = :" << searchEle->nodePath() << ":" << endl;
        cout << "searchParent comp = :" << searchEle->nodeComp() << ":" << endl;
        cout << "searchParent   id = :" << searchEle->nodeId()   << ":" << endl;
        cout << "searchParent type = :" << searchEle->nodeType() << ":" << endl;
        retVal = 0;
    }

    return retVal;
}

int test_get (RpLibrary* lib, string path)
{
    int retVal = 1;
    string searchVal = lib->get(path);

    cout << "TESTING GET       : path = " << path << endl;

    if (searchVal.empty()) {
        cout << "searchVal is EMPTY STRING" << endl;
        retVal = 1;
    }
    else {
        cout << "searchVal = :" << searchVal << ":" << endl;
        retVal = 0;
    }

    return retVal;
}

int test_getString (RpLibrary* lib, string path)
{
    int retVal = 1;
    string searchVal = lib->getString(path);

    cout << "TESTING GET String: path = " << path << endl;

    if (searchVal.empty()) {
        cout << "searchVal is EMPTY STRING" << endl;
        retVal = 1;
    }
    else {
        cout << "searchVal = :" << searchVal << ":" << endl;
        retVal = 0;
    }

    return retVal;
}

int test_getDouble (RpLibrary* lib, string path)
{
    int retVal = 1;
    double searchVal = lib->getDouble(path);

    cout << "TESTING GET Double: path = " << path << endl;

    cout << "searchVal = :" << searchVal << ":" << endl;
    retVal = 0;

    return retVal;
}

int test_getInt (RpLibrary* lib, string path)
{
    int retVal = 1;
    int searchVal = lib->getInt(path);

    cout << "TESTING GET INT: path = " << path << endl;

    cout << "searchVal = :" << searchVal << ":" << endl;
    retVal = 0;

    return retVal;
}

int 
test_getBool (RpLibrary* lib, string path)
{
    int retVal = 1;
    bool searchVal = lib->getBool(path);

    cout << "TESTING GET Bool: path = " << path << endl;

    cout << "searchVal = :" << searchVal << ":" << endl;
    retVal = 0;

    return retVal;
}

int 
test_putObj (   RpLibrary* fromLib, string fromPath,
                    RpLibrary* toLib, string toPath)
{
    int retVal = 1;
    RpLibrary* fromEle = fromLib->element(fromPath);
    RpLibrary* foundEle = NULL;
    string newNodeName = "";

    cout << "TESTING PUT Object:" << endl;
    cout << "fromPath = " << fromPath << endl;
    cout << "toPath = " << toPath << endl;

    // put the element into the new library/xmltree
    toLib->put(toPath,fromEle);

    // check the result
    // create the name of where the element should be
    newNodeName = toPath + "." + fromEle->nodeComp();
    // grab the element
    if ((foundEle = toLib->element(newNodeName))){
        cout << "SUCCESS: foundEle was found" << endl;
        retVal = 0;
    }

    return retVal;
}

int test_copy (   RpLibrary* fromLib, string fromPath,
                    RpLibrary* toLib, string toPath)
{
    int retVal = 1;
    RpLibrary* fromEle = fromLib->element(fromPath);
    RpLibrary* foundEle = NULL;
    string newNodeName = "";

    cout << "TESTING COPY:" << endl;
    cout << "fromPath = " << fromPath << endl;
    cout << "toPath = " << toPath << endl;

    // put the element into the new library/xmltree
    toLib->copy(toPath,fromPath);

    // check the result
    // create the name of where the element should be
    newNodeName = toPath + "." + fromEle->nodeComp();
    // grab the element
    if ((foundEle = toLib->element(newNodeName))){
        cout << "SUCCESS: foundEle was found" << endl;
        retVal = 0;
    }
    else {
        cout << "FAILURE: foundEle WAS NOT found" << endl;
    }

    return retVal;
}

int test_remove (RpLibrary* lib, string path)
{
    RpLibrary* getRslt = NULL;
    int retVal = 1;
    cout << "TESTING REMOVE:" << endl;
    // cout << "lib's xml:" << endl << lib->xml() << endl;
    if (lib) {
        lib->remove(path);
        if (lib) {
            getRslt = lib->element(path);
            if (getRslt) {
                cout << "FAILURE: " << path << " not removed" << endl;
                cout << "lib's xml:" << endl << lib->xml() << endl;
            }
            else {
                cout << "SUCCESS: " << path << " removed" << endl;
                retVal = 0;
            }
        }
    }

    return retVal;
}

int test_entities (RpLibrary* lib)
{
    list<string>::iterator iter;
    list<string> elist = lib->entities();

    iter = elist.begin();

    cout << "TESTING ENTITIES BEGIN" << endl;
    cout << "path = "<< lib->nodePath() << endl;

    while (iter != elist.end() ) {
        cout << *iter << endl;
        iter++;
    }

    cout << "TESTING ENTITIES END" << endl;

    return 0;
}

int test_diff (RpLibrary* lib1, RpLibrary* lib2)
{
    list<string>::iterator iter;
    list<string> elist;

    elist = lib1->diff(lib2,"input");

    iter = elist.begin();

    cout << "TESTING DIFF BEGIN" << endl;

    int count = 0;
    while (iter != elist.end() ) {
        cout << *iter << " ";
        iter++;
        count++;
        if (count == 4) {
            cout << endl;
            count = 0;
        }

    }

    cout << "TESTING DIFF END" << endl;

    return 0;
}

/*
int test_children (RpLibrary* lib, string path, string type )
{
    int retVal = 1;
    int childNum = -1;

    cout << "TESTING CHILDREN: path = " << path << endl;
    RpLibrary** searchEle = lib->children(path,"",type);

    if (!searchEle || !*searchEle) {
        cout << "searchEle is NULL -> NO CHILDREN" << endl;
        retVal = 1;
    }
    else {

        while (searchEle[++childNum]) {
            cout << "searchEle comp = :" << searchEle[childNum]->nodeComp() 
                << ":" << endl;
            cout << "searchEle   id = :" << searchEle[childNum]->nodeId()   
                << ":" << endl;
            cout << "searchEle type = :" << searchEle[childNum]->nodeType() 
                << ":" << endl;
            delete (searchEle[childNum]);
            searchEle[childNum] = NULL;
        }

        retVal = 0;

    }

    delete[] searchEle;

    return retVal;
}
*/

int test_children (RpLibrary* lib, string path, string type )
{
    int retVal = 1;
    RpLibrary* childEle = NULL;

    cout << "TESTING CHILDREN: path = " << path << endl;

    while ( (childEle = lib->children(path,childEle,type)) ) {

        cout << "childEle path = :" << childEle->nodePath() 
            << ":" << endl;
        cout << "childEle comp = :" << childEle->nodeComp() 
            << ":" << endl;
        cout << "childEle   id = :" << childEle->nodeId()   
            << ":" << endl;
        cout << "childEle type = :" << childEle->nodeType() 
            << ":" << endl;

        retVal = 0;

    }

    if (!childEle) {
        cout << "No more children" << endl;
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

    lib = new RpLibrary(string(argv[1]));

    test_element(lib,"");
    test_element(lib,"input.number(min)");
    test_element(lib,"input.number(max)");
    test_element(lib,"output.curve(result)");

    test_parent(lib,"input.number(min).current");
    test_parent(lib,"input.number(max)");
    test_parent(lib,"output");

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

    lib->put("input.number(test).current", 5);
    test_getInt(lib, "input.number(test).current");
    lib->put("input.number(test).current", 50);
    test_getInt(lib, "input.number(test).current");
    lib->put("input.number(test).current", 53.6);
    test_getInt(lib, "input.number(test).current");
    lib->put("input.number(test).current", 52.94);
    test_getInt(lib, "input.number(test).current");

    lib->put("input.boolean(boolval).current", "yes");
    test_getBool(lib, "input.boolean(boolval).current");
    lib->put("input.boolean(boolval).current", "no");
    test_getBool(lib, "input.boolean(boolval).current");
    lib->put("input.boolean(boolval).current", "true");
    test_getBool(lib, "input.boolean(boolval).current");
    lib->put("input.boolean(boolval).current", "false");
    test_getBool(lib, "input.boolean(boolval).current");
    lib->put("input.boolean(boolval).current", "on");
    test_getBool(lib, "input.boolean(boolval).current");
    lib->put("input.boolean(boolval).current", "off");
    test_getBool(lib, "input.boolean(boolval).current");
    lib->put("input.boolean(boolval).current", "1");
    test_getBool(lib, "input.boolean(boolval).current");
    lib->put("input.boolean(boolval).current", "0");
    test_getBool(lib, "input.boolean(boolval).current");
    lib->put("input.boolean(boolval).current", "ye");
    test_getBool(lib, "input.boolean(boolval).current");
    lib->put("input.boolean(boolval).current", "n");
    test_getBool(lib, "input.boolean(boolval).current");
    lib->put("input.boolean(boolval).current", "tr");
    test_getBool(lib, "input.boolean(boolval).current");
    lib->put("input.boolean(boolval).current", "fa");
    test_getBool(lib, "input.boolean(boolval).current");
    lib->put("input.boolean(boolval).current", "on");
    test_getBool(lib, "input.boolean(boolval).current");
    lib->put("input.boolean(boolval).current", "of");
    test_getBool(lib, "input.boolean(boolval).current");

    test_putObj(lib, "input.number(max)", lib, "input.test");
    test_copy(lib, "input.number(min)", lib, "input.test.number(min)");

    test_children(lib,"","");
    test_children(lib,"input.number(test)","");
    test_children(lib,"input","");
    test_children(lib,"input","number");

    cout << lib->xml() << endl;

    RpLibrary* remEle = lib->element("input.test.(min)");
    test_remove(remEle,"");
    remEle->remove();
    test_remove(lib, "input.test.(min)");
    test_remove(lib, "input.test.(max)");
    cout << lib->xml() << endl;

    RpLibrary* libInput = lib->element("input");
    test_entities(lib);
    test_entities(libInput);

    /*
    list<string> l1 = lib->value("input.number(min)");
    list<string> l2 = lib->value("input.number(max)");
    cout << "l1 = " << l1.front() << " " << l1.back() << endl;
    cout << "l2 = " << l2.front() << " " << l2.back() << endl;
    */

    RpLibrary* dlib1 = new RpLibrary(string(argv[1]));
    RpLibrary* dlib2 = new RpLibrary(string(argv[1]));
    test_diff(dlib1,dlib2);
    cout << "dlib1-------------------------------------" << endl;
    cout << dlib1->xml() << endl;
    cout << "dlib2-------------------------------------" << endl;
    cout << dlib1->xml() << endl;
    dlib1 = lib;
    test_diff(dlib1,dlib2);
    cout << "dlib1-------------------------------------" << endl;
    cout << dlib1->xml() << endl;
    cout << "dlib2-------------------------------------" << endl;
    cout << dlib2->xml() << endl;



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


    cout << "//////////////////// LIB 2 ////////////////////" << endl;
    cout << lib2.xml() << endl;
    cout << "//////////////////// LIB 3 ////////////////////" << endl;
    cout << lib3.xml() << endl;

    lib2.result();


    cout << "testing with &lt;number&gt;" << endl;
    lib2.put("input.dsk.test", "slkdjfs slkdfj lks &lt;number&gt; sdlkfj sdlkjf","",1);
    cout << lib2.xml() << endl;
    cout << "testing get &lt;number&gt;" << endl;
    cout << lib2.get("input.dsk.test") << endl;
    cout << "testing with <number>" << endl;
    lib2.put("input.dsk.test2", "slkdjfs slkdfj lks <number> sdlkfj sdlkjf","",1);
    cout << lib2.xml() << endl;

    /*
    RpLibrary* childEle  = NULL;
    RpLibrary* childEle2 = NULL;
    while ( (childEle = lib2.children("input",childEle)) ) {
        cout << "childEle path = :" << childEle->nodePath() << ":" << endl;
        while ( (childEle2 = childEle->children("",childEle2)) ) {
            cout << "childEle2 path = :" << childEle2->nodePath() << ":" << endl;
        }
        childEle2 = NULL;
        if (!childEle) {
            cout << "childEle Not null" << endl;
        }
    }
    */

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
