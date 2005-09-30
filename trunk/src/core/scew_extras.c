#include "scew.h"

#include "xelement.h"
#include "xerror.h"

#include <assert.h>

#include "scew_extras.h"

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
