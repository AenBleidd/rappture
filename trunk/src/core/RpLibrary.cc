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
    for (   pos = path.find(".",0);
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
        else {
            // count == 0 ???
        }
        scew_element_list_free(siblings);

    }
    else {
        // node has attribute id
        //

        // retVal = scew_strjoinXX(type,name);
        retVal << type << "(" << name << ")";

    }

    return (retVal.str());
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
            if (create == 0) {
                // break out instead of returning because we still need to
                // free the list variable
                // return node;
                tmpElement = node;
                break;
            }
            else {
                // create == 1
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
 */

RpLibrary*
RpLibrary::element (std::string path)
{
    RpLibrary* retLib; 

    if (path.empty()) {
        // an empty path returns the current RpLibrary
        return this;
    }

    scew_element* retNode = _find(path,0);

    if (retNode == NULL) {
        // add error information as to why we are returning NULL
        return NULL;
    }

    retLib = new RpLibrary( retNode ); 

    if (!retLib) {
        // add error information as to why we are returning NULL
        /* there was an error mallocing memory for a RpLibrary object */
        return NULL;
    }

    return retLib;
}

/**********************************************************************/
// METHOD: children()
/// Return the next child of the node located at 'path'
//
// The lookup is reset when you send a NULL rpChilNode.
// 
/**
 */

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
            parentNode = _find(path,0);
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

    if ( (childNode = scew_element_next(parentNode,childNode)) ) {

        if (!type.empty()) {
            childName = scew_element_name(childNode);
            // we are searching for a specific child name
            // keep looking till we find a name that matches the type.
            while (type != childName) {
                childNode = scew_element_next(parentNode,childNode);
                childName = scew_element_name(childNode);
            }
            if (type == childName) {
                // found a child with a name that matches type
                // clean up old memory
                delete retLib;
                retLib = new RpLibrary( childNode );
            }
            else {
                // no children with names that match 'type' were found
                delete retLib;
                retLib = NULL;
            }
        }
        else {
            // clean up old memory
            delete retLib;
            retLib = new RpLibrary( childNode );
        }
    }
    else {
        // somthing happened within scew, get error code and display
        // its probable there are no more child elements left to report
        // clean up old memory
        delete retLib;
        retLib = NULL;
    }

    return retLib;
}

/**********************************************************************/
// METHOD: get()
/// Return the string value of the object held at location 'path'
/**
 */

std::string
RpLibrary::get (std::string path)
{
    return (this->getString(path));
}

/**********************************************************************/
// METHOD: getString()
/// Return the string value of the object held at location 'path'
/**
 */

std::string
RpLibrary::getString (std::string path)
{
    scew_element* retNode = _find(path,0);
    XML_Char const* retCStr = NULL;

    if (retNode == NULL) {
        // need to raise error
        return std::string("");
    }

    retCStr = scew_element_contents(retNode);

    if (!retCStr) {
        return std::string("");
    }

    return std::string(retCStr);
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
RpLibrary::put ( std::string path, std::string value, std::string id, int append )
{
    scew_element* retNode = NULL;
    std::string tmpVal = "";
    const char* contents = NULL;

    if (! path.empty()) {
        retNode = _find(path,1);

        if (retNode) {

            if (append) {
                if ( (contents = scew_element_contents(retNode)) ) {
                    tmpVal = std::string(contents);
                }
                value = tmpVal + value;
            }

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

    valStr << value;

    return this->put(path,valStr.str(),id,append);
}

/*
RpLibrary&
RpLibrary::remove ( std::string path )
{
    scew_element* ele = _find(path,0);
    RpLibrary* retLib;
    if (ele) {
        retLib = new RpLibrary(ele)
    }

    return *retLib
}
*/

/**********************************************************************/
// METHOD: xml()
/// Return the xml text held in this RpLibrary
/**
 */

std::string
RpLibrary::xml ()
{
    std::stringstream outString;

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
    return _node2comp(root);
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

