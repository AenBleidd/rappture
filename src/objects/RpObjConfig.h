/*
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef RAPPTURE_OBJECT_CONFIG_H
#define RAPPTURE_OBJECT_CONFIG_H

namespace Rappture {

enum RapptureConfigureFlags {
    RPCONFIG_XML    =(1<<0),       /* Configure Object from Rappture 1.0 XML. */
    RPCONFIG_TREE   =(1<<1),       /* Configure Object from Rappture 1.5 Tree. */
};

typedef struct {
    size_t indent;
    size_t tabstop;
    const char *retStr;
} ClientDataXml;

} // end namespace Rappture

#endif // RAPPTURE_OBJECT_CONFIG_H
