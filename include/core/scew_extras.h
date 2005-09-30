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

#ifdef __cplusplus 
}
#endif

