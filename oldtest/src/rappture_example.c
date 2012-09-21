/*
 * ======================================================================
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "../include/rappture_interface.h"

/* fix buffer overflow possibility*/
char* null_terminate(char* inStr, int len)
{
    int retVal = 0;
    char* newStr = NULL;
    char* current = NULL;

    if (inStr) {

        current = inStr+len-1;

        while ((len > 0) && (isspace(*(current)))) {
            // dont strip off newlines

            if ( (*(current) == '\f')
              || (*(current) == '\n')
              || (*(current) == '\r')
              || (*(current) == '\t')
              || (*(current) == '\v') )
            {
                break;
            }

            if (--len) {
                current--;
            }
        }

        newStr = (char*) calloc(len,(sizeof(char)));
        strncpy(newStr,inStr,len);
        *(newStr+len) = '\0';

        retVal++;
    }

    // return retVal;

    return newStr;
}


int main(int argc, char * argv[])
{

    PyObject* rpClass = NULL;
    PyObject* lib = NULL;

    // char* filePath = "/home/derrick/playground/rappture_fortran/xml_files/driver_big.xml";
    // char* filePath = NULL; // "/home/derrick/playground/rappture_fortran/xml_files/driver_big.xml";
    const char* xmlText = NULL;

    // filePath = (char*) calloc(67,sizeof(char));

    char* filePath2 = "/home/derrick/playground/rappture_fortran/xml_files/driver_big.xml     ";
    char* filePath = null_terminate(filePath2,70);
//    char* filePath = "/home/derrick/playground/rappture_fortran/xml_files/driver_small.xml";

    printf("filePath = :%s:\n",filePath);

    //scanf("%66c", filePath );
    // scanf("%70c", filePath );
    
    // initialize the interpreter
    Py_Initialize();

    rpClass = importRappture();
    if (rpClass) {
        printf("created rappture object\n");
        lib = createRapptureObj(rpClass, filePath);
        Py_DECREF(rpClass);

        if (lib) {

            // test lib.xml()
            printf("test lib.xml()\n");
            
            xmlText = rpXml(lib);
            if (xmlText) {
                printf("\nXML FILE\n\n%s\n\n",xmlText);
            }

            printf("\n");

            // test lib.get("...")
            xmlText = rpGet(lib, "input.(ambient).(temperature).current");
            if (xmlText) {
                printf("xmlText = %s\n", xmlText);
            }


            Py_DECREF(lib);
        }
    }
    
    // shut down the interpreter
    Py_Finalize();
    return 0;
}

/*
 *
python notes

>>> print lib.xml()
<?xml version="1.0" ?>
<nanohub xmlns="http://www.nanohub.org" xmlns:class="http://www.nanohub.org">
  <purdue>
    <class class:number="101" class:school="ECE">
      <name>Nanotechnology 101</name>
      <desc>Introduction to Nanotechnology</desc>
    </class>
  <class id="NANO512"><name>Nanotechnology 512</name><desc>Not so intro to Nanotech</desc></class></purdue>
</nanohub>
>>> lib.children('purdue','component')
[u'class', 'class(NANO512)']
>>> lib.children('purdue','type')
[u'class', 'class']
>>> lib.children('purdue','id')
[u'class', 'NANO512']
>>> lib.children('purdue','object')
[<Rappture.library.library instance at 0xb7d2b32c>, <Rappture.library.library instance at 0xb7d2b42c>]
>>> lib.children('class', 'component')
[u'name', u'desc']
>>> lib.children('name', 'component')
[]
>>> lib.children('desc', 'component')
[]
>>> 
>>> 
>>> lib.element('purdue','component')
u'purdue'
>>> lib.element('class','component')
u'class'
>>> lib.element('class(NANO512)','component')
'class(NANO512)'
>>> 
>>> lib.element('purdue','type')
u'purdue'
>>> lib.element('class','type')
u'class'
>>> lib.element('class(NANO512)','type')
'class'
>>> 
>>> lib.element('purdue','id')
u'purdue'
>>> lib.element('class','id')
u'class'
>>> lib.element('class(NANO512)','id')
'NANO512'
>>> lib.get('purdue.class(NANO512).name')
'Nanotechnology 512'
>>> lib.get('purdue.class.name')
u'Nanotechnology 101'
>>> lib.put('uiuc.class(NANO500).name','Nanotechnology 500')
>>> lib.put('uiuc.class(NANO500).desc','Carbon NanoTubes')
>>> print lib.xml()
<?xml version="1.0" ?>
<nanohub xmlns="http://www.nanohub.org" xmlns:class="http://www.nanohub.org">
  <purdue>
    <class class:number="101" class:school="ECE">
      <name>Nanotechnology 101</name>
      <desc>Introduction to Nanotechnology</desc>
    </class>
  <class id="NANO512"><name>Nanotechnology 512</name><desc>Not so intro to Nanotech</desc></class></purdue>
<uiuc><class id="NANO500"><name>Nanotechnology 500</name><desc>Carbon NanoTubes</desc></class></uiuc></nanohub>
>>> nano500 = lib.element('class(NANO500)')
>>> nano500.get('name')
'Nanotechnology 500'
>>> lib.put('sdsc',nano500)
>>> print lib.xml()
<?xml version="1.0" ?>
<nanohub xmlns="http://www.nanohub.org" xmlns:class="http://www.nanohub.org">
  <purdue>
    <class class:number="101" class:school="ECE">
      <name>Nanotechnology 101</name>
      <desc>Introduction to Nanotechnology</desc>
    </class>
    <class id="NANO512">
      <name>Nanotechnology 512</name>
      <desc>Not so intro to Nanotech</desc>
    </class>
  </purdue>
  <uiuc/>
  <sdsc>
    <class id="NANO500">
      <name>Nanotechnology 500</name>
      <desc>hi</desc>
    </class>
  </sdsc>
</nanohub>
>>> 
>>> lib.put('uw',nano500)
>>> print lib.xml()
<?xml version="1.0" ?>
<nanohub xmlns="http://www.nanohub.org" xmlns:class="http://www.nanohub.org">
  <purdue>
    <class class:number="101" class:school="ECE">
      <name>Nanotechnology 101</name>
      <desc>Introduction to Nanotechnology</desc>
    </class>
    <class id="NANO512">
      <name>Nanotechnology 512</name>
      <desc>Not so intro to Nanotech</desc>
    </class>
  </purdue>
  <uiuc/>
  <sdsc/>
  <uw>
    <class id="NANO500">
      <name>Nanotechnology 500</name>
      <desc>hi</desc>
    </class>
  </uw>
</nanohub>
>>> 


*/


