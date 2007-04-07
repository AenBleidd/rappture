#include "rappture.h"
#include <string>

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

using namespace std;

MODULE = Rappture		PACKAGE = Rappture::RpLibrary
PROTOTYPES: ENABLE

RpLibrary *
RpLibrary::new(filename = "")
char *filename
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

const char *
RpLibrary::get( path )
char *path
	CODE:
		string result;
		result = THIS->get(path);
		RETVAL = result.c_str();
	OUTPUT:
		RETVAL

void
RpLibrary::put( path, value, append )
char *path
char *value
int append
	CODE:
		THIS->put(path,value,"",append);

void
RpLibrary::putFile( path, fileName, compress, append )
char *path
char *fileName
int compress
int append
	CODE:
		THIS->putFile(path,fileName,compress,append);

void
RpLibrary::result()

MODULE = Rappture		PACKAGE = Rappture::RpUnits

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

MODULE = Rappture		PACKAGE = Rappture::Utils

int
progress( percent, message )
int percent
const char *message
	CODE:
		RETVAL = Rappture::Utils::progress(percent,message);
	OUTPUT:
		RETVAL
