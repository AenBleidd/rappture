/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 Choice Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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
    :   Object    (),
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
    :   Object    (),
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
    :   Object(o)
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

/**********************************************************************/
// METHOD: xml()
/// view this object's xml
/**
 * View this object as an xml element returned as text.
 */

const char *
Choice::xml(size_t indent, size_t tabstop)
{
    size_t l1width = indent + tabstop;
    size_t l2width = indent + (2*tabstop);
    size_t l3width = indent + (3*tabstop);
    const char *sp = "";

    Path p(path());
    _tmpBuf.clear();

    _tmpBuf.appendf(
"%7$*4$s<choice id='%1$s'>\n\
%7$*5$s<about>\n\
%7$*6$s<label>%2$s</label>\n\
%7$*6$s<description>%3$s</description>\n\
%7$*5$s</about>\n",
       p.id(),label(),desc(),indent,l1width,l2width,sp);

    Rp_ChainLink *l = NULL;
    l = Rp_ChainFirstLink(_options);
    while (l != NULL) {
        option *op = (option *)Rp_ChainGetValue(l);
        _tmpBuf.appendf(
"%7$*4$s<option>\n\
%7$*5$s<about>\n\
%7$*6$s<label>%1$s</label>\n\
%7$*6$s<description>%2$s</description>\n\
%7$*5$s</about>\n\
%7$*5$s<value>%3$s</value>\n\
%7$*4$s</option>\n",
           op->label(),op->desc(),op->val(),l1width,l2width,l3width,sp);
        l = Rp_ChainNextLink(l);
    }

    _tmpBuf.appendf(
"%5$*4$s<default>%1$s</default>\n\
%5$*4$s<current>%2$s</current>\n\
%5$*3$s</choice>",
       def(),cur(),indent,l1width,sp);

    return _tmpBuf.bytes();
}

/**********************************************************************/
// METHOD: is()
/// what kind of object is this
/**
 * return hex value telling what kind of object this is.
 */

const int
Choice::is() const
{
    // return "choi" in hex
    return 0x63686F69;
}

// -------------------------------------------------------------------- //

