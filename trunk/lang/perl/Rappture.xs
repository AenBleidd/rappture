#include "rappture.h"
#include <string>

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

using namespace std;

MODULE = Rappture        PACKAGE = Rappture::RpLibrary
PROTOTYPES: ENABLE

RpLibrary *
RpLibrary::new(filename = "")
const char *filename
    CODE:
        RpLibrary *library;
        if ((filename == NULL) || (*filename == '\0'))
            library = new RpLibrary();
        else
            library = new RpLibrary(filename);

        if (library->isNull())
                {
                        delete library;
            XSRETURN_UNDEF;
                }
        else
            RETVAL = library;
    OUTPUT:
        RETVAL

void *
RpLibrary::DESTROY()
    CODE:
        RETVAL = 0;
    OUTPUT:
	RETVAL

SV *
RpLibrary::get( path )
const char *path
    CODE:
        string result;
        result = THIS->get(path);
        RETVAL = newSVpvn(result.data(),result.length());
    OUTPUT:
        RETVAL

void
RpLibrary::put( path, value, append )
const char *path
const char *value
int append
    CODE:
        THIS->put(path,value,"",append);

void
RpLibrary::putFile( path, fileName, compress, append )
const char *path
const char *fileName
int compress
int append
    CODE:
        THIS->putFile(path,fileName,compress,append);

void
RpLibrary::result()
    CODE:
        THIS->put("tool.version.rappture.language", "perl");
        THIS->result();

MODULE = Rappture        PACKAGE = Rappture::RpUnits

const char *
convert( fromVal, toUnitsName, showUnits = 1 )
const char *fromVal
const char *toUnitsName
int showUnits
    CODE:
        string result;
        result = RpUnits::convert(fromVal,toUnitsName,showUnits);

                if (result.empty())
                    XSRETURN_UNDEF;

        RETVAL = result.c_str();
    OUTPUT:
        RETVAL

MODULE = Rappture        PACKAGE = Rappture::Utils

int
progress( percent, message )
int percent
const char *message
    CODE:
        RETVAL = Rappture::Utils::progress(percent,message);
    OUTPUT:
        RETVAL
