/*
 * ----------------------------------------------------------------------
 *  Rappture Library Source
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
#include "RpEntityRef.h"

static RpEntityRef ERTranslator;

/**********************************************************************/
// METHOD: _get_attribute()
/// Return the attribute value matching the provided attribute name.
/**
 */

std::string
RpLibrary::_get_attribute(scew_element* element, std::string attributeName)
{
    scew_attribute* attribute = NULL;
    std::string attrVal;
    // int attrValSize = 0;

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
RpLibrary::_path2list (std::string& path, std::string** list, int listLen) 
{
    std::string::size_type pos = 0;
    std::string::size_type start = 0;
    int index = 0;
    int retVal = 0;

    // listLen should be the highest index + 1
    for (   pos = path.find(".",NO_CREATE_PATH);
            (pos != std::string::npos) || (index >= listLen);
            pos = path.find(".",pos)   )
    {
        list[index++] = new std::string(path.substr(start,pos-start));
        start = ++pos;
    }

    // add the last path to the list
    if (index < listLen) {
        // error checking for path names?
        // what if you get something like p1.p2. ?
        list[index] = new std::string(path.substr(start,path.length()-start));
    }

    retVal = index++;

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
RpLibrary::_node2name (scew_element* node)
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
RpLibrary::_node2comp (scew_element* node)
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
RpLibrary::_node2path (scew_element* node)
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
RpLibrary::_splitPath (std::string& path, std::string& tagName, int* idx, std::string& id )
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
RpLibrary::_find(std::string path, int create) 
{
    // std::string* tagName;
    // std::string* id;
    std::string tagName = "";
    std::string id = "";
    int index = 0;
    int listLen = (path.length()/2)+1;
    std::string* list[listLen];
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
        //user gave an empty path
        // return this;
        return tmpElement;
    }

    if (!list) {
        // error calloc'ing space for list
        return NULL;
    }

    path_size = _path2list (path,list,listLen);

    while ( (listIdx <= path_size) && (tmpElement != NULL ) ){

        _splitPath(*(list[listIdx]),tagName,&index,id);

        // if (id->empty()) {
        if (id.empty()) {
            /*
            # If the name is like "type2", then look for elements with
            # the type name and return the one with the given index.
            # If the name is like "type", then assume the index is 0.
            */

            // eleList = scew_element_list(tmpElement, tagName->c_str(), &count);
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

            // eleList = scew_element_list(tmpElement, tagName->c_str(), &count);
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
                    // if (*id == tmpId) {
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
                // return node;
                tmpElement = node;
                break;
            }
            else {
                // create == CREATE_PATH
                // we should create the rest of the path

                // create the new element
                // need to figure out how to properly number the new element
                // node = scew_element_add(tmpElement,tagName->c_str());
                node = scew_element_add(tmpElement,tagName.c_str());
                if (! node) {
                    // a new element was not created 
                }

                // add an attribute and attrValue to the new element
                // if (id && !id->empty()) {
                    // scew_element_add_attr_pair(node,"id",id->c_str());
                if (!id.empty()) {
                    scew_element_add_attr_pair(node,"id",id.c_str());
                }
            }
        }


        // change this so youre not allocating and deallocating so much.
        // make it static or something.
        // if (tagName)    { delete (tagName); }
        // if (id)         { delete (id); }
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

        // should we really be freeing list since its on the stack as a fxn variable?
        // free(list);
        // list = NULL;
    }


    return tmpElement;
}


/**********************************************************************/
// METHOD: element()
/// Search the path of a xml tree and return a RpLibrary node.
/**
 * It is the user's responsibility to delete the object when 
 * they are finished using it?, else i need to make this static
 */

RpLibrary*
RpLibrary::element (std::string path)
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
RpLibrary::entities  (std::string path)
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
RpLibrary::diff (RpLibrary* otherLib, std::string path)
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
        retList.push_back(otherVal.front());
        retList.push_back("");

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
RpLibrary::value (std::string path)
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
RpLibrary::parent (std::string path)
{
    RpLibrary* retLib = NULL;
    std::string parentPath = "";
    // std::string::size_type pos = 0;
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
RpLibrary::copy (std::string toPath, std::string fromPath, RpLibrary* fromObj)
{
    RpLibrary* value = NULL;
    // RpLibrary* child = NULL;

    if (!this->root) {
        // library doesn't exist, do nothing;
        // need a good way to raise error, and this is not it.
        return *this;
    }

    if (fromObj == NULL) {
        fromObj = this;
    }

    value = fromObj->element(fromPath);

    if ( !value ) {
        // need a good way to raise error, and this is not it.
        return *this;
    }

    this->put(toPath, value);
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
// METHOD: children()
/// Returns a std::list<RpLibrary*> of all children under 'path'
//
// 
/**
 */

RpLibrary&
RpLibrary::childCount ( std::string path,
                        int* childCount)
{
    scew_element* parentNode;
    int myChildCount = 0;

    if (this->root) {

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
RpLibrary::isNull ()
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
RpLibrary::get (std::string path, int translateFlag)
{
    return (this->getString(path, translateFlag));
}

/**********************************************************************/
// METHOD: getString()
/// Return the string value of the object held at location 'path'
/**
 */

std::string
RpLibrary::getString (std::string path, int translateFlag)
{
    scew_element* retNode = NULL;
    XML_Char const* retCStr = NULL;
    char* translatedContents = NULL;
    std::string retStr = "";
    int len = 0;

    if (!this->root) {
        // library doesn't exist, do nothing;
        return std::string("");
    }

    retNode = _find(path,NO_CREATE_PATH);

    if (retNode == NULL) {
        // need to raise error
        return std::string("");
    }

    retCStr = scew_element_contents(retNode);

    if (!retCStr) {
        return std::string("");
    }

    if (translateFlag == TRANSLATE) {
        // translatedContents = new char[strlen(retCStr)+1];
        len = strlen(retCStr);
        translatedContents = (char*) calloc((len+1),sizeof(char));
        if (translatedContents) {
            strncpy(translatedContents, retCStr, len);
            _translateOut(translatedContents);
            retStr = std::string(translatedContents);
            // delete[] translatedContents;
            free(translatedContents);
        }
        else {
            // error allocating space
            return std::string("");
        }
    }
    else {
        retStr = std::string(retCStr);
    }

    return retStr;
}

/**********************************************************************/
// METHOD: getDouble()
/// Return the double value of the object held at location 'path'
/**
 */

double
RpLibrary::getDouble (std::string path)
{
    std::string retValStr = "";
    double retValDbl = 0;

    if (!this->root) {
        // library doesn't exist, do nothing;
        return retValDbl;
    }

    retValStr = this->getString(path); 
    // think about changing this to strtod()
    retValDbl = atof(retValStr.c_str());

    // how do we raise error?
    // currently we depend on getString to raise the error
    return retValDbl;
}


/**********************************************************************/
// METHOD: put()
/// Put a string value into the xml.
/**
 */

RpLibrary&
RpLibrary::put (    std::string path, 
                    std::string value, 
                    std::string id, 
                    int append,
                    int translateFlag)
{
    scew_element* retNode = NULL;
    std::string tmpVal = "";
    const char* contents = NULL;
    std::string translatedContents = "";

    if (!this->root) {
        // library doesn't exist, do nothing;
        return *this;
    }

    retNode = _find(path,CREATE_PATH);

    if (retNode) {

        if (append) {
            if ( (contents = scew_element_contents(retNode)) ) {
                tmpVal = std::string(contents);
            }
            value = tmpVal + value;
        }

        if (translateFlag == TRANSLATE) {
            _translateIn(value,translatedContents);
            scew_element_set_contents(retNode,translatedContents.c_str());
        }
        else {
            scew_element_set_contents(retNode,value.c_str());
        }
    }

    return *this;
}

/**********************************************************************/
// METHOD: put()
/// Put a double value into the xml.
/**
 */

RpLibrary&
RpLibrary::put ( std::string path, double value, std::string id, int append )
{
    std::stringstream valStr;

    if (!this->root) {
        // library doesn't exist, do nothing;
        return *this;
    }

    valStr << value;

    return this->put(path,valStr.str(),id,append);
}

/**********************************************************************/
// METHOD: put()
/// Put a RpLibrary* value into the xml.
/**
 *  Append flag adds additional nodes, it does not merge same 
 *  named nodes together
 */

RpLibrary&
RpLibrary::put ( std::string path, RpLibrary* value, std::string id, int append )
{
    scew_element* retNode   = NULL;
    // scew_element* old_elem  = NULL;
    scew_element* new_elem  = NULL;
    scew_element* childNode = NULL;
    // std::string nodeName    = "";

    int retVal = 1;

    if (!this->root) {
        // library doesn't exist, do nothing;
        return *this;
    }

    // you cannot put a null RpLibrary into the tree
    if (!value) {
        // need to send back an error saying that user specified a null value
        return *this;
    }

    // nodeName = value->nodeComp();
    // old_elem = _find(path+"."+nodeName,NO_CREATE_PATH);

    if (!append) {
        retNode = _find(path,NO_CREATE_PATH);
        if (retNode) {
            scew_element_free(retNode);
        }
        else {
            // path did not exist and was not created
            // do nothing
        }
    }

    retNode = _find(path,CREATE_PATH);

    if (retNode) {
        while ( (childNode = scew_element_next(value->root,childNode)) ) {
            if ((new_elem = scew_element_copy(childNode))) {
                if (scew_element_add_elem(retNode, new_elem)) {
                    // maybe we want to count the number of children
                    // that we have successfully added?
                    retVal = 0;
                }
                else {
                    // adding new element failed
                }
            }
            else {
                // copying new element failed
            }
        }
    }
    else {
        // path did not exist and was not created.
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
RpLibrary::xml ()
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
RpLibrary::nodeType ()
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
RpLibrary::nodeId ()
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
RpLibrary::nodeComp ()
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
RpLibrary::nodePath ()
{
    if (!this->root) {
        // library doesn't exist, do nothing;
        return std::string("");
    }

    return _node2path(root);
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
 *  Copyright (c) 2004-2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */
void
RpLibrary::result() {
    std::stringstream outputFile;
    std::fstream file;
    std::string xmlText = "";
    time_t t = 0;

    if (this->root) {
        outputFile << "run" << (int)time(&t) << ".xml";
        file.open(outputFile.str().c_str(),std::ios::out);

        if ( file.is_open() ) {
            xmlText = xml();
            if (!xmlText.empty()) {
                file << xmlText;
            }
        }
        std::cout << "=RAPPTURE-RUN=>" << outputFile.str() << std::endl;
    }
}

/**********************************************************************/
// METHOD: print_indent()
/// Add indentations to the requested stringstream object.
/**
 */

void
RpLibrary::print_indent(unsigned int indent, std::stringstream& outString)
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
RpLibrary::print_attributes(scew_element* element, std::stringstream& outString)
{
    scew_attribute* attribute = NULL;

    if (element != NULL)
    {
        /**
         * Iterates through the element's attribute list, printing the
         * pair name-value.
         */
        attribute = NULL;
        while ((attribute = scew_attribute_next(element, attribute)) != NULL)
        {
            outString << " " << scew_attribute_name(attribute) << "=\"" <<
                   scew_attribute_value(attribute) << "\"";
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
                            std::stringstream& outString    )
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

/**********************************************************************/
// METHOD: translateIn()
/// Translate entity and character reference text coming in from the user
/**
 */

int
RpLibrary::_translateIn(std::string& outStr,std::string& translatedStr)
{
    ERTranslator.appendEscaped(outStr, translatedStr);
    return 0;
}


/**********************************************************************/
// METHOD: translateOut()
/// Translate entity and character reference text going out to the user
/**
 */

int
RpLibrary::_translateOut(char* inStr) 
{
    int newLen = 0;
    ERTranslator.TranslateEntityRefs(inStr, &newLen);
    return 0;
}
