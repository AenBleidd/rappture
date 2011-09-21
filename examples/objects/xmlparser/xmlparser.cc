#include <iostream>
#include <errno.h>
#include <fstream>
#include <sys/stat.h>
#include "RpParserXML.h"

size_t
readFile (
    const char *filePath,
    const char **buf)
{
    if (buf == NULL) {
        fprintf(stderr,"buf is NULL while opening file \"%s\"", filePath);
        return 0;
    }

    FILE *f;
    f = fopen(filePath, "rb");
    if (f == NULL) {
        fprintf(stderr,"can't open \"%s\": %s", filePath, strerror(errno));
        return 0;
    }
    struct stat stat;
    if (fstat(fileno(f), &stat) < 0) {
        fprintf(stderr,"can't stat \"%s\": %s", filePath, strerror(errno));
        return 0;
    }
    off_t size;
    size = stat.st_size;
    char* memblock;
    memblock = new char [size+1];
    if (memblock == NULL) {
        fprintf(stderr,"can't allocate %zu bytes for file \"%s\": %s",
            (size_t) size, filePath, strerror(errno));
        fclose(f);
        return 0;
    }

    size_t nRead;
    nRead = fread(memblock, sizeof(char), size, f);
    fclose(f);

    if (nRead != (size_t)size) {
        fprintf(stderr,"can't read %zu bytes from \"%s\": %s", (size_t) size, filePath,
            strerror(errno));
        return 0;
    }

    memblock[size] = '\0';
    *buf = memblock;
    return nRead;
}

int xmlparser_0_0 ()
{
    const char *desc = "test creating xml parser";
    const char *testname = "xmlparser_0_0";
    int retVal = 0;

    const char *expected = "initialized ParserXml Object";
    const char *received = NULL;

    Rp_ParserXml *p = NULL;

    p = Rp_ParserXmlCreate();

    if (p == NULL) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", desc);
        printf("\texpected \"%s\"\n",expected);
        printf("\treceived \"%s\"\n",received);
        retVal = 1;
    }

    if (p) {
        Rp_ParserXmlDestroy(&p);
    }

    return retVal;
}

int xmlparser_1_0 ()
{
    const char *desc = "test sending xml text to xml parser";
    const char *testname = "xmlparser_1_0";
    const char *infile = "xmlparser_1_0_in.xml";
    const char *outfile = "xmlparser_1_0_out.xml";
    int retVal = 0;

    const char *xmltext = NULL;
    const char *expected = NULL;

    readFile(infile,&xmltext);
    readFile(outfile,&expected);

    const char *received = NULL;

    Rp_ParserXml *p = NULL;

    p = Rp_ParserXmlCreate();

    Rp_ParserXmlParse(p, xmltext);

    received = Rp_ParserXmlXml(p);


    if (strcmp(expected,received) != 0) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", desc);
        printf("\texpected %zu bytes: \"%s\"\n",strlen(expected), expected);
        printf("\treceived %zu bytes: \"%s\"\n",strlen(received), received);
        retVal = 1;
    }

    if (p) {
        Rp_ParserXmlDestroy(&p);
    }

    if (xmltext != NULL) {
        delete [] xmltext;
        xmltext = NULL;
    }
    if (expected != NULL) {
        delete [] expected;
        expected = NULL;
    }

    return retVal;
}


int xmlparser_1_1 ()
{
    const char *desc = "test sending xml text to xml parser";
    const char *testname = "xmlparser_1_1";
    const char *infile = "xmlparser_1_1_in.xml";
    const char *outfile = "xmlparser_1_1_out.xml";
    int retVal = 0;

    const char *xmltext = NULL;
    const char *expected = NULL;

    readFile(infile,&xmltext);
    readFile(outfile,&expected);

    const char *received = NULL;

    Rp_ParserXml *p = NULL;

    p = Rp_ParserXmlCreate();

    Rp_ParserXmlParse(p, xmltext);

    received = Rp_ParserXmlXml(p);


    if (strcmp(expected,received) != 0) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", desc);
        printf("\texpected %zu bytes: \"%s\"\n",strlen(expected), expected);
        printf("\treceived %zu bytes: \"%s\"\n",strlen(received), received);
        retVal = 1;
    }

    if (p) {
        Rp_ParserXmlDestroy(&p);
    }

    if (xmltext) {
        delete[] xmltext;
        xmltext = NULL;
    }
    if (expected) {
        delete[] expected;
        expected = NULL;
    }

    return retVal;
}


int xmlparser_2_0 ()
{
    const char *desc = "test printing xml as path/val combos";
    const char *testname = "xmlparser_2_0";
    const char *infile = "xmlparser_2_0_in.xml";
    int retVal = 0;

    const char *xmltext = NULL;

    readFile(infile,&xmltext);

    const char *expected = "\
run.tool.about Press Simulate to view results.\n\
run.tool.command tclsh @tool/fermi.tcl @driver\n\
run.input.number(Ef).about.label Fermi Level\n\
run.input.number(Ef).about.description Energy at center of distribution.\n\
run.input.number(Ef).units eV\n\
run.input.number(Ef).min -10eV\n\
run.input.number(Ef).max 10eV\n\
run.input.number(Ef).default 0eV\n\
run.input.number(Ef).current 0eV\n";

    const char *received = NULL;

    Rp_ParserXml *p = NULL;

    p = Rp_ParserXmlCreate();

    Rp_ParserXmlParse(p, xmltext);

    received = Rp_ParserXmlPathVal(p);


    if (strcmp(expected,received) != 0) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", desc);
        printf("\texpected %zu bytes: \"%s\"\n",strlen(expected), expected);
        printf("\treceived %zu bytes: \"%s\"\n",strlen(received), received);
        retVal = 1;
    }

    if (p) {
        Rp_ParserXmlDestroy(&p);
    }

    if (xmltext) {
        delete[] xmltext;
        xmltext = NULL;
    }

    return retVal;
}

int xmlparser_3_0 ()
{
    const char *desc = "test getting xml value";
    const char *testname = "xmlparser_3_0";
    const char *infile = "xmlparser_2_0_in.xml";
    int retVal = 0;

    const char *xmltext = NULL;

    readFile(infile,&xmltext);

    const char *expected = "0eV";

    const char *received = NULL;

    Rp_ParserXml *p = NULL;

    p = Rp_ParserXmlCreate();

    Rp_ParserXmlParse(p, xmltext);

    received = Rp_ParserXmlGet(p,"input.number(Ef).current");


    if (strcmp(expected,received) != 0) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", desc);
        printf("\texpected %zu bytes: \"%s\"\n",strlen(expected), expected);
        printf("\treceived %zu bytes: \"%s\"\n",strlen(received), received);
        retVal = 1;
    }

    if (p) {
        Rp_ParserXmlDestroy(&p);
    }

    if (xmltext) {
        delete[] xmltext;
        xmltext = NULL;
    }

    return retVal;
}

int xmlparser_4_0 ()
{
    const char *desc = "test putting xml value";
    const char *testname = "xmlparser_4_0";
    const char *infile = "xmlparser_2_0_in.xml";
    int retVal = 0;

    const char *xmltext = NULL;

    readFile(infile,&xmltext);

    const char *expected = "hi mom";

    const char *received = NULL;

    Rp_ParserXml *p = NULL;

    p = Rp_ParserXmlCreate();

    Rp_ParserXmlParse(p, xmltext);

    Rp_ParserXmlPut(p,"input.number(gg).current","hi mom",0);
    received = Rp_ParserXmlGet(p,"input.number(gg).current");

    if (strcmp(expected,received) != 0) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", desc);
        printf("\texpected %zu bytes: \"%s\"\n",strlen(expected), expected);
        printf("\treceived %zu bytes: \"%s\"\n",strlen(received), received);
        retVal = 1;
    }

    if (p) {
        Rp_ParserXmlDestroy(&p);
    }

    if (xmltext) {
        delete[] xmltext;
        xmltext = NULL;
    }

    return retVal;
}

int xmlparser_5_0 ()
{
    const char *desc = "test printing xml with degrees of presets";
    const char *testname = "xmlparser_5_0";
    const char *infile = "xmlparser_5_0_in.xml";
    int retVal = 0;

    const char *xmltext = NULL;

    readFile(infile,&xmltext);

    const char *expected = "\
run.tool.about Press Simulate to view results.\n\
run.tool.command tclsh @tool/fermi.tcl @driver\n\
run.input.number(Ef).about.label Fermi Level\n\
run.input.number(Ef).about.description Energy at center of distribution.\n\
run.input.number(Ef).units eV\n\
run.input.number(Ef).min -10eV\n\
run.input.number(Ef).max 10eV\n\
run.input.number(Ef).default 0eV\n\
run.input.number(Ef).current 0eV\n\
run.input.number(Ef).preset.value 300K\n\
run.input.number(Ef).preset.label 300K (room temperature)\n\
run.input.number(Ef).preset.value 77K\n\
run.input.number(Ef).preset.label 77K (liquid nitrogen)\n\
run.input.number(Ef).preset.value 4.2K\n\
run.input.number(Ef).preset.label 4.2K (liquid helium)\n\
";

    const char *received = NULL;

    Rp_ParserXml *p = NULL;

    p = Rp_ParserXmlCreate();

    Rp_ParserXmlParse(p, xmltext);

    received = Rp_ParserXmlPathVal(p);


    if (strcmp(expected,received) != 0) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", desc);
        printf("\texpected %zu bytes: \"%s\"\n",strlen(expected), expected);
        printf("\treceived %zu bytes: \"%s\"\n",strlen(received), received);
        retVal = 1;
    }

    if (p) {
        Rp_ParserXmlDestroy(&p);
    }

    if (xmltext) {
        delete[] xmltext;
        xmltext = NULL;
    }

    return retVal;
}

// FIXME: test what happens when parser sees self closing tag <tag/>
// FIXME: test get function
// FIXME: test put function
// FIXME: test putf function
// FIXME: test appendf function
// FIXME: look into why Rp_ParserXmlPathVal hits some nodes twice in gdb

int main()
{
    xmlparser_0_0();
    xmlparser_1_0();
    xmlparser_1_1();
    xmlparser_2_0();
    xmlparser_3_0();
    xmlparser_4_0();
    xmlparser_5_0();

    return 0;
}
