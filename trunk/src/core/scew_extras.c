/*
 * ----------------------------------------------------------------------
 *  Extra helper functions for the scew library
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "scew/scew.h"
#include "scew/xelement.h"
#include "scew/xattribute.h"
#include "scew/xerror.h"
#include "scew/str.h"
#include "scew_extras.h"
#include <assert.h>

scew_element*
scew_element_parent(scew_element const* element)
{
    assert(element != NULL);
    return element->parent;
}

scew_element**
scew_element_list_all(scew_element const* parent, unsigned int* count)
{
    unsigned int curr = 0;
    unsigned int max = 0;
    scew_element** list = NULL;
    scew_element* element;

    assert(parent != NULL);
    assert(count != NULL);

    element = scew_element_next(parent, 0);
    while (element)
    {
        if (curr >= max)
        {
            max = (max + 1) * 2;
            list = (scew_element**) realloc(list,
                                            sizeof(scew_element*) * max);
            if (!list)
            {
                set_last_error(scew_error_no_memory);
                return NULL;
            }
        }
        list[curr++] = element;

        element = scew_element_next(parent, element);
    }

    *count = curr;

    return list;
}

int
scew_element_copy_attr(scew_element const* fromElement, scew_element* toElement)
{
    scew_attribute* attribute = NULL;
    XML_Char const* attrVal   = NULL;
    XML_Char const* attrName  = NULL;
    int attrCount = 0;
    int attrAdded = 0;

    if ( (fromElement) && (toElement) && (fromElement != toElement) )
    {
        if ((attrCount = scew_attribute_count(fromElement)) > 0) {

            while((attribute=scew_attribute_next(fromElement, attribute)) != NULL)
            {
                attrName = scew_attribute_name(attribute),
                attrVal = scew_attribute_value(attribute);
                if (scew_element_add_attr_pair(toElement, attrName, attrVal)) {
                    attrAdded++;
                }
            }
        }
        else {
            // there are no attributes, count == 0
        }
    }

    return (attrCount - attrAdded);
}

scew_element*
scew_element_copy (scew_element* element)
{
    XML_Char const* name = NULL;
    XML_Char const* contents = NULL;
    scew_element* new_elem = NULL;
    scew_element* new_child = NULL;
    scew_element* childNode = NULL;

    name = scew_element_name(element);
    contents = scew_element_contents(element);

    new_elem = scew_element_create(name);
    scew_element_set_name(new_elem,name);
    if (contents) {
        scew_element_set_contents(new_elem, contents);
    }
    scew_element_copy_attr(element,new_elem);

    while ( (childNode = scew_element_next(element,childNode)) ) {
        new_child = scew_element_copy(childNode);
        scew_element_add_elem(new_elem, new_child);
    }

    return new_elem;
}

XML_Char const*
scew_element_set_contents_binary(scew_element* element,
                                 XML_Char const* bytes,
                                 unsigned int* nbytes)
{
    XML_Char* out = NULL;

    assert(element != NULL);
    assert(nbytes != NULL);
    if (*nbytes == 0) {
        return element->contents;
    }
    assert(bytes != NULL);

    free(element->contents);
    out = (XML_Char*) calloc(*nbytes+1, sizeof(XML_Char));
    element->contents = (XML_Char*) scew_memcpy(out, (XML_Char*)bytes, *nbytes);

    return element->contents;
}
