/*
 * ----------------------------------------------------------------------
 *  INTERFACE: C Rappture Library Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2005  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpLibrary.h"
#include "RpLibraryCInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

    RpLibrary*
    rpLibrary (const char* path)
    {
        return new RpLibrary(path);
    }

    void
    rpFreeLibrary (RpLibrary** lib)
    {
        if (lib && (*lib)) {
            delete (*lib);
            (*lib) = NULL;
        }
    }

    RpLibrary*
    rpElement (RpLibrary* lib, const char* path)
    {
        return lib->element(path);
    }

    RpLibrary*
    rpElementAsObject (RpLibrary* lib, const char* path)
    {
        return rpElement(lib,path);
    }

    const char*
    rpElementAsType (RpLibrary* lib, const char* path)
    {
        static std::string retStr = "";
        RpLibrary* retLib = lib->element(path);

        if (retLib) {
            retStr = retLib->nodeType();
        }

        return retStr.c_str();
    }

    const char*
    rpElementAsComp (RpLibrary* lib, const char* path)
    {
        static std::string retStr = "";
        RpLibrary* retLib = lib->element(path);

        if (retLib) {
            retStr = retLib->nodeComp();
        }

        return retStr.c_str();
    }

    const char*
    rpElementAsId (RpLibrary* lib, const char* path)
    {
        static std::string retStr = "";
        RpLibrary* retLib = lib->element(path);

        if (retLib) {
            retStr = retLib->nodeId();
        }

        return retStr.c_str();
    }


    RpLibrary*
    rpChildren (RpLibrary* lib, const char* path, RpLibrary* childEle   )
    {
        return lib->children(path,childEle);
    }

    RpLibrary*
    rpChildrenByType( RpLibrary* lib,
                    const char* path,
                    RpLibrary* childEle,
                    const char* type    )
    {
        return lib->children(path,childEle,type);
    }

    const char*
    rpGet (RpLibrary* lib, const char* path)
    {
        static std::string retStr = "";
        retStr = lib->getString(path);
        return retStr.c_str();
    }

    const char*
    rpGetString (RpLibrary* lib, const char* path)
    {
        static std::string retStr = "";
        retStr = lib->getString(path);
        return retStr.c_str();
    }

    double
    rpGetDouble (RpLibrary* lib, const char* path)
    {
        return lib->getDouble(path);
    }

    void
    rpPut         (RpLibrary* lib,
                 const char* path,
                 const char* value,
                 const char* id,
                 int append         )
    {
        lib->put(path,value,id,append);
    }

    void
    rpPutStringId (RpLibrary* lib,
                 const char* path,
                 const char* value,
                 const char* id,
                 int append          )
    {
        lib->put(path,value,id,append);
    }

    void
    rpPutString ( RpLibrary* lib,
                const char* path,
                const char* value,
                int append          )
    {
        lib->put(path,value,"",append);
    }

    void
    rpPutDoubleId (RpLibrary* lib,
                 const char* path,
                 double value,
                 const char* id,
                 int append         )
    {
        lib->put(path,value,id,append);
    }

    void
    rpPutDouble   (RpLibrary* lib,
                 const char* path,
                 double value,
                 int append         )
    {
        lib->put(path,value,"",append);
    }

    const char*
    rpXml (RpLibrary* lib)
    {
        static std::string retStr = "";
        retStr = lib->xml();
        return retStr.c_str();
    }

    const char*
    rpNodeComp (RpLibrary* node)
    {
        static std::string retStr = "";
        retStr = node->nodeComp();
        return retStr.c_str();
    }

    const char*
    rpNodeType (RpLibrary* node)
    {
        static std::string retStr = "";
        retStr = node->nodeType();
        return retStr.c_str();
    }

    const char*
    rpNodeId (RpLibrary* node)
    {
        static std::string retStr = "";
        retStr = node->nodeId();
        return retStr.c_str();
    }

    void
    rpResult (RpLibrary* lib)
    {
        // signal the processing is complete
        lib->result();
    }

#ifdef __cplusplus
}
#endif
