/*
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "RpTree.h"
#include "RpChain.h"

#ifndef RAPPTURE_PARSER_XML_H
#define RAPPTURE_PARSER_XML_H

#define PARSERXML_LEVEL_WIDTH 4

enum RapptureParserXmlFlags {
    RPXML_CREATE=(1<<0),    /* Create the xml path if needed. */
    RPXML_APPEND=(1<<1),    /* Append data to the data already available. */
};

extern const char *Rp_ParserXml_Field_ID;

typedef struct Rp_ParserXmlStruct Rp_ParserXml;

Rp_ParserXml *Rp_ParserXmlCreate();
void Rp_ParserXmlDestroy(Rp_ParserXml **p);

void Rp_ParserXmlParse(Rp_ParserXml *p, const char *xml);
int Rp_ParserXmlNodeIdentify(Rp_ParserXml *p, Rp_TreeNode n,
    const char *name, const char *id);
void Rp_ParserXmlFindChild( Rp_ParserXml *p, Rp_TreeNode parent,
    const char *childName, const char *childId, size_t degree,
    Rp_TreeNode *child, size_t *numFound);
Rp_TreeNode Rp_ParserXmlSearch( Rp_ParserXml *p, const char *path, int create);
const char *Rp_ParserXmlGet(Rp_ParserXml *p, const char *path);
void Rp_ParserXmlPut(Rp_ParserXml *p, const char *path, const char *val, int append);
void Rp_ParserXmlPutF(Rp_ParserXml *p, const char *path, const char *val, ...);
void Rp_ParserXmlAppendF(Rp_ParserXml *p, const char *path, const char *val, ...);

Rp_Tree Rp_ParserXmlTreeClient(Rp_ParserXml *p);

Rp_TreeNode Rp_ParserXmlElement(Rp_ParserXml *p, const char *path);
Rp_TreeNode Rp_ParserXmlParent(Rp_ParserXml *p, const char *path);
size_t Rp_ParserXmlChildren(Rp_ParserXml *p, const char *path,
    const char *type, Rp_Chain *children);
size_t Rp_ParserXmlNumberChildren(Rp_ParserXml *p, const char *path,
    const char *type);
void Rp_ParserXmlBaseNode(Rp_ParserXml *p, Rp_TreeNode node);

const char *Rp_ParserXmlNodePath(Rp_ParserXml *p, Rp_TreeNode node);
const char *Rp_ParserXmlNodeId(Rp_ParserXml *p, Rp_TreeNode node);

const char *Rp_ParserXmlXml(Rp_ParserXml *p);
const char *Rp_ParserXmlPathVal(Rp_ParserXml *p);

#endif // RAPPTURE_PARSER_XML_H
