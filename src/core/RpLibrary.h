/*
 * ----------------------------------------------------------------------
 *  Rappture Library Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef _RpLIBRARY_H
#define _RpLIBRARY_H

enum RP_LIBRARY_CONSTS {
    RPLIB_OVERWRITE     = 0,
    RPLIB_APPEND        = 1,
    RPLIB_NO_TRANSLATE  = 0,
    RPLIB_TRANSLATE     = 1,
    RPLIB_NO_COMPRESS   = 0,
    RPLIB_COMPRESS      = 1,
};


#ifdef __cplusplus

typedef struct _scew_parser scew_parser;
typedef struct _scew_tree scew_tree;
typedef struct _scew_element scew_element;

#include <list>
#include "RpBuffer.h"
#include "RpOutcome.h"

/* indentation size (in whitespaces) */

#define INDENT_SIZE 4
#define CREATE_PATH 1
#define NO_CREATE_PATH 0

class RpLibrary
{
    public:

        // users member fxns

        RpLibrary* element (std::string path = "") const;
        RpLibrary* parent (std::string path = "") const;

        std::list<std::string> entities  (std::string path = "") const;
        std::list<std::string> value     (std::string path = "") const;
        std::list<std::string> diff      ( RpLibrary* otherLib,
                                            std::string path) const;

        bool isNull() const;

        // should return RpObject& but for simplicity right now it doesnt
        // RpObject will either be an Array, RpString, RpNumber ...

        RpLibrary*  children  ( std::string path = "",
                                RpLibrary* rpChildNode = NULL,
                                std::string type = "",
                                int* childCount = NULL  );

        RpLibrary& childCount ( std::string path,
                                int* childCount);

        RpLibrary& copy       ( std::string toPath,
                                RpLibrary* fromObj,
                                std::string fromPath);

        std::string get       ( std::string path = "",
                                int translateFlag = RPLIB_TRANSLATE) const;
        std::string getString ( std::string path = "",
                                int translateFlag = RPLIB_TRANSLATE) const;

        double      getDouble ( std::string path = "") const;
        int         getInt    ( std::string path = "") const;
        bool        getBool   ( std::string path = "") const;
        Rappture::Buffer getData ( std::string path = "") const;
        size_t      getFile   ( std::string path,
                                std::string fileName) const;

        /*
         * Should return some kind of RpError object
        RpLibrary&  get       ( std::string path = "",
                                std::string retVal = "",
                                int translateFlag = RPLIB_TRANSLATE);

        RpLibrary&  get       ( std::string path = "",
                                double retVal = 0.0,
                                int translateFlag = RPLIB_TRANSLATE);

        RpLibrary&  get       ( std::string path = "",
                                int retVal = 0,
                                int translateFlag = RPLIB_TRANSLATE);

        RpLibrary&  get       ( std::string path = "",
                                bool retVal = false,
                                int translateFlag = RPLIB_TRANSLATE);
        */

        RpLibrary& put (    std::string path,
                            std::string value,
                            std::string id = "",
                            unsigned int append = RPLIB_OVERWRITE,
                            unsigned int translateFlag = RPLIB_TRANSLATE   );

        RpLibrary& put (    std::string path,
                            double value,
                            std::string id = "",
                            unsigned int append = RPLIB_OVERWRITE    );

        RpLibrary& put (    std::string path,
                            RpLibrary* value,
                            std::string id = "",
                            unsigned int append = RPLIB_OVERWRITE    );

        RpLibrary& putData( std::string path,
                            const char* bytes,
                            int nbytes,
                            unsigned int append = RPLIB_OVERWRITE    );

        RpLibrary& putFile( std::string path,
                            std::string fileName,
                            unsigned int compress = RPLIB_COMPRESS,
                            unsigned int append = RPLIB_OVERWRITE    );

        RpLibrary* remove (std::string path = "");

        std::string xml() const;

        std::string nodeType() const;
        std::string nodeId() const;
        std::string nodeComp() const;
        std::string nodePath() const;

        Rappture::Outcome& outcome() const;

        void result(int exitStatus=0);


        RpLibrary ();
        RpLibrary (const std::string filePath);
        RpLibrary (const RpLibrary& other);
        RpLibrary& operator= (const RpLibrary& other);
        virtual ~RpLibrary ();

    private:

        scew_parser* parser;
        scew_tree* tree;
        scew_element* root;

        // flag to tell if we are responsible for calling scew_tree_free
        // on the tree structure. if we get our tree by using the
        // scew_tree_create
        // fxn, we need to free it. if we get our tree using the
        // scew_parser_tree
        // fxn, then it will be free'd when the parser is free'd.
        int freeTree;

        // some object (like those returned from children() and element )
        // are previously allocated scew objects with an artificial RpLibrary
        // wrapper. these objects never really allocated any memory, they
        // just point to memory. they should not free any memory when they 
        // are deleted (because none was allocated when they were created).
        // when the larger Rappture Library is deleted, they will have 
        // pointers to bad/deleted memory...libscew has the same problem in 
        // their implementation of a similar children() fxn.
        //
        // this flag tells the destructor that when above said object is
        // deleted, dont act on the root pointer
        int freeRoot;

        mutable Rappture::Outcome status;

        RpLibrary ( scew_element* node, scew_tree* tree)
            :   parser      (NULL),
                tree        (tree),
                root        (node)

        {
            freeTree = 0;
            freeRoot = 0;
        }


        std::string _get_attribute (scew_element* element,
                                    std::string attributeName) const;
        int _path2list (std::string& path,
                        std::string** list,
                        int listLen) const;
        std::string _node2name (scew_element* node) const;
        std::string _node2comp (scew_element* node) const;
        std::string _node2path (scew_element* node) const;

        int _splitPath (std::string& path,
                        std::string& tagName,
                        int* idx,
                        std::string& id ) const;
        scew_element* _find (std::string path, int create) const;
        int _checkPathConflict (scew_element *nodeA, scew_element *nodeB) const;
        void print_indent ( unsigned int indent,
                            std::stringstream& outString) const;
        void print_attributes ( scew_element* element,
                                std::stringstream& outString) const;
        void print_element( scew_element* element,
                            unsigned int indent,
                            std::stringstream& outString ) const;

};

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif // ifdef __cplusplus

#endif // ifndef _RpLIBRARY_H
