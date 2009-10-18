#include <iostream>
#include "RpPath.h"

int test(
    const char *testname,
    const char *desc,
    const char *expected,
    const char *received)
{
    if ((!expected && received) ||
        (expected && !received) ||
        (expected && received && strcmp(expected,received) != 0)) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", desc);
        printf("\texpected \"%s\"\n",expected);
        printf("\treceived \"%s\"\n",received);
        return 1;
    }
    return 0;
}


int path_0_0 ()
{
    const char *desc = "test creation of path";
    const char *testname = "path_0_0";

    const char *expected = "input.number(mynum).current";
    const char *received = NULL;

    Rappture::Path p(expected);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_1_0 ()
{
    const char *desc = "test component function with type with no id";
    const char *testname = "path_1_0";

    const char *path = "input.group(tabs).number(mynum).current";
    const char *expected = "current";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.component();

    return test(testname,desc,expected,received);
}

int path_1_1 ()
{
    const char *desc = "test component function with type and id";
    const char *testname = "path_1_1";

    const char *path = "input.group(tabs).number(mynum)";
    const char *expected = "number(mynum)";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.component();

    return test(testname,desc,expected,received);
}

int path_1_2 ()
{
    const char *desc = "test component function with no type with id";
    const char *testname = "path_1_2";

    const char *path = "input.group(tabs).(mynum)";
    const char *expected = "(mynum)";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.component();

    return test(testname,desc,expected,received);
}

int path_1_3 ()
{
    const char *desc = "test component function with no type with no id";
    const char *testname = "path_1_3";

    const char *path = "input.group(tabs).";
    const char *expected = "";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.component();

    return test(testname,desc,expected,received);
}

int path_1_4 ()
{
    const char *desc = "test component function with no type with blank id";
    const char *testname = "path_1_4";

    const char *path = "input.group(tabs).()";
    const char *expected = "()";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.component();

    return test(testname,desc,expected,received);
}

int path_2_0 ()
{
    const char *desc = "test id function no id";
    const char *testname = "path_2_0";

    const char *path = "input.group(tabs).number(mynum).current";
    const char *expected = NULL;
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.id();

    return test(testname,desc,expected,received);
}

int path_2_1 ()
{
    const char *desc = "test id function with id";
    const char *testname = "path_2_1";

    const char *path = "input.group(tabs).number(mynum)";
    const char *expected = "mynum";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.id();

    return test(testname,desc,expected,received);
}

int path_2_2 ()
{
    const char *desc = "test id function with blank id";
    const char *testname = "path_2_2";

    const char *path = "input.group(tabs).number()";
    const char *expected = "";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.id();

    return test(testname,desc,expected,received);
}

int path_2_3 ()
{
    const char *desc = "test id function with no type with id";
    const char *testname = "path_2_2";

    const char *path = "input.group(tabs).(mynum)";
    const char *expected = "mynum";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.id();

    return test(testname,desc,expected,received);
}

int path_3_0 ()
{
    const char *desc = "test type function with type with no id";
    const char *testname = "path_3_0";

    const char *path = "input.group(tabs).number(mynum).current";
    const char *expected = "current";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.type();

    return test(testname,desc,expected,received);
}

int path_3_1 ()
{
    const char *desc = "test type function with type with id";
    const char *testname = "path_3_1";

    const char *path = "input.group(tabs).number(mynum)";
    const char *expected = "number";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.type();

    return test(testname,desc,expected,received);
}

int path_3_2 ()
{
    const char *desc = "test type function with no type with id";
    const char *testname = "path_3_2";

    const char *path = "input.group(tabs).(mynum)";
    const char *expected = "";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.type();

    return test(testname,desc,expected,received);
}

int path_3_3 ()
{
    const char *desc = "test type function with no type with no id";
    const char *testname = "path_3_3";

    const char *path = "input.group(tabs).()";
    const char *expected = "";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.type();

    return test(testname,desc,expected,received);
}

int path_4_0 ()
{
    const char *desc = "test parent function, parent with type with id";
    const char *testname = "path_4_0";

    const char *path = "input.number(mynum).current";
    const char *expected = "input.number(mynum)";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.parent();

    return test(testname,desc,expected,received);
}

int path_4_1 ()
{
    const char *desc = "test parent function, parent with type with no id";
    const char *testname = "path_4_1";

    const char *path = "input.number.current";
    const char *expected = "input.number";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.parent();

    return test(testname,desc,expected,received);
}

int path_4_2 ()
{
    const char *desc = "test parent function, parent with no type with id";
    const char *testname = "path_4_2";

    const char *path = "input.(mynum).current";
    const char *expected = "input.(mynum)";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.parent();

    return test(testname,desc,expected,received);
}

int path_4_3 ()
{
    const char *desc = "test parent function, parent with no type with no id";
    const char *testname = "path_4_3";

    const char *path = "input.().current";
    const char *expected = "input.()";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.parent();

    return test(testname,desc,expected,received);
}

int path_4_4 ()
{
    const char *desc = "test parent function, parent of path with one component";
    const char *testname = "path_4_4";

    const char *path = "input";
    const char *expected = "";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.parent();

    return test(testname,desc,expected,received);
}

int path_4_5 ()
{
    const char *desc = "test parent function, parent of empty path";
    const char *testname = "path_4_5";

    const char *path = "";
    const char *expected = "";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.parent();

    return test(testname,desc,expected,received);
}


int path_5_0 ()
{
    const char *desc = "test add function with type with id";
    const char *testname = "path_5_0";

    const char *path = "input.group(tabs)";
    const char *addStr = "number(mynum)";
    const char *expected = "input.group(tabs).number(mynum)";
    const char *received = NULL;

    Rappture::Path p(path);
    p.add(addStr);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_5_1 ()
{
    const char *desc = "test add function with type with no id";
    const char *testname = "path_5_1";

    const char *path = "input.group(tabs)";
    const char *addStr = "number";
    const char *expected = "input.group(tabs).number";
    const char *received = NULL;

    Rappture::Path p(path);
    p.add(addStr);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_5_2 ()
{
    const char *desc = "test add function with type with blank id";
    const char *testname = "path_5_2";

    const char *path = "input.group(tabs)";
    const char *addStr = "number()";
    const char *expected = "input.group(tabs).number()";
    const char *received = NULL;

    Rappture::Path p(path);
    p.add(addStr);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_5_3 ()
{
    const char *desc = "test add function with no type with id";
    const char *testname = "path_5_3";

    const char *path = "input.group(tabs)";
    const char *addStr = "(mynum)";
    const char *expected = "input.group(tabs).(mynum)";
    const char *received = NULL;

    Rappture::Path p(path);
    p.add(addStr);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_5_4 ()
{
    const char *desc = "test add function with no type with blank id";
    const char *testname = "path_5_4";

    const char *path = "input.group(tabs)";
    const char *addStr = "()";
    const char *expected = "input.group(tabs).()";
    const char *received = NULL;

    Rappture::Path p(path);
    p.add(addStr);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_5_5 ()
{
    const char *desc = "test add function, multiple components, type and id";
    const char *testname = "path_5_5";

    const char *path = "input.group(tabs)";
    const char *addStr = "number(mynum).current";
    const char *expected = "input.group(tabs).number(mynum).current";
    const char *received = NULL;

    Rappture::Path p(path);
    p.add(addStr);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_5_6 ()
{
    const char *desc = "test add function, adding to empty path";
    const char *testname = "path_5_6";

    const char *path = "";
    const char *addStr = "input.group(tabs).number(mynum).current";
    const char *expected = "input.group(tabs).number(mynum).current";
    const char *received = NULL;

    Rappture::Path p(path);
    p.add(addStr);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_6_0 ()
{
    const char *desc = "test del function ";
    const char *testname = "path_6_0";

    const char *path = "input.group(tabs).number(mynum).current";
    const char *expected = "input.group(tabs).number(mynum)";
    const char *received = NULL;

    Rappture::Path p(path);
    p.del();
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_6_1 ()
{
    const char *desc = "test del function, del component with type and id ";
    const char *testname = "path_6_1";

    const char *path = "input.group(tabs).number(mynum)";
    const char *expected = "input.group(tabs)";
    const char *received = NULL;

    Rappture::Path p(path);
    p.del();
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_6_2 ()
{
    const char *desc = "test del function, del component with no type with id ";
    const char *testname = "path_6_2";

    const char *path = "input.group(tabs).(mynum)";
    const char *expected = "input.group(tabs)";
    const char *received = NULL;

    Rappture::Path p(path);
    p.del();
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_6_3 ()
{
    const char *desc = "test del function, del component with type with blank id ";
    const char *testname = "path_6_3";

    const char *path = "input.group(tabs).()";
    const char *expected = "input.group(tabs)";
    const char *received = NULL;

    Rappture::Path p(path);
    p.del();
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_6_4 ()
{
    const char *desc = "test del function, del component with blank type no id ";
    const char *testname = "path_6_4";

    const char *path = "input.group(tabs).";
    const char *expected = "input.group(tabs)";
    const char *received = NULL;

    Rappture::Path p(path);
    p.del();
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_6_5 ()
{
    const char *desc = "test del function, del last component from path";
    const char *testname = "path_6_5";

    const char *path = "input";
    const char *expected = "";
    const char *received = NULL;

    Rappture::Path p(path);
    p.del();
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_6_6 ()
{
    const char *desc = "test del function, del from empty path";
    const char *testname = "path_6_6";

    const char *path = "";
    const char *expected = "";
    const char *received = NULL;

    Rappture::Path p(path);
    p.del();
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_7_0 ()
{
    const char *desc = "test ifs function, change ifs to /";
    const char *testname = "path_7_0";

    const char *path = "input.group(tabs).number(mynum).current";
    const char *ifs = "/";
    const char *expected = "input/group(tabs)/number(mynum)/current";
    const char *received = NULL;

    Rappture::Path p(path);
    p.ifs(ifs);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_7_1 ()
{
    const char *desc = "test ifs function, change ifs to \\0";
    const char *testname = "path_7_1";

    const char *path = "input.group(tabs).number(mynum).current";
    const char *ifs = "\0";
    const char *expected = "input.group(tabs).number(mynum).current";
    const char *received = NULL;

    Rappture::Path p(path);
    p.ifs(ifs);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_7_2 ()
{
    const char *desc = "test ifs function, change ifs to (";
    const char *testname = "path_7_2";

    const char *path = "input.group(tabs).number(mynum).current";
    const char *ifs = "(";
    const char *expected = "input.group(tabs).number(mynum).current";
    const char *received = NULL;

    Rappture::Path p(path);
    p.ifs(ifs);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_7_3 ()
{
    const char *desc = "test ifs function, change ifs to (";
    const char *testname = "path_7_3";

    const char *path = "input.group(tabs).number(mynum).current";
    const char *ifs = ")";
    const char *expected = "input.group(tabs).number(mynum).current";
    const char *received = NULL;

    Rappture::Path p(path);
    p.ifs(ifs);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_7_4 ()
{
    const char *desc = "test ifs function, change ifs to NULL";
    const char *testname = "path_7_4";

    const char *path = "input.group(tabs).number(mynum).current";
    const char *ifs = NULL;
    const char *expected = "input.group(tabs).number(mynum).current";
    const char *received = NULL;

    Rappture::Path p(path);
    p.ifs(ifs);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_7_5 ()
{
    const char *desc = "test ifs function, change ifs to .";
    const char *testname = "path_7_5";

    const char *path = "input.group(tabs).number(mynum).current";
    const char *ifs = ".";
    const char *expected = "input.group(tabs).number(mynum).current";
    const char *received = NULL;

    Rappture::Path p(path);
    p.ifs(ifs);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_7_6 ()
{
    const char *desc = "test ifs function, change ifs to # on one component path";
    const char *testname = "path_7_6";

    const char *path = "input";
    const char *ifs = "#";
    const char *expected = "input";
    const char *received = NULL;

    Rappture::Path p(path);
    p.ifs(ifs);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_7_7 ()
{
    const char *desc = "test ifs function, change ifs to $ on empty path";
    const char *testname = "path_7_7";

    const char *path = "";
    const char *ifs = "$";
    const char *expected = "";
    const char *received = NULL;

    Rappture::Path p(path);
    p.ifs(ifs);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_8_0 ()
{
    const char *desc = "test path function, no arguments, empty path";
    const char *testname = "path_8_0";

    const char *path = "";
    const char *expected = "";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_8_1 ()
{
    const char *desc = "test path function, no arguments, with path";
    const char *testname = "path_8_1";

    const char *path = "input.group(tabs).number(mynum).current";
    const char *expected = "input.group(tabs).number(mynum).current";
    const char *received = NULL;

    Rappture::Path p(path);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_8_2 ()
{
    const char *desc = "test path function, empty object, new path argument";
    const char *testname = "path_8_2";

    const char *path = "input.group(tabs).number(mynum).current";
    const char *expected = "input.group(tabs).number(mynum).current";
    const char *received = NULL;

    Rappture::Path p;
    p.path(path);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_8_3 ()
{
    const char *desc = "test path function, populated object, new path argument";
    const char *testname = "path_8_3";

    const char *path1 = "input.group(tabs).number(mynum).current";
    const char *path2 = "input.number(mynum).current";
    const char *expected = "input.number(mynum).current";
    const char *received = NULL;

    Rappture::Path p(path1);
    p.path(path2);
    received = p.path();

    return test(testname,desc,expected,received);
}

int path_8_4 ()
{
    const char *desc = "test path function, populated object, new path fxn argument is empty string";
    const char *testname = "path_8_4";

    const char *path1 = "input.group(tabs).number(mynum).current";
    const char *path2 = "";
    const char *expected = "";
    const char *received = NULL;

    Rappture::Path p(path1);
    p.path(path2);
    received = p.path();

    return test(testname,desc,expected,received);
}

// FIXME: add test for constructor
// FIXME: add test for destructor
// FIXME: add test for component get/set functions
// FIXME: add test for id get/set functions
// FIXME: add test for type get/set functions
// FIXME: add test for parent get/set functions
// FIXME: add test for path get/set functions
// FIXME: add test for changing the curr location
//        using first,last,next,prev and checking
//        comp, type, parent,id fxns
// FIXME: add test for numbered paths
//        input.number2
//        input.number2(eee)
//        input.number(eee)2
// FIXME: add test for _ifs inside of id tags

int main()
{
    path_0_0();
    path_1_0();
    path_1_1();
    path_1_2();
    path_1_3();
    path_1_4();
    path_2_0();
    path_2_1();
    path_2_2();
    path_2_3();
    path_3_0();
    path_3_1();
    path_3_2();
    path_3_3();
    path_4_0();
    path_4_1();
    path_4_2();
    path_4_3();
    path_4_4();
    path_4_5();
    path_5_0();
    path_5_1();
    path_5_2();
    path_5_3();
    path_5_4();
    path_5_5();
    path_5_6();
    path_6_0();
    path_6_1();
    path_6_2();
    path_6_3();
    path_6_4();
    path_6_5();
    path_6_6();
    path_7_0();
    path_7_1();
    path_7_2();
    path_7_3();
    path_7_4();
    path_7_5();
    path_7_6();
    path_7_7();
    path_8_0();
    path_8_1();
    path_8_2();
    path_8_3();
    path_8_4();

    return 0;
}
