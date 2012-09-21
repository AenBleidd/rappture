/*
 * ----------------------------------------------------------------------
 *  INTERFACE: C Rappture Library Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpLibrary.h"
#include "RpLibraryCInterface.h"
#include "RpBufferCHelper.h"

#ifdef __cplusplus
extern "C" {
#endif

RpLibrary*
rpLibrary (const char* path)
{
    if (path == NULL) {
        return (new RpLibrary());
    }

    return (new RpLibrary(path));
}

int
rpFreeLibrary (RpLibrary** lib)
{
    int retVal = 1;

    if (lib && (*lib)) {
        delete (*lib);
        (*lib) = NULL;
        retVal =  0;
    }

    return retVal;
}

RpLibrary*
rpElement (RpLibrary* lib, const char* path)
{
    return (lib->element(path));
}

RpLibrary*
rpElementAsObject (RpLibrary* lib, const char* path)
{
    return rpElement(lib,path);
}


int
rpElementAsType (RpLibrary* lib, const char* path, const char** retCStr)
{
    int retVal = 1;
    static std::string retStr = "";
    RpLibrary* retLib = lib->element(path);

    if (retLib) {
        retStr = retLib->nodeType();
        // check to see if retStr is empty
        retVal = 0;
    }

    *retCStr = retStr.c_str();

    return retVal;
}

int
rpElementAsComp (RpLibrary* lib, const char* path, const char** retCStr)
{
    int retVal = 1;
    static std::string retStr = "";
    RpLibrary* retLib = lib->element(path);

    if (retLib) {
        retStr = retLib->nodeComp();
        // check to see if retStr is empty
        retVal = 0;
    }

    *retCStr = retStr.c_str();
    return retVal;
}

int
rpElementAsId (RpLibrary* lib, const char* path, const char** retCStr)
{
    int retVal = 1;
    static std::string retStr = "";
    RpLibrary* retLib = lib->element(path);

    if (retLib) {
        retStr = retLib->nodeId();
        // check to see if retStr is empty
        retVal = 0;
    }

    *retCStr = retStr.c_str();
    return retVal;
}

RpLibrary*
rpChildren (RpLibrary* lib, const char* path, RpLibrary* childEle )
{
    return ( lib->children(path,childEle) );
}

RpLibrary*
rpChildrenByType( RpLibrary* lib,
                const char* path,
                RpLibrary* childEle,
                const char* type )
{
    return ( lib->children(path,childEle,type) );
}

int
rpGet (RpLibrary* lib, const char* path, const char** retCStr)
{
    return rpGetString(lib,path,retCStr);
}

int
rpGetString (RpLibrary* lib, const char* path, const char** retCStr)
{
    int retVal = 0;
    static std::string retStr = "";

    retStr = lib->getString(path);

    *retCStr = retStr.c_str();
    return retVal;
}

int
rpGetDouble (RpLibrary* lib, const char* path, double* retDVal)
{
    int retVal = 0;
    *retDVal = lib->getDouble(path);
    return retVal;
}

int
rpGetData (RpLibrary* lib, const char* path, RapptureBuffer* retBuf)
{
    Rappture::Buffer rpbuf;
    int retVal = 0;

    if (retBuf == NULL) {
        return -1;
    }

    rpbuf = lib->getData(path);
    RpBufferToCBuffer(&rpbuf, retBuf);
    return retVal;
}

int
rpPut ( RpLibrary* lib,
        const char* path,
        const char* value,
        const char* id,
        unsigned int append )
{
    int retVal = 0;
    lib->put(path,value,id,append);
    return retVal;
}

int
rpPutString (   RpLibrary* lib,
                const char* path,
                const char* value,
                unsigned int append )
{
    int retVal = 0;
    lib->put(path,value,"",append);
    return retVal;
}

int
rpPutDouble (   RpLibrary* lib,
                const char* path,
                double value,
                unsigned int append )
{
    int retVal = 0;
    lib->put(path,value,"",append);
    return retVal;
}

int rpPutData ( RpLibrary* lib,
                const char* path,
                const char* bytes,
                int nBytes,
                unsigned int append )
{
    int retVal = 0;
    lib->putData(path,bytes,nBytes,append);
    return retVal;
}

int rpPutFile ( RpLibrary* lib,
                const char* path,
                const char* fileName,
                unsigned int binary,
                unsigned int append  )
{
    int retVal = 0;
    lib->putFile(path,fileName,binary,append);
    return retVal;
}

int
rpXml (RpLibrary* lib, const char** retCStr)
{
    int retVal = 1;
    static std::string retStr = "";

    if (lib != NULL) {
        retStr = "";
        retStr = lib->xml();

        // string should never be empty after lib->xml fxn call
        // lib->xml returns xml header at the very least.
        if ( !retStr.empty() ) {
            retVal = 0;
        }
    }

    *retCStr = retStr.c_str();
    return retVal;
}

int
rpNodeComp (RpLibrary* node, const char** retCStr)
{
    int retVal = 1;
    static std::string retStr = "";

    if (node != NULL) {
        retStr = "";
        retStr = node->nodeComp();
        if ( !retStr.empty() ) {
            retVal = 0;
        }
        *retCStr = retStr.c_str();
    }

    return retVal;
}

int
rpNodeType (RpLibrary* node, const char** retCStr)
{
    int retVal = 1;
    static std::string retStr = "";

    if (node != NULL) {
        retStr = "";
        retStr = node->nodeType();
        if ( !retStr.empty() ) {
            retVal = 0;
        }
        *retCStr = retStr.c_str();
    }

    return retVal;
}

int
rpNodeId (RpLibrary* node, const char** retCStr)
{
    int retVal = 0;
    static std::string retStr = "";

    if (node != NULL) {
        retStr = "";
        retStr = node->nodeId();
        if ( !retStr.empty() ) {
            retVal = 0;
        }
        *retCStr = retStr.c_str();
    }

    return retVal;
}

int
rpResult (RpLibrary* lib)
{
    int retVal = 0;
    // signal the processing is complete
    lib->put("tool.version.rappture.language", "c");
    lib->result();
    return retVal;
}

#ifdef __cplusplus
}
#endif
