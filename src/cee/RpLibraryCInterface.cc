#include "RpLibrary.h"
#include "RpLibraryCInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

    RpLibrary* 
    library (const char* path)
    {
        return new RpLibrary(path);
    }

    void
    freeLibrary (RpLibrary* lib)
    {
        delete lib;
        lib = NULL;
    }

    RpLibrary*
    element (RpLibrary* lib, const char* path)
    {
        return lib->element(path);
    }
    
    RpLibrary*  
    elementAsObject (RpLibrary* lib, const char* path)
    {
        return element(lib,path);
    }

    const char* 
    elementAsType (RpLibrary* lib, const char* path)
    {
        static std::string retStr = "";
        RpLibrary* retLib = lib->element(path);

        if (retLib) {
            retStr = retLib->nodeType();
        }

        return retStr.c_str();
    }

    const char* 
    elementAsComp (RpLibrary* lib, const char* path)
    {
        static std::string retStr = "";
        RpLibrary* retLib = lib->element(path);

        if (retLib) {
            retStr = retLib->nodeComp();
        }

        return retStr.c_str();
    }

    const char* 
    elementAsId (RpLibrary* lib, const char* path)
    {
        static std::string retStr = "";
        RpLibrary* retLib = lib->element(path);

        if (retLib) {
            retStr = retLib->nodeId();
        }

        return retStr.c_str();
    }


    RpLibrary*
    children (RpLibrary* lib, const char* path, RpLibrary* childEle   )
    {
        return lib->children(path,childEle);
    }

    RpLibrary*
    childrenByType( RpLibrary* lib, 
                    const char* path, 
                    RpLibrary* childEle, 
                    const char* type    )
    {
        return lib->children(path,childEle,type);
    }

    /*
    RpLibrary** 
    childrenAsObject (RpLibrary* lib, const char* path, const char* type) 
    {
        return chilren(lib, path,type);
    }

    const char* 
    childrenAsType (RpLibrary* lib, const char* path, const char* type)
    {

    }

    const char* 
    childrenAsComp (RpLibrary* lib, const char* path, const char* type)
    {

    }

    const char* 
    childrenAsId (RpLibrary* lib, const char* path, const char* type)
    {

    }
    */

    RpLibrary*  
    get (RpLibrary* lib, const char* path)
    {
        return lib->get(path);
    }

    const char* 
    getString (RpLibrary* lib, const char* path)
    {
        static std::string retStr = "";
        retStr = lib->getString(path);
        return retStr.c_str();
    }

    double 
    getDouble (RpLibrary* lib, const char* path)
    {
        return lib->getDouble(path);
    }

    void 
    put         (RpLibrary* lib, 
                 const char* path, 
                 const char* value,
                 const char* id,
                 int append         )
    {
        lib->put(path,value,id,append);
    }

    void 
    putStringId (RpLibrary* lib, 
                 const char* path, 
                 const char* value, 
                 const char* id,
                 int append          )
    {
        lib->put(path,value,id,append);
    }

    void 
    putString ( RpLibrary* lib, 
                const char* path, 
                const char* value, 
                int append          )
    {
        lib->put(path,value,"",append);
    }

    void
    putDoubleId (RpLibrary* lib, 
                 const char* path, 
                 double value, 
                 const char* id,
                 int append         )
    {
        lib->put(path,value,id,append);
    }

    void
    putDouble   (RpLibrary* lib, 
                 const char* path, 
                 double value, 
                 int append         )
    {
        lib->put(path,value,"",append);
    }

    const char* 
    xml (RpLibrary* lib)
    {
        static std::string retStr = "";
        retStr = lib->xml();
        return retStr.c_str();
    }

    const char* 
    nodeComp (RpLibrary* node)
    {
        static std::string retStr = "";
        retStr = node->nodeComp();
        return retStr.c_str();
    }

    const char* 
    nodeType (RpLibrary* node)
    {
        static std::string retStr = "";
        retStr = node->nodeType();
        return retStr.c_str();
    }

    const char* 
    nodeId (RpLibrary* node)
    {
        static std::string retStr = "";
        retStr = node->nodeId();
        return retStr.c_str();
    }



#ifdef __cplusplus
}
#endif
