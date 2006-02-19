/*
 * ======================================================================
 *  Copyright (c) 2004-2005  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifdef __cplusplus
extern "C" {
#endif

extern scew_element*
scew_element_parent(scew_element const* element);

/**
 * Returns a list of elements that matches the name specified. The
 * number of elements is returned in count. The returned pointer must be
 * freed after using it, calling the <code>scew_element_list_free</code>
 * function.
 */
extern scew_element**
scew_element_list_all(scew_element const* parent, unsigned int* count);

extern int
scew_element_copy_attr(scew_element const* fromElement, scew_element* toElement);

extern scew_element*
scew_element_copy (scew_element* element);

#ifdef __cplusplus
}
#endif

