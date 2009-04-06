/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 Choice Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpChoice.h"

using namespace Rappture;

Choice::Choice (
            const char *path,
            const char *val
        )
    :   Variable    (),
        _options    (NULL)
{
    this->path(path);
    this->label("");
    this->desc("");
    this->def(val);
    this->cur(val);
}

Choice::Choice (
            const char *path,
            const char *val,
            const char *label,
            const char *desc
        )
    :   Variable    (),
        _options    (NULL)
{
    this->path(path);
    this->label(label);
    this->desc(desc);
    this->def(val);
    this->cur(val);
}

// copy constructor
Choice::Choice ( const Choice& o )
    :   Variable(o)
{
    this->def(o.def());
    this->cur(o.cur());

    // need to copy _options
}

// default destructor
Choice::~Choice ()
{
    // clean up dynamic memory
    // unallocate the _options?

}

/**********************************************************************/
// METHOD: addOption()
/// Add an option value to the object
/**
 * Add an option value to the object. Currently all
 * labels must be unique.
 */

Choice&
Choice::addOption(
    const char *label,
    const char *desc,
    const char *val)
{
    option *p = NULL;

    p = new option;
    if (!p) {
        // raise error and exit
    }

    p->label(label);
    p->desc(desc);
    p->val(val);

    if (_options == NULL) {
        _options = Rp_ChainCreate();
        if (_options == NULL) {
            // raise error and exit
        }
    }

    Rp_ChainAppend(_options,p);

    return *this;
}

/**********************************************************************/
// METHOD: delOption()
/// Delete an option value from the object
/**
 * Delete an option value from the object.
 */

Choice&
Choice::delOption(const char *label)
{
    if (label == NULL) {
        return *this;
    }

    if (_options == NULL) {
        return *this;
    }

    option *p = NULL;
    const char *plabel = NULL;
    Rp_ChainLink *l = NULL;

    // traverse the list looking for the matching option
    l = Rp_ChainFirstLink(_options);
    while (l != NULL) {
        p = (option *) Rp_ChainGetValue(l);
        plabel = p->label();
        if ((*label == *plabel) && (strcmp(plabel,label) == 0)) {
            // we found matching entry, remove it
            if (p) {
                delete p;
                Rp_ChainDeleteLink(_options,l);
            }
            break;
        }
    }


    return *this;
}

// -------------------------------------------------------------------- //

