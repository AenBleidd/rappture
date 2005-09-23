#include "rappture_interface.h"
#include <stdlib.h>
#include <time.h>

//
// write result xml file out
// put in directive to cause "run" to read and plot using run.xml
//
// return val:    0: error
//		> 0: success
//
//
int myOutputResult(PyObject* lib)
{
	char outputFile[50];
        char* xtext = NULL;
	FILE* fp = NULL;

	xtext = rpXml(lib);
	if (!xtext) {
		printf("rpXml error\n");
		return 0;
	}

	// create output filename
	

	time_t t;

	snprintf(outputFile, 50, "run%d.xml", (int)time(&t));
	if(!(fp = fopen(outputFile, "w"))) {
		printf("output file open error (err=%d)\n", errno);
		return 0;
	}

	int fsize = fwrite(xtext, strlen(xtext), 1, fp);
	if(DEBUG)
		printf("output file size=%d\n", fsize);
	fclose(fp);

	printf("=RAPPTURE-RUN=>%s\n", outputFile);

	return 1;
}

int main(int argc, char * argv[])
{

    PyObject* rpClass = NULL;
    PyObject* lib = NULL;

    //char filePath[100];
    char *filePath;
    char* xmltext = NULL;
    double fmin, fmax;
    char strFormula[100];
    int i;

    memset(strFormula, '\0', 100);

    filePath = argv[1];

    if (DEBUG)
        printf("filePath: %s:\n", filePath);

    // initialize the interpreter
    Py_Initialize();
    if(DEBUG)
        printf("py_init successful\n");

    // import the rappture library
    if(!(rpClass = importRappture())) {
	    printf("importRappture returns NULL\n");
	    exit(1);
    }

    if(!(lib = createRapptureObj(rpClass, filePath))) {
	    printf("createRapptureObj returns NULL\n");
	    exit(1);
	}

    Py_DECREF(rpClass);

    if(DEBUG)
    	printf("createRapptureObj successful\n");

    if((xmltext = (char*)rpXml(lib))) {
        if(DEBUG) {
	    //printf("XML file content:\n");
	    //printf("%s\n", xmltext);
	}
    }
    else {
	    printf("rpXml(lib) failed\n");
	    exit(1);
    }

    // get the min
    if (!(xmltext = (char *) rpGet(lib, "input.number(min).current"))) {
	    printf("rpGet(input.number(xmin).current) returns null\n");
	    exit(1);
    }
    if(DEBUG)
        printf("xml min: %s: len=%d\n", xmltext, strlen(xmltext));
    fmin = atof((char*)xmltext);
    if(DEBUG)
        printf("min: %f\n", fmin);

    // get the max
    if (!(xmltext = (char *) rpGet(lib, "input.number(max).current"))) {
	    printf("rpGet(input.number(xmax).current) returns null\n");
	    exit(1);
    }
    if(DEBUG)
    	printf("xml max: %s: len=%d\n", xmltext, strlen(xmltext));
    fmax = atof((char*)xmltext);
    if(DEBUG)
    	printf("max: %f\n", fmax);

    // evaluate formula and generate results
    double fx, fy;
    int npts = 100;
    char str[50];
    for (i = 0; i<npts; i++) {
	    fx = i* (fmax - fmin)/npts + fmin;
	    fy = sin(fx);
	    sprintf(str, "%f %f\n", fx, fy);
	    rpPut(lib, "output.curve.component.xy", str, "result", 1);
    }
    

    // output
    myOutputResult(lib);

    Py_DECREF(lib);
    
    // shut down the interpreter
    Py_Finalize();
    return 0;
}
