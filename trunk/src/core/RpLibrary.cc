/*
 * ----------------------------------------------------------------------
 *  Rappture Library Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "config.h"
#include "scew/scew.h"
#include "scew_extras.h"
#include "RpLibrary.h"
#include "RpEntityRef.h"
#include "RpEncode.h"
#include <algorithm>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <iterator>
#include <cctype>

#ifdef _POSIX_SOURCE
    #include <sys/time.h>
#endif /* _POSIX_SOURCE */

// no arg constructor
// used when we dont want to read an xml file to populate the xml tree
// we are building a new xml structure
RpLibrary::RpLibrary ()
    :   parser      (NULL),
        tree        (NULL),
        root        (NULL),
        freeTree    (1),
        freeRoot    (1)
{
    tree = scew_tree_create();
    root = scew_tree_add_root(tree, "run");
}

RpLibrary::RpLibrary (
            const std::string filePath
        )
    :   parser      (NULL),
        tree        (NULL),
        root        (NULL),
        freeTree    (0),
        freeRoot    (1)
{
    std::stringstream msg;

    if (filePath.length() != 0) {
        // file path should not be null or empty string unless we are
        // creating a new xml file

        parser = scew_parser_create();

        // Don't ignore whitespaces!
        // Things like string inputs may have trailing newlines that
        // matter to the underlying application.
        scew_parser_ignore_whitespaces(parser, 1);

        /* Loads an XML file */
        if (!scew_parser_load_file(parser, filePath.c_str()))
        {
            scew_error code = scew_error_code();
            printf("Unable to load file (error #%d: %s)\n", code,
                   scew_error_string(code));
            msg << "Unable to load file (error #" << code
                << ": " << scew_error_string(code) << ")\n";

            if (code == scew_error_expat)
            {
                enum XML_Error expat_code =
                    scew_error_expat_code(parser);
                printf("Expat error #%d (line %d, column %d): %s\n",
                       expat_code,
                       scew_error_expat_line(parser),
                       scew_error_expat_column(parser),
                       scew_error_expat_string(expat_code));
                msg << "Expat error #" << expat_code << " (line "
                    << scew_error_expat_line(parser) << ", column "
                    << scew_error_expat_column(parser) << "): "
                    << "\n";
            }

            fflush(stdout);
            scew_parser_free(parser);
            parser = NULL;

            // update the status of the call
            status.error(msg.str().c_str());
            status.addContext("RpLibrary::RpLibrary()");
        }
        else
        {
            tree = scew_parser_tree(parser);
            freeTree = 0;
            root = scew_tree_root(tree);
        }
    }
    else {
        // create a new xml (from an empty file)
        freeTree = 1;
        tree = scew_tree_create();
        root = scew_tree_add_root(tree, "run");
    }
}


// copy constructor
// for some reason making this a const gives me problems 
// when calling xml()
// need help looking into this
// RpLibrary ( const RpLibrary& other )
RpLibrary::RpLibrary ( const RpLibrary& other )
    : parser    (NULL),
      tree      (NULL),
      root      (NULL),
      freeTree  (0),
      freeRoot  (1)
{
    std::stringstream msg;
    std::string buffer;
    int buffLen;

    // fill in the current RpLibrary's data with other's data
    parser = scew_parser_create();
    scew_parser_ignore_whitespaces(parser, 1);

    // Loads the XML from other
    // the length cannot be 0 because xml() should not be returning 
    // empty strings
    buffer = other.xml();
    buffLen = buffer.length();

    if (buffLen > 0) {
        if (!scew_parser_load_buffer(parser,buffer.c_str(),buffLen))
        {
            // there was an error loading the buffer
            // how do you tell the user, you couldn't make a copy?
            scew_error code = scew_error_code();
            printf("Unable to load buffer (error #%d: %s)\n", code,
                   scew_error_string(code));
            msg << "Unable to load file (error #" << code
                << ": " << scew_error_string(code) << ")\n";

            if (code == scew_error_expat)
            {
                enum XML_Error expat_code =
                    scew_error_expat_code(parser);
                printf("Expat error #%d (line %d, column %d): %s\n",
                       expat_code,
                       scew_error_expat_line(parser),
                       scew_error_expat_column(parser),
                       scew_error_expat_string(expat_code));
                msg << "Expat error #" << expat_code << " (line "
                    << scew_error_expat_line(parser) << ", column "
                    << scew_error_expat_column(parser) << "): "
                    << "\n";
            }

            // return an empty RpLibrary?
            // return EXIT_FAILURE;

            parser = NULL;

            // update the status of the call
            status.error(msg.str().c_str());
            status.addContext("RpLibrary::RpLibrary()");
        }
        else {

            // parsing the buffer was successful
            // populate the new data members.

            tree = scew_parser_tree(parser);
            freeTree = 0;
            freeRoot = 1;
            root = scew_tree_root(tree);

        }

    } // end if (buffer.length() != 0) {
}// end copy constructor

// copy assignment operator
// for some reason making this a const gives me problems 
// when calling xml()
// need help looking into this
// RpLibrary& operator= (const RpLibrary& other) {
RpLibrary&
RpLibrary::operator= (const RpLibrary& other) {

    std::stringstream msg;
    std::string buffer;
    int buffLen;

    scew_parser* tmp_parser;
    scew_tree* tmp_tree;
    scew_element* tmp_root;
    int tmp_freeTree;
    int tmp_freeRoot;

    if (this != &other) {

        tmp_parser   = parser;
        tmp_tree     = tree;
        tmp_root     = root;
        tmp_freeTree = freeTree;
        tmp_freeRoot = freeRoot;

        // fill in the current RpLibrary's data with other's data
        parser = scew_parser_create();
        scew_parser_ignore_whitespaces(parser, 1);

        // Loads the XML from other
        // the length cannot be 0 because xml() 
        // should not be returning empty strings
        buffer = other.xml();
        buffLen = buffer.length();

        if (buffLen > 0) {
            if (!scew_parser_load_buffer(parser,buffer.c_str(),buffLen))
            {
                // there was an error loading the buffer
                // how do you tell the user, you couldn't make a copy?
                scew_error code = scew_error_code();
                printf("Unable to load buffer (error #%d: %s)\n", code,
                       scew_error_string(code));
                msg << "Unable to load file (error #" << code
                    << ": " << scew_error_string(code) << ")\n";

                if (code == scew_error_expat)
                {
                    enum XML_Error expat_code = 
                        scew_error_expat_code(parser);
                    printf("Expat error #%d (line %d, column %d): %s\n",
                           expat_code,
                           scew_error_expat_line(parser),
                           scew_error_expat_column(parser),
                           scew_error_expat_string(expat_code));
                    msg << "Expat error #" << expat_code << " (line "
                        << scew_error_expat_line(parser) << ", column "
                        << scew_error_expat_column(parser) << "): "
                        << "\n";
                }

                // return things back to the way they used to be
                // or maybe return an empty RpLibrary?
                // return EXIT_FAILURE;

                // return this object to its previous state.
                parser = tmp_parser;

                // update the status of the call
                status.error(msg.str().c_str());
                status.addContext("RpLibrary::RpLibrary()");
            }
            else {

                // parsing the buffer was successful
                // populate the new data members.

                tree = scew_parser_tree(parser);
                freeTree = 0;
                freeRoot = 1;
                root = scew_tree_root(tree);

                // free the current RpLibrary's data
                // we do the free so far down so we can see if
                // parsing the other object's xml fails.
                // if the parsing fails, we can still return this
                // object to its previous state.
                if (tmp_tree && tmp_freeTree) {
                    scew_tree_free(tmp_tree);
                    tmp_tree = NULL;
                }
                if (tmp_parser) {
                    scew_parser_free(tmp_parser);
                    tmp_parser = NULL;
                }
                if (tmp_root && tmp_freeRoot) {
                    tmp_root = NULL;
                }
            }

        } // end if (buffer.length() != 0) {
    } // end if (this != &other)

    return *this;
} // end operator=


// default destructor
RpLibrary::~RpLibrary ()
{
    // clean up dynamic memory

    if (tree && freeTree) {
        scew_tree_free(tree);
        tree = NULL;
    }
    if (parser) {
        scew_parser_free(parser);
        parser = NULL;
    }
    if (!freeTree && root && freeRoot) {
        scew_element_free(root);
        root = NULL;
    }
}
/**********************************************************************/
// METHOD: _get_attribute()
/// Return the attribute value matching the provided attribute name.
/**
 */

std::string
RpLibrary::_get_attribute (
    scew_element* element,
    std::string attributeName
    ) const
{
    scew_attribute* attribute = NULL;
    std::string attrVal;

    if (element != NULL)
    {
        if (scew_attribute_count(element) > 0) {

            while((attribute=scew_attribute_next(element, attribute)) != NULL)
            {
                if (    strcmp( scew_attribute_name(attribute),
                                attributeName.c_str()) == 0     ){
                    attrVal = scew_attribute_value(attribute);
                }
            }
        }
        else {
            // there are no attributes, count == 0
        }
    }

    return attrVal;
}

/**********************************************************************/
// METHOD: _path2list()
/// Convert a path into a list of element names.
/**
 */

int
RpLibrary::_path2list (
    std::string& path,
    std::string** list,
    int listLen
    ) const
{
    std::string::size_type pos = 0;
    std::string::size_type start = 0;
    std::string::size_type end = path.length();
    int index = 0;
    int retVal = 0;
    unsigned int parenDepth = 0;

    for (   pos = 0; (pos < end) && (index < listLen); pos++) {
        if (path[pos] == '(') {
            parenDepth++;
            continue;
        }

        if (path[pos] == ')') {
            parenDepth--;
            continue;
        }

        if ( (path[pos] == '.') && (parenDepth == 0) ) {
            list[index] = new std::string(path.substr(start,pos-start));
            index++;
            start = pos + 1;
        }
    }

    // add the last path to the list
    // error checking for path names like p1.p2.
    if ( (start < end) && (pos == end) ) {
        list[index] = new std::string(path.substr(start,pos-start));
    }
    retVal = index;
    index++;

    // null out the rest of the pointers so we know where to stop free'ing
    while (index < listLen) {
        list[index++] = NULL;
    }

    return retVal;
}

/**********************************************************************/
// METHOD: _node2name()
/// Retrieve the id of a node.
/**
 */

std::string
RpLibrary::_node2name (scew_element* node) const
{
    // XML_Char const* name = _get_attribute(node,"id");
    std::string name = _get_attribute(node,"id");
    std::stringstream retVal;
    XML_Char const* type = NULL;
    scew_element* parent = NULL;
    scew_element** siblings = NULL;
    unsigned int count = 0;
    int tmpCount = 0;
    int index = 0;
    std::string indexStr;

    type = scew_element_name(node);
    parent = scew_element_parent(node);

    if (parent) {

        // if (name == NULL) {
        if (name.empty()) {
            siblings = scew_element_list(parent, type, &count);
            if (count > 0) {
                tmpCount = count;
                while ((index < tmpCount) && (siblings[index] != node)) {
                    index++;
                }

                if (index < tmpCount) {

                    if (index > 0) {

                        retVal << type << --index;
                    }
                    else {

                        retVal << type;
                    }

                    /*
                    if (retVal == NULL) {
                        // error with allocating space
                        return NULL;
                    }
                    */
                }
            }

            scew_element_list_free(siblings);

        }
        else {

            retVal << name;
        }
    }

    return (retVal.str());
}

/**********************************************************************/
// METHOD: _node2comp()
/// Retrieve the component name of a node
/**
 */

std::string
RpLibrary::_node2comp (scew_element* node) const
{
    // XML_Char const* name = _get_attribute(node,"id");
    std::string id = _get_attribute(node,"id");
    std::stringstream retVal;
    XML_Char const* type = NULL;
    scew_element* parent = NULL;
    scew_element** siblings = NULL;
    unsigned int count = 0;
    int tmpCount = 0;
    int index = 0;
    std::string indexStr;

    type = scew_element_name(node);
    parent = scew_element_parent(node);

    if (parent) {
        if (id.empty()) {
            siblings = scew_element_list(parent, type, &count);
            if (count > 0) {
                tmpCount = count;
                // figure out what the index value should be
                while ((index < tmpCount) && (siblings[index] != node)) {
                    index++;
                }

                if (index < tmpCount) {
                    if (index > 0) {
                        // retVal << type << --index;
                        retVal << type << index;
                    }
                    else {
                        retVal << type;
                    }
                }

            }
            else {
                // count == 0 ??? this state should never be reached
            }
            scew_element_list_free(siblings);

        }
        else {
            // node has attribute id
            retVal << type << "(" << id << ")";

        }
    }

    return (retVal.str());
}

/**********************************************************************/
// METHOD: _node2path()
/// Retrieve the full path name of a node
/**
 */

std::string
RpLibrary::_node2path (scew_element* node) const
{

    std::stringstream path;
    scew_element* snode = node;
    scew_element* parent = NULL;

    if (snode) {
        parent = scew_element_parent(snode);
        path.clear();

        while (snode && parent) {

            if (!path.str().empty()) {
                path.str(_node2comp(snode) + "." + path.str());
                // path.str("." + _node2comp(snode) + path.str());
            }
            else {
                path.str(_node2comp(snode));
                // path.str("." + _node2comp(snode));
            }

            snode = scew_element_parent(snode);
            parent = scew_element_parent(snode);
        }
    }

    return (path.str());
}

/**********************************************************************/
// METHOD: _splitPath()
/// Split a path (component) name into its tag name, index, and id
/**
 */

int
RpLibrary::_splitPath ( std::string& path,
                        std::string& tagName,
                        int* idx,
                        std::string& id ) const
{
    int stop = 0;
    int start = 0;
    int index = path.length();

    if (index) {
        index--;
    }

    if (!path.empty()) {
        if (path[index] == ')') {
            stop = index;
            while (path[index] != '(') {
                index--;
            }
            start = index+1;
            // strncpy(id,path+start,stop-start);
            // id = new std::string(path.substr(start,stop-start));
            id = path.substr(start,stop-start);
            index--;
        }
        if (isdigit(path[index])) {
            stop = index;
            while (isdigit(path[index])) {
                index--;
            }
            // sscanf(path[index+1],"%d",idx);
            sscanf(path.c_str()+index+1,"%d",idx);
        }
        if (isalpha(path[index])) {
            start = 0;
            stop = index+1;
            // tagName = new std::string(path.substr(start,stop-start));
            tagName = path.substr(start,stop-start);
            // strncpy(tagName,path+start,stop-start);
        }
    }
    else {
        tagName = "";
        *idx = 0;
        id = "";
    }

    return 1;
}

/**********************************************************************/
// METHOD: _find()
/// Find or create a node and return it.
/**
 */

scew_element*
RpLibrary::_find(std::string path, int create) const
{
    std::string tagName = "";
    std::string id = "";
    int index = 0;
    int listLen = (path.length()/2)+1;
    std::string** list;
    int path_size = 0;
    int listIdx = 0;
    unsigned int count = 0;
    int tmpCount = 0;
    int lcv = 0;
    std::string tmpId;

    scew_element* tmpElement = this->root;
    scew_element* node = NULL;
    scew_element** eleList = NULL;


    if (path.empty()) {
        // user gave an empty path
        return tmpElement;
    }

    list = (std::string **) calloc(listLen, sizeof( std::string * ) );

    if (!list) {
        // error calloc'ing space for list
        return NULL;
    }

    path_size = _path2list (path,list,listLen);

    while ( (listIdx <= path_size) && (tmpElement != NULL ) ){

        _splitPath(*(list[listIdx]),tagName,&index,id);

        if (id.empty()) {
            /*
            # If the name is like "type2", then look for elements with
            # the type name and return the one with the given index.
            # If the name is like "type", then assume the index is 0.
            */

            eleList = scew_element_list(tmpElement, tagName.c_str(), &count);
            tmpCount = count;
            if (index < tmpCount) {
                node = eleList[index];
            }
            else {
                /* there is no element with the specified index */
                node = NULL;
            }

            scew_element_list_free(eleList);
            eleList = NULL;
        }
        else {

            /* what if its like type2(id) ? */
            /* still unresolved */

            /*
            # If the name is like "type(id)", then look for elements
            # that match the type and see if one has the requested name.
            # if the name is like "(id)", then look for any elements
            # with the requested name.
            */

            if (!tagName.empty()) {
                eleList = scew_element_list(tmpElement, tagName.c_str(), &count);
            }
            else {
                eleList = scew_element_list_all(tmpElement, &count);
            }

            tmpCount = count;
            for (lcv = 0; (lcv < tmpCount); lcv++) {
                tmpId = _get_attribute(eleList[lcv], "id");
                if (!tmpId.empty()) {
                    if (id == tmpId) {
                        node = eleList[lcv];
                        break;
                    }
                }
            }

            if (lcv >= tmpCount) {
                node = NULL;
            }

            scew_element_list_free(eleList);
            eleList = NULL;

        }

        if (node == NULL) {
            if (create == NO_CREATE_PATH) {
                // break out instead of returning because we still need to
                // free the list variable
                tmpElement = node;
                break;
            }
            else {
                // create == CREATE_PATH
                // we should create the rest of the path

                // create the new element
                // need to figure out how to properly number the new element
                node = scew_element_add(tmpElement,tagName.c_str());
                if (! node) {
                    // a new element was not created
                }

                // add an attribute and attrValue to the new element
                if (!id.empty()) {
                    scew_element_add_attr_pair(node,"id",id.c_str());
                }
            }
        }

        tagName = "";
        id = "";
        index = 0;
        tmpElement = node;
        listIdx++;
    }

    // need to free the strings inside of list

    if (list) {
        for (listIdx = 0; listIdx < listLen; listIdx++) {
            if (list[listIdx]) {
                delete(list[listIdx]);
                list[listIdx] = NULL;
            }
        }

        free(list);
        list = NULL;
    }


    return tmpElement;
}

/**********************************************************************/
// METHOD: _checkPathConflict(scew_element *nodeA,scew_element *nodeB)
/// check to see if nodeA is in nodeB's path
/**
 * This is used by put() function (the RpLibrary flavor). It is
 * used to check if nodeA can be safely deleted and not effect nodeB
 */

int
RpLibrary::_checkPathConflict(scew_element *nodeA, scew_element *nodeB) const
{
    scew_element *testNode = NULL;

    if ( (nodeA == NULL) || (nodeB == NULL) ) {
        return 0;
    }

    if (nodeA == nodeB) {
        return 1;
    }

    testNode = nodeB;

    while ((testNode = scew_element_parent(testNode)) != NULL) {
        if (testNode == nodeA) {
            return 1;
        }
    }

    return 0;
}

/**********************************************************************/
// METHOD: element()
/// Search the path of a xml tree and return a RpLibrary node.
/**
 * It is the user's responsibility to delete the object when 
 * they are finished using it?, else i need to make this static
 */

RpLibrary*
RpLibrary::element (std::string path) const
{
    RpLibrary* retLib = NULL;
    scew_element* retNode = NULL;

    if (!this->root) {
        // library doesn't exist, do nothing;
        return NULL;
    }

    /*
    if (path.empty()) {
        // an empty path returns the current RpLibrary
        return this;
    }
    */

    if (path.empty()) {
        // this should be a smart pointer, 
        // if someone deletes the original this->root, this object is void
        // and will be a memory leak.
        // retNode = this->root;
        retLib = new RpLibrary(*this);
    }
    else {
        // get the node located at path
        retNode = _find(path,NO_CREATE_PATH);

        // if the node exists, create a rappture library object for it.
        if (retNode) {
            retLib = new RpLibrary( retNode,this->tree );
        }
    }

    return retLib;
}

/**********************************************************************/
// METHOD: entities()
/// Search the path of a xml tree and return a list of its entities.
/**
 */

std::list<std::string>
RpLibrary::entities  (std::string path) const
{
    std::list<std::string> queue;
    std::list<std::string>::iterator iter;
    std::list<std::string> retList;
    std::list<std::string> childList;
    std::list<std::string>::iterator childIter;

    RpLibrary* ele = NULL;
    std::string pathBack = "";

    RpLibrary* child = NULL;
    std::string childType = "";
    std::string childPath = "";
    std::string paramsPath = "";

    RpLibrary* cchild = NULL;
    std::string cchildType = "";
    std::string cchildPath = "";

    queue.push_back(path);
    iter = queue.begin();

    while( iter != queue.end() ) {
        ele = this->element(*iter);
        child = NULL;

        if ((*iter).empty()) {
            pathBack = "";
        }
        else {
            pathBack = *iter + ".";
        }

        while ( ele && (child = ele->children("",child)) != NULL ) {
            childList.push_back(child->nodeComp());
            delete child;
        }

        childIter = childList.begin();

        while (childIter != childList.end()) {
            child = ele->element(*childIter);

            childType = child->nodeType();
            childPath = child->nodeComp();
            if ( (childType == "group") || (childType == "phase") ) {
                // add this path to the queue for paths to search
                queue.push_back(pathBack+childPath);
            }
            else if (childType == "structure") {
                // add this path to the return list
                retList.push_back(pathBack+child->nodeComp());

                // check to see if there is a ".current.parameters" node
                // if so, add them to the queue list for paths to search
                paramsPath = "current.parameters";
                if (child->element(paramsPath) != NULL) {
                    queue.push_back((pathBack+child->nodeComp()+"."+paramsPath));
                }
            }
            else {
                // add this path to the return list
                retList.push_back(pathBack+child->nodeComp());

                // look for embedded groups and phases
                // add them to the queue list for paths to search
                cchild = NULL;
                while ( (cchild = child->children("",cchild)) != NULL ) {
                    cchildType = cchild->nodeType();
                    cchildPath = cchild->nodePath();
                    if ( (cchildType == "group") || (cchildType == "phase") ) {
                        // add this path to the queue for paths to search
                        queue.push_back(cchildPath);
                    }
                    delete cchild;
                }
            }

            childList.erase(childIter);
            childIter = childList.begin();
            delete child;
        }

        queue.erase(iter);
        iter = queue.begin();
    }

    return retList;
}

/**********************************************************************/
// METHOD: diff()
/// find the differences between two xml trees.
/**
 */

std::list<std::string>
RpLibrary::diff (RpLibrary* otherLib, std::string path) const
{

    std::list<std::string> thisVal; // two node list of specific entity's value
    std::list<std::string> otherVal; // two node list of specific entity's value

    std::list<std::string> thisv; // list of this library's entities
    std::list<std::string>::iterator thisIter;

    std::list<std::string> otherv; // list of other library's entities
    std::list<std::string>::iterator otherIter;

    std::list<std::string> retList;

    std::string entry = "";
    std::string thisSpath = "";  // temp string
    std::string otherSpath = ""; // temp string

    if ( (!this->root) || (!otherLib->root) ) {
        // library doesn't exist, do nothing;
        return retList;
    }


    thisv = this->entities(path);
    otherv = otherLib->entities(path);

    thisIter = thisv.begin();

    while (thisIter != thisv.end() ) {
        // reset otherIter for a new search.
        otherIter = otherv.begin();
        while ( (otherIter != otherv.end()) && (*otherIter != *thisIter) ) {
            otherIter++;
        }

        //if (!path.empty()) {
        //    thisSpath = path + "." + *thisIter;
        //}
        //else {
        //    thisSpath = *thisIter;
        //}
        thisSpath = *thisIter;

        if (otherIter == otherv.end()) {
            // we've reached the end of the search 
            // and did not find anything, mark this as a '-'
            thisVal = this->value(thisSpath);
            retList.push_back("-");
            retList.push_back(thisSpath);
            retList.push_back(thisVal.front());
            retList.push_back("");
        }
        else {

            //if (!path.empty()) {
            //    otherSpath = path + "." + *otherIter;
            //}
            //else {
            //    otherSpath = *otherIter;
            //}
            otherSpath = *otherIter;

            thisVal = this->value(thisSpath);
            otherVal = otherLib->value(otherSpath);
            if (thisVal.back() != otherVal.back()) {
                // add the difference to the return list
                retList.push_back("c");
                retList.push_back(otherSpath);
                retList.push_back(thisVal.front());
                retList.push_back(otherVal.front());
            }

            // remove the last processed value from otherv
            otherv.erase(otherIter);
        }

        // increment thisv's iterator.
        thisIter++;
    }

    // add any left over values in otherv to the return list
    otherIter = otherv.begin();
    while ( otherIter != otherv.end() ) {

        //if (!path.empty()) {
        //    otherSpath = path + "." + *otherIter;
        //}
        //else {
        //    otherSpath = *otherIter;
        //}
        otherSpath = *otherIter;

        otherVal = otherLib->value(otherSpath);

        retList.push_back("+");
        retList.push_back(otherSpath);
        retList.push_back("");
        retList.push_back(otherVal.front());

        otherv.erase(otherIter);
        otherIter = otherv.begin();
    }

    return retList;
}

/**********************************************************************/
// METHOD: value(path)
/// Return a 2 element list containing the regular and normalized values.
/**
 */

std::list<std::string>
RpLibrary::value (std::string path) const
{
    std::list<std::string> retArr;

    std::string raw = "";
    std::string val = "";

    RpLibrary* ele = NULL;
    RpLibrary* tele = NULL;

    int childCount = 0;
    std::stringstream valStr;

    ele = this->element(path);

    if (ele != NULL ) {

        if (ele->nodeType() == "structure") {
            raw = path;
            // try to find a label to represent the structure
            val = ele->get("about.label");

            if (val == "") {
               val = ele->get("current.about.label");
            }

            if (val == "") {
               tele = ele->element("current");
               if ( (tele != NULL) && (tele->nodeComp() != "") ) {
                   tele->children("components",NULL,"",&childCount);
                   valStr << "<structure> with " << childCount  << " components";
                   val = valStr.str();
               }
            }

        }
        /*
        else if (ele->nodeType() == "number") {
            raw = "";
            retArr[1] = "";
            if ( (tele = ele->element("current")) != NULL) {
                raw = tele->get();
            }
            else if ( (tele = ele->element("default")) != NULL) {
                raw = tele->get();
            }
            val = raw
            if ( "" != raw) {
                // normalize the units
                units = ele->get("units");
                if ( "" != units) {

                }
        }
        */
        else {
            raw = "";
            if ( (tele = ele->element("current")) != NULL) {
                raw = tele->get();
            }
            else if ( (tele = ele->element("default")) != NULL) {
                raw = tele->get();
            }
            val = raw;
        }
    }

    retArr.push_back(raw);
    retArr.push_back(val);

    return retArr;
}

/**********************************************************************/
// METHOD: parent()
/// Search the path of a xml tree and return its parent.
/**
 * It is the user's responsibility to delete the object when 
 * they are finished using it?, else i need to make this static
 */

RpLibrary*
RpLibrary::parent (std::string path) const
{
    RpLibrary* retLib = NULL;
    std::string parentPath = "";
    scew_element* ele = NULL;
    scew_element* retNode = NULL;

    if (!this->root) {
        // library doesn't exist, do nothing;
        return NULL;
    }

    if (path.empty()) {
        // an empty path returns the parent of the current RpLibrary
        ele = this->root;
    }
    else {
        // else find the node representing the provided path
        ele = _find(path,NO_CREATE_PATH);
    }

    if (ele != NULL) {
        retNode = scew_element_parent(ele);
        if (retNode) {
            // allocate a new rappture library object for the node
            retLib = new RpLibrary( retNode,this->tree );
        }
    }
    else {
        // path was not found by _find
    }


    return retLib;
}

/**********************************************************************/
// METHOD: copy()
/// Copy the value from fromPath to toPath.
//  Copy the value from fromObj.fromPath to toPath 
//  of the current rappture library object. This can copy rappture
//  library elements within or between xml trees.
/**
 */

RpLibrary&
RpLibrary::copy (   std::string toPath,
                    RpLibrary* fromObj,
                    std::string fromPath )
{
    RpLibrary* value = NULL;

    if (!this->root) {
        // library doesn't exist, do nothing;
        // need a good way to raise error, and this is not it.
        return *this;
    }

    if (fromObj == NULL) {
        fromObj = this;
    }

    if ( (fromObj == this) && (toPath == fromPath) ) {
        /* cannot copy over myself, causes path to disappear */
        return (*this);
    }

    value = fromObj->element(fromPath);

    if ( !value ) {
        status.error("fromPath could not be found within fromObj");
        status.addContext("RpLibrary::copy");
        return *this;
    }

    this->put(toPath, value);
    status.addContext("RpLibrary::copy");
    delete value;

    return (*this);

}

/**********************************************************************/
// METHOD: children()
/// Return the next child of the node located at 'path'
//
// The lookup is reset when you send a NULL rpChilNode.
// User is responsible for deleting returned values
// 
/**
 */

/*
RpLibrary*
RpLibrary::children (   std::string path, 
                        RpLibrary* rpChildNode, 
                        std::string type,
                        int* childCount)
{
    static std::string old_path = "";
    static RpLibrary* retLib = NULL; 
    int myChildCount = 0;
    scew_element* parentNode = NULL;
    scew_element* childNode = NULL;
    std::string childName = "";

    if (!this->root) {
        // library doesn't exist, do nothing;
        return NULL;
    }


    if (path.empty()) {
        // an empty path uses the current RpLibrary as parent
        parentNode = this->root;
    }
    else {
        // check to see if this path is the same as the one set
        // in the last call to this function.
        // if so, then we dont need to reset the parentNode
        //
        // this check is probably more dependent on rpChildNode
        // because we want to see if the person want to continue
        // an old search or start from the beginning of the child list
        //
        if ( (path.compare(old_path) == 0) && (rpChildNode != NULL) ) {
            parentNode = NULL;
        }
        // we need to search for a new parentNode
        else {
            parentNode = _find(path,NO_CREATE_PATH);
            if (parentNode == NULL) {
                // node not found
                // add error code here
                return NULL;
            }
        }
    }

    old_path = path;


    if (rpChildNode) {
        childNode = rpChildNode->root;
    }

    if (parentNode) {
        myChildCount = scew_element_count(parentNode);
    }

    if (childCount) {
        *childCount = myChildCount;
    }

    // clean up old memory
    delete retLib;

    if ( (childNode = scew_element_next(parentNode,childNode)) ) {

        if (!type.empty()) {
            childName = scew_element_name(childNode);
            // we are searching for a specific child name
            // keep looking till we find a name that matches the type.
            // if the current name does not match out search type, 
            // grab the next child and check to see if its null
            // if its not null, get its name and test for type again
            while (  (type != childName)
                  && (childNode = scew_element_next(parentNode,childNode)) ) {

                childName = scew_element_name(childNode);
            }
            if (type == childName) {
                // found a child with a name that matches type
                retLib = new RpLibrary( childNode,this->tree );
            }
            else {
                // no children with names that match 'type' were found
                retLib = NULL;
            }
        }
        else {
            retLib = new RpLibrary( childNode,this->tree );
        }
    }
    else {
        // somthing happened within scew, get error code and display
        // its probable there are no more child elements left to report
        retLib = NULL;
    }

    return retLib;
}
*/

RpLibrary*
RpLibrary::children (   std::string path,
                        RpLibrary* rpChildNode,
                        std::string type,
                        int* childCount)
{
    // this is static for efficency reasons
    static std::string old_path = "";
    // this was static so user never has to delete the retLib.
    // should be replaced by a smart pointer
    // static RpLibrary* retLib = NULL; 
    RpLibrary* retLib = NULL; 
    int myChildCount = 0;
    scew_element* parentNode = NULL;
    scew_element* childNode = NULL;
    std::string childName = "";

    if (!this->root) {
        // library doesn't exist, do nothing;
        return NULL;
    }


    // check to see if the last call to this function
    // was searching for children of the same path.
    if ( (path.compare(old_path) == 0) && (rpChildNode != NULL) ) {
        parentNode = NULL;
    }
    else if (path.empty()) {
        // searching for children in a new path.
        // an empty path uses the current RpLibrary as parent
        parentNode = this->root;
    }
    else {
        // searching for children in a new, non-empty, path.
        parentNode = _find(path,NO_CREATE_PATH);
        if (parentNode == NULL) {
            // node not found
            // add error code here
            return NULL;
        }
    }

    old_path = path;

    if (rpChildNode) {
        childNode = rpChildNode->root;
    }

    if (parentNode) {
        myChildCount = scew_element_count(parentNode);
        if (childCount) {
            *childCount = myChildCount;
        }
    }

    // clean up old memory
    // delete retLib;

    if ( (childNode = scew_element_next(parentNode,childNode)) ) {

        if (!type.empty()) {
            childName = scew_element_name(childNode);
            // we are searching for a specific child name
            // keep looking till we find a name that matches the type.
            // if the current name does not match out search type, 
            // grab the next child and check to see if its null
            // if its not null, get its name and test for type again
            while (  (type != childName)
                  && (childNode = scew_element_next(parentNode,childNode)) ) {

                childName = scew_element_name(childNode);
            }
            if (type == childName) {
                // found a child with a name that matches type
                retLib = new RpLibrary( childNode,this->tree );
            }
            else {
                // no children with names that match 'type' were found
                retLib = NULL;
            }
        }
        else {
            retLib = new RpLibrary( childNode,this->tree );
        }
    }
    else {
        // somthing happened within scew, get error code and display
        // its probable there are no more child elements left to report
        retLib = NULL;
    }

    return retLib;
}

/**********************************************************************/
// METHOD: childCount()
/// Returns a std::list<RpLibrary*> of all children under 'path'
//
// 
/**
 */

RpLibrary&
RpLibrary::childCount(std::string path, int* childCount)
{
    if (this->root) {
        scew_element* parentNode;
        int myChildCount = 0;

        parentNode = NULL;
        if (path.empty()) {
            // an empty path uses the current RpLibrary as parent
            parentNode = this->root;
        }

        if (parentNode) {
            myChildCount = scew_element_count(parentNode);
        }

        if (childCount) {
            *childCount = myChildCount;
        }
    }
    return *this;
}

/**********************************************************************/
// METHOD: isNull()
/// returns whether this RpLibrary is a valid library node
//
// 
/**
 */

bool
RpLibrary::isNull () const
{
    if (this->root) {
        return false;
    }

    return true;
}

/**********************************************************************/
// METHOD: get()
/// Return the string value of the object held at location 'path'
/**
 */

std::string
RpLibrary::get (std::string path, int translateFlag) const
{
    return (this->getString(path, translateFlag));
}

/**********************************************************************/
// METHOD: getString()
/// Return the string value of the object held at location 'path'
/**
 */

std::string
RpLibrary::getString (std::string path, int translateFlag) const
{
    Rappture::EntityRef ERTranslator;
    scew_element* retNode = NULL;
    XML_Char const* retCStr = NULL;
    const char* translatedContents = NULL;
    std::string retStr = "";
    Rappture::Buffer inData;

    status.addContext("RpLibrary::getString");
    if (!this->root) {
        // library doesn't exist, do nothing;
        return retStr;
    }

    retNode = _find(path,NO_CREATE_PATH);

    if (retNode == NULL) {
        // need to raise error
        return retStr;
    }

    retCStr = scew_element_contents(retNode);
    if (!retCStr) {
        return retStr;
    }
    inData = Rappture::Buffer(retCStr);

    if (Rappture::encoding::headerFlags(inData.bytes(),inData.size()) != 0) {
        // data is encoded,
        // coming from an rplib, this means it was at least base64 encoded
        // there is no reason to do entity translation
        // because base64 character set does not include xml entity chars
        if (!Rappture::encoding::decode(status, inData, 0)) {
            return retStr;
        }
        retStr = std::string(inData.bytes(),inData.size());
    } else {
        // check translateFlag to see if we need to translate entity refs
        if (translateFlag == RPLIB_TRANSLATE) {
            translatedContents = ERTranslator.decode(inData.bytes(),
                                                     inData.size());
            if (translatedContents == NULL) {
                // translation failed
                if (!status) {
                    status.error("Error while translating entity references");
                    return retStr;
                }
            } else {
                // subtract 1 from size because ERTranslator adds extra NULL
                retStr = std::string(translatedContents,ERTranslator.size()-1);
                translatedContents = NULL;
            }
        }
    }
    inData.clear();
    return retStr;
}

/**********************************************************************/
// METHOD: getDouble()
/// Return the double value of the object held at location 'path'
/**
 */

double
RpLibrary::getDouble (std::string path) const
{
    std::string retValStr = "";
    double retValDbl = 0;

    if (!this->root) {
        // library doesn't exist, do nothing;
        return retValDbl;
    }

    retValStr = this->getString(path);
    status.addContext("RpLibrary::getDouble");
    // think about changing this to strtod()
    retValDbl = atof(retValStr.c_str());

    return retValDbl;
}


/**********************************************************************/
// METHOD: getInt()
/// Return the integer value of the object held at location 'path'
/**
 */

int
RpLibrary::getInt (std::string path) const
{
    std::string retValStr = "";
    int retValInt = 0;

    if (!this->root) {
        // library doesn't exist, do nothing;
        return retValInt;
    }

    retValStr = this->getString(path);
    status.addContext("RpLibrary::getInt");
    // think about changing this to strtod()
    retValInt = atoi(retValStr.c_str());

    return retValInt;
}


/**********************************************************************/
// METHOD: getBool()
/// Return the boolean value of the object held at location 'path'
/**
 */

bool
RpLibrary::getBool (std::string path) const
{
    std::string retValStr = "";
    bool retValBool = false;
    int retValLen = 0;

    if (!this->root) {
        // library doesn't exist, do nothing;
        return retValBool;
    }

    retValStr = this->getString(path);
    status.addContext("RpLibrary::getBool");
    std::transform (retValStr.begin(),retValStr.end(),retValStr.begin(),tolower);
    retValLen = retValStr.length();

    if ((retValStr.compare(0,retValLen,"1",0,retValLen) == 0  )   ||
        (retValStr.compare(0,retValLen,"yes",0,retValLen) == 0 )  ||
        (retValStr.compare(0,retValLen,"true",0,retValLen) == 0 ) ||
        (retValStr.compare(0,retValLen,"on",0,retValLen) == 0))
    {
        retValBool = true;
    }
    else if((retValStr.compare(0,retValLen,"0",0,retValLen) == 0  )    ||
            (retValStr.compare(0,retValLen,"no",0,retValLen) == 0 )    ||
            (retValStr.compare(0,retValLen,"false",0,retValLen) == 0 ) ||
            (retValStr.compare(0,retValLen,"off",0,retValLen) == 0))
    {
        retValBool = false;
    }
    else {
        // default to false?
        retValBool = false;
    }

    return retValBool;
}


/**********************************************************************/
// METHOD: getData()
/// Return a pointer and memory size of the object held at location 'path'
/**
 */

Rappture::Buffer
RpLibrary::getData (std::string path) const
{
    Rappture::EntityRef ERTranslator;
    scew_element* retNode = NULL;
    const char* retCStr = NULL;
    Rappture::Buffer buf;
    int translateFlag = RPLIB_TRANSLATE;
    const char* translatedContents = NULL;
    int len = 0;

    if (!this->root) {
        // library doesn't exist, do nothing;
        return buf;
    }

    retNode = _find(path,NO_CREATE_PATH);

    if (retNode == NULL) {
        // need to raise error
        status.error("could not find element located at path");
        status.addContext("RpLibrary::getData()");
        return buf;
    }

    retCStr = scew_element_contents(retNode);

    if (retCStr == NULL) {
        // element located at path is empty
        return buf;
    }

    if (translateFlag == RPLIB_TRANSLATE) {
        translatedContents = ERTranslator.decode(retCStr,0);
        if (translatedContents == NULL) {
            // translation failed
            if (!status) {
                status.error("Error while translating entity references");
                status.addContext("RpLibrary::getData()");
            }
        }
        else {
            len = strlen(translatedContents);
            buf.append(translatedContents,len);
            translatedContents = NULL;
        }
    }
    else {
        len = strlen(retCStr);
        buf.append(retCStr,len);
    }

    return buf;
}


/**********************************************************************/
// METHOD: getFile()
/// Get the value at path and write it to the file at fileName
/**
 * Return the number of bytes written
 */

size_t
RpLibrary::getFile (std::string path, std::string fileName) const
{
    Rappture::Buffer buf;

    buf = getData(path);

    if (buf.bad()) {
        status.addContext("RpLibrary::getFile()");
        return 0;
    }

    if (!buf.dump(status, fileName.c_str())) {
        status.addContext("RpLibrary::getFile()");
        return 0;
    }

    return buf.size();
}


/**********************************************************************/
// METHOD: put()
/// Put a string value into the xml.
/**
 */

RpLibrary&
RpLibrary::put (std::string path, std::string value, std::string id,
                unsigned int append, unsigned int translateFlag)
{
    Rappture::EntityRef ERTranslator;
    scew_element* retNode = NULL;
    std::string tmpVal = "";
    const char* contents = NULL;
    const char* translatedContents = NULL;

    status.addContext("RpLibrary::put() - putString");

    if (!this->root) {
        // library doesn't exist, do nothing;
        status.error("invalid library object");
        return *this;
    }

    // check for binary data
    // FIXME: I've already appended a NUL-byte to this assuming that
    //        it's a ASCII string. This test must come before.
    if (Rappture::encoding::isBinary(value.c_str(), value.length())) {
        putData(path, value.c_str(), value.length(), append);
        return *this;
    }

    retNode = _find(path, CREATE_PATH);
    if (retNode == NULL) {
        // node not found, set error
        status.error("Error while searching for node: node not found");
        return *this;
    }

    if (translateFlag == RPLIB_TRANSLATE) {
        translatedContents = ERTranslator.encode(value.c_str(),0);
        if (translatedContents == NULL) {
            // entity referene translation failed
            if (!status) {
                status.error("Error while translating entity references");
                return *this;
            }
        }
        else {
            value = std::string(translatedContents);
            translatedContents = NULL;
        }
    }
    if (append == RPLIB_APPEND) {
        contents = scew_element_contents(retNode);
        if (contents != NULL) {
            tmpVal = std::string(contents);
            value = tmpVal + value;
        }
    }
    scew_element_set_contents(retNode,value.c_str());
    return *this;
}

/**********************************************************************/
// METHOD: put()
/// Put a double value into the xml.
/**
 */

RpLibrary&
RpLibrary::put (std::string path, double value, std::string id,
                unsigned int append)
{
    std::stringstream valStr;

    if (this->root == NULL) {
        // library doesn't exist, do nothing;
        status.error("invalid library object");
        status.addContext("RpLibrary::put() - putDouble");
        return *this;
    }

    valStr << value;

    put(path,valStr.str(),id,append);
    return *this;
}

/**********************************************************************/
// METHOD: put()
/// Put a RpLibrary* value into the xml. This is used by copy()
/**
 *  Append flag adds additional nodes, it does not merge same 
 *  named nodes together
 */

RpLibrary&
RpLibrary::put (    std::string path,
                    RpLibrary* value,
                    std::string id,
                    unsigned int append )
{
    scew_element *retNode   = NULL;
    scew_element *new_elem  = NULL;
    scew_element *childNode = NULL;
    scew_element *tmpNode   = NULL;
    const char *contents    = NULL;
    int deleteTmpNode       = 0;

    if (this->root == NULL) {
        // library doesn't exist, do nothing;
        status.error("invalid library object");
        status.addContext("RpLibrary::put()");
        return *this;
    }

    if (value == NULL) {
        // you cannot put a null RpLibrary into the tree
        // user specified a null value
        status.error("user specified NULL value");
        status.addContext("RpLibrary::put()");
        return *this;
    }

    if (value->root == NULL) {
        // you cannot put a null RpLibrary into the tree
        // user specified a null value
        status.error("user specified uninitialized RpLibrary object");
        status.addContext("RpLibrary::put()");
        return *this;
    }

    tmpNode = value->root;

    if (append == RPLIB_OVERWRITE) {
        retNode = _find(path,NO_CREATE_PATH);
        if (retNode) {
            // compare roots to see if they are part of the
            // same xml tree, if so, make a tmp copy of the
            // tree to be copied before freeing it.
            if (_checkPathConflict(retNode,tmpNode)) {
                tmpNode = scew_element_copy(tmpNode);
                deleteTmpNode = 1;
            }
            contents = scew_element_contents(tmpNode);
            if (contents) {
                scew_element_set_contents(retNode, "");
            }

            while ( (childNode = scew_element_next(retNode,childNode)) ) {
                scew_element_free(childNode);
            }
        }
        else {
            // path did not exist and was not created
            // do nothing
        }
    }

    retNode = _find(path,CREATE_PATH);

    if (retNode) {
        contents = scew_element_contents(tmpNode);
        if (contents) {
            scew_element_set_contents(retNode, contents);
        }

        while ( (childNode = scew_element_next(tmpNode,childNode)) ) {
            if ((new_elem = scew_element_copy(childNode))) {
                if (scew_element_add_elem(retNode, new_elem)) {
                    // maybe we want to count the number of children
                    // that we have successfully added?
                }
                else {
                    // adding new element failed
                    status.error("error while adding child node");
                    status.addContext("RpLibrary::put()");
                }
            }
            else {
                // copying new element failed
                status.error("error while copying child node");
                status.addContext("RpLibrary::put()");
            }
        }

        if (deleteTmpNode == 1) {
            scew_element_free(tmpNode);
        }
    }
    else {
        // path did not exist and was not created.
        status.error("error while creating child node");
        status.addContext("RpLibrary::put()");
    }

    return *this;
}


/**********************************************************************/
// METHOD: putData()
/// Put a data from a buffer into the xml.
/**
 *  Append flag adds additional nodes, it does not merge same 
 *  named nodes together
 */

RpLibrary&
RpLibrary::putData (std::string path,
                    const char* bytes,
                    int nbytes,
                    unsigned int append  )
{
    scew_element* retNode = NULL;
    const char* contents = NULL;
    Rappture::Buffer inData;
    unsigned int bytesWritten = 0;
    size_t flags = 0;

    status.addContext("RpLibrary::putData()");
    if (!this->root) {
        // library doesn't exist, do nothing;
        return *this;
    }
    retNode = _find(path,CREATE_PATH);

    if (retNode == NULL) {
        status.addError("can't create node from path \"%s\"", path.c_str());
        return *this;
    }
    if (append == RPLIB_APPEND) {
        if ( (contents = scew_element_contents(retNode)) ) {
            inData.append(contents);
            // base64 decode and un-gzip the data
            if (!Rappture::encoding::decode(status, inData, 0)) {
                return *this;
            }
        }
    }
    if (inData.append(bytes, nbytes) != nbytes) {
        status.addError("can't append %d bytes", nbytes);
        return *this;
    }
    // gzip and base64 encode the data
    flags = RPENC_Z|RPENC_B64|RPENC_HDR;
    if (!Rappture::encoding::encode(status, inData,flags)) {
        return *this;
    }
    bytesWritten = (unsigned int) inData.size();
    scew_element_set_contents_binary(retNode,inData.bytes(),&bytesWritten);
    return *this;
}


/**********************************************************************/
// METHOD: putFile()
/// Put data from a file into the xml.
/**
 *  Append flag adds additional nodes, it does not merge same
 *  named nodes together
 */

RpLibrary&
RpLibrary::putFile(std::string path, std::string fileName,
                   unsigned int compress, unsigned int append)
{
    Rappture::Buffer buf;
    Rappture::Buffer fileBuf;

    if (!this->root) {
        // library doesn't exist, do nothing;
        return *this;
    }

    if (!fileBuf.load(status, fileName.c_str())) {
        fprintf(stderr, "error loading file: %s\n", status.remark());
        status.addContext("RpLibrary::putFile()");
        return *this;
    }
    if ((compress == RPLIB_COMPRESS) ||
	(Rappture::encoding::isBinary(fileBuf.bytes(), fileBuf.size()))) {	
        putData(path, fileBuf.bytes(), fileBuf.size(), append);
    } else {
        /* Always append a NUL-byte to the end of ASCII strings. */
        fileBuf.append("\0", 1);
        put(path, fileBuf.bytes(), "", append, RPLIB_TRANSLATE);
    }
    return *this;
}


/**********************************************************************/
// METHOD: remove()
/// Remove the provided path from this RpLibrary
/**
 */

RpLibrary*
RpLibrary::remove ( std::string path )
{
    scew_element* ele = NULL;
    int setNULL = 0;
    RpLibrary* retLib = NULL;

    if (!this->root) {
        // library doesn't exist, do nothing;
        return NULL;
    }

    if ( !path.empty() ) {
        ele = _find(path,NO_CREATE_PATH);
    }
    else {
        // telling this function to remove "" is essentially destroying
        // the object. most functions will fail after a call like this.
        ele = this->root;
        setNULL++;
    }

    if (ele) {
        scew_element_free(ele);
        if (setNULL != 0) {
            // this is the case where user specified an empty path.
            // the object is useless, and will be deleted.
            this->root = NULL;
            retLib = NULL;
        }
        else {
            retLib = this;
        }
    }

    return retLib;
}

/**********************************************************************/
// METHOD: xml()
/// Return the xml text held in this RpLibrary
/**
 */

std::string
RpLibrary::xml () const
{
    std::stringstream outString;

    if (!this->root) {
        // library doesn't exist, do nothing;
        return std::string("");
    }

    outString << "<?xml version=\"1.0\"?>\n";
    print_element(this->root, 0, outString);

    return outString.str();
}

/**********************************************************************/
// METHOD: nodeType()
/// Return the type name of this node
/**
 */

std::string
RpLibrary::nodeType () const
{
    if (!this->root) {
        // library doesn't exist, do nothing;
        return std::string("");
    }

    return std::string(scew_element_name(root));
}

/**********************************************************************/
// METHOD: nodeId()
/// Return the id of this node.
/**
 */

std::string
RpLibrary::nodeId () const
{
    if (!this->root) {
        // library doesn't exist, do nothing;
        return std::string("");
    }

    return _node2name(root);
}

/**********************************************************************/
// METHOD: nodeComp()
/// Return the component name of this node.
/**
 */

std::string
RpLibrary::nodeComp () const
{
    if (!this->root) {
        // library doesn't exist, do nothing;
        return std::string("");
    }

    return _node2comp(root);
}

/**********************************************************************/
// METHOD: nodePath()
/// Return the component name of this node's path.
/**
 */

std::string
RpLibrary::nodePath () const
{
    if (!this->root) {
        // library doesn't exist, do nothing;
        return std::string("");
    }

    return _node2path(root);
}

/**********************************************************************/
// METHOD: outcome()
/// Return the status object of this library object.
/**
 */

Rappture::Outcome&
RpLibrary::outcome() const
{
    return status;
}

/*
 * ----------------------------------------------------------------------
 *  METHOD: result
 *
 *  Clients call this function at the end of their simulation, to
 *  pass the simulation result back to the Rappture GUI.  It writes
 *  out the given XML object to a runXXX.xml file, and then writes
 *  out the name of that file to stdout.
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2007
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */
void
RpLibrary::result(int exitStatus)
{
    std::fstream file;
    std::string xmlText = "";
    time_t t;
    struct tm* timeinfo;
    std::stringstream outputFile;
    std::string timestamp;
    std::string username;
    std::string hostname;
    char *user = NULL;

    if (this->root == NULL) {
	return;				/* No tree available */
    }

    t = time(NULL);			/* This is presumably the time the
					 * simulation finished. */
#ifdef HAVE_GETTIMEOFDAY
    /* If the posix function gettimeofday is available, use it to produce
     * unique filenames. */
    struct timeval tv;
    gettimeofday(&tv,NULL);
    outputFile << "run" << tv.tv_sec << tv.tv_usec << ".xml";
#else
    outputFile << "run" << (int)t << ".xml";
#endif
    file.open(outputFile.str().c_str(),std::ios::out);
    
    
    put("tool.version.rappture.version", RAPPTURE_VERSION);
    put("tool.version.rappture.revision", SVN_VERSION);
    put("tool.version.rappture.modified",
	"$LastChangedDate$");
    if ( "" == get("tool.version.rappture.language") ) {
	put("tool.version.rappture.language","c++");
    }
    // generate a timestamp for the run file
    timeinfo = localtime(&t);
    timestamp = std::string(ctime(&t));
    // erase the 24th character because it is a newline
    timestamp.erase(24);
    // concatenate the timezone
    timestamp.append(" ");
#ifdef _WIN32
    timestamp.append(_tzname[_daylight]);
    username = "";
    hostname = "";
    user = getenv("USERNAME");
    if (user != NULL) {
	username = std::string(user);
    } else {
	user = getenv("LOGNAME");
	if (user != NULL) {
	    username = std::string(user);
	}
    }
#else
    timestamp.append(timeinfo->tm_zone);
    user = getenv("USER");
    if (user != NULL) {
	username = std::string(user);
    } else {
	user = getenv("LOGNAME");
	if (user != NULL) {
	    username = std::string(user);
	}
    }
#endif

    // add the timestamp to the run file
    put("output.time", timestamp);
    put("output.status",exitStatus);
    put("output.user",username);
    put("output.host",hostname);
    
    if ( file.is_open() ) {
	xmlText = xml();
	if (!xmlText.empty()) {
	    file << xmlText;
	}
	// check to make sure there were no
	// errors while writing the run.xml file.
	if (   (!file.good())
	       || ((long)xmlText.length() != ((long)file.tellp()-(long)1))
               ) {
	    status.error("Error while writing run file");
	    status.addContext("RpLibrary::result()");
	}
	file.close();
    } else {
	status.error("Error while opening run file");
	status.addContext("RpLibrary::result()");
    }
    std::cout << "=RAPPTURE-RUN=>" << outputFile.str() << std::endl;
}

/**********************************************************************/
// METHOD: print_indent()
/// Add indentations to the requested stringstream object.
/**
 */

void
RpLibrary::print_indent(    unsigned int indent,
                            std::stringstream& outString ) const
{

    // keep this around incase you want to use tabs instead of spaces
    // while ( (indent--) > 0)
    // {
    //     outString << "\t";
    // }

    // keep this around incase you want to use spaces instead of tabs
    int cnt = indent*INDENT_SIZE;
    while ( (cnt--) > 0)
    {
        outString << " ";
    }

}

/**********************************************************************/
// METHOD: print_attributes()
/// Print the attribute names and values for the provided xml node
/**
 */

void
RpLibrary::print_attributes(    scew_element* element,
                                std::stringstream& outString ) const
{
    scew_attribute* attribute = NULL;

    if (element != NULL)
    {
        if (scew_attribute_count(element) > 0) {
            /**
             * Iterates through the element's attribute list, printing the
             * pair name-value.
             */
            attribute = NULL;
            while((attribute=scew_attribute_next(element, attribute)) != NULL)
            {
                outString << " " << scew_attribute_name(attribute) << "=\"" <<
                       scew_attribute_value(attribute) << "\"";
            }
        }
    }
}


/**********************************************************************/
// METHOD: print_element()
/// Print the value of the node and its attributes to a stringstream object
/**
 */

void
RpLibrary::print_element(   scew_element* element,
                            unsigned int indent,
                            std::stringstream& outString    ) const
{
    scew_element* child = NULL;
    XML_Char const* contents = NULL;

    if (element == NULL)
    {
        return;
    }

    /**
     * Prints the starting element tag with its attributes.
     */
    print_indent(indent, outString);
    outString << "<" << scew_element_name(element);
    print_attributes(element,outString);
    outString << ">";
    contents = scew_element_contents(element);
    if (contents == NULL)
    {
        outString << "\n";
    }

    /**
     * Call print_element function again for each child of the
     * current element.
     */
    child = NULL;
    while ((child = scew_element_next(element, child)) != NULL)
    {
        print_element(child, indent + 1, outString);
    }

    /* Prints element's content. */
    if (contents != NULL)
    {
        outString << contents;
    }
    else
    {
        print_indent(indent, outString);
    }

    /**
     * Prints the closing element tag.
     */
    outString << "</" << scew_element_name(element) << ">\n" ;
}
