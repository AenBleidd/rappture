/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 XML Parser Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpParserXML.h"
#include "RpSimpleBuffer.h"
#include "RpPath.h"

static const char *Rp_ParserXml_Field_ID = "id";
static const char *Rp_ParserXml_Field_VALUE = "value";
static const char *Rp_ParserXml_Field_VISITED = "visited";

struct Rp_ParserXmlStruct {
    Rp_Tree tree;
    Rp_TreeNode curr;
    Rappture::Path *path;
    Rappture::SimpleCharBuffer *buf;
};


static void XMLCALL
Rp_ParserXmlStartHandler(
    void *data,
    const char *el,
    const char **attr)
{
    Rp_ParserXml *inf = (Rp_ParserXml *) data;
    size_t i = 0;

    inf->curr = Rp_TreeCreateNode(inf->tree, inf->curr, el, -1);

    // store the attributes in the node
    while (attr[i] != NULL) {
        const char *attrName = attr[i++];
        size_t attrValueLen = strlen(attr[i]);
        // FIXME: memory leak originating from this new call
        // I'm not sure where/when the attrValue is deleted yet
        char *attrValue = new char[attrValueLen+1];
        strcpy(attrValue,attr[i++]);
        Rp_TreeSetValue(inf->tree,inf->curr,attrName,(void *)attrValue);
    }
    inf->path->add(el);
}

static void XMLCALL
Rp_ParserXmlEndHandler(
    void *data,
    const char *el)
{
    Rp_ParserXml *inf = (Rp_ParserXml *) data;

    if (inf == NULL) {
        return;
    }

    char *value = NULL;
    Rp_TreeGetValue(inf->tree,inf->curr,Rp_ParserXml_Field_VALUE,(void **)&value);

    // strip trailing spaces
    int i = 0;
    for (i = strlen(value)-1; i >= 0; i--) {
        if (isspace(value[i])) {
            value[i] = '\0';
        } else {
            break;
        }
    }

    // strip leading spaces
    int j = 0;
    for (j = 0; j < i; j++) {
        if (isspace(value[j])) {
            value[j] = '\0';
        } else {
            break;
        }
    }

    if (j > 0) {
        // reallocate the trimmed string
        char *newValue = new char[i-j+1+1];
        strcpy(newValue,value+j);
        Rp_TreeSetValue(inf->tree,inf->curr,Rp_ParserXml_Field_VALUE,(void *)newValue);
        delete value;
        value = NULL;
    }

    inf->path->del();
    inf->curr = Rp_TreeNodeParent(inf->curr);
}

void
Rp_ParserXmlDefaultCharHandler(
    void* data,
    XML_Char const* s,
    int len)
{
    Rp_ParserXml *inf = (Rp_ParserXml *) data;

    if (inf == NULL) {
        return;
    }

    // FIXME: where/when is this deallocated
    char *d = new char[len+1];

    if ( s != NULL) {
        strncpy(d,s,len);
    }

    d[len] = '\0';
    Rp_TreeSetValue(inf->tree,inf->curr,Rp_ParserXml_Field_VALUE,(void *)d);
}

Rp_ParserXml *
Rp_ParserXmlCreate()
{
    Rp_ParserXml *p = new Rp_ParserXml();

    Rp_TreeCreate("rapptureTree",&(p->tree));
    p->curr = Rp_TreeRootNode(p->tree);
    p->path = new Rappture::Path();
    p->buf = new Rappture::SimpleCharBuffer();


    return p;
}

void
Rp_ParserXmlDestroy(Rp_ParserXml **p)
{
    if (p == NULL) {
        return;
    }

    if (*p == NULL) {
        return;
    }

    Rp_TreeReleaseToken((*p)->tree);
    delete (*p)->buf;
    delete (*p)->path;
    delete *p;
    p = NULL;
}

void
Rp_ParserXmlParse(
    Rp_ParserXml *p,
    const char *xml)
{
    if (xml == NULL) {
        return;
    }

    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser,p);
    XML_SetElementHandler(parser, Rp_ParserXmlStartHandler,
        Rp_ParserXmlEndHandler);
    XML_SetDefaultHandlerExpand(parser, Rp_ParserXmlDefaultCharHandler);

    int done = 1;
    int len = 0;

    len = strlen(xml);
    if (XML_Parse(parser, xml, len, done) == XML_STATUS_ERROR) {
        fprintf(stderr, "Parse error at line %lu:\n%s\n",
                XML_GetCurrentLineNumber(parser),
                XML_ErrorString(XML_GetErrorCode(parser)));
        exit(-1);
    }

    XML_ParserFree(parser);

    // reset the root node
    p->curr = Rp_TreeFirstChild(Rp_TreeRootNode(p->tree));

    return;
}

Rp_TreeNode
Rp_ParserXmlCreateNode(
    Rp_ParserXml *p,
    Rp_TreeNode parent,
    const char *type,
    const char *id)
{
    if (!p->tree || !parent || !type) {
        fprintf(stderr,"failed to create node because of invalid data\n");
        return NULL;
    }

    Rp_TreeNode child = Rp_TreeCreateNode(p->tree, parent, type, -1);

    if (id != NULL) {
        size_t attrValueLen = strlen(id);
        // FIXME: memory leak originating from this new call
        // I'm not sure where/when the attrValue is deleted yet
        char *attrValue = new char[attrValueLen+1];
        strcpy(attrValue,id);
        Rp_TreeSetValue(p->tree,child,Rp_ParserXml_Field_ID,(void *)attrValue);
    }

    return child;
}

Rp_TreeNode
Rp_ParserXmlSearch(
    Rp_ParserXml *p,
    const char *path,
    int create)
{
    Rappture::Path pathObj(path);
    Rp_TreeNode parent = NULL;
    Rp_TreeNode child = NULL;
    size_t degree = 0;

    if (p == NULL) {
        return NULL;
    }

    // start searching from the base node
    parent = p->curr;

    // if (path == NULL) return the base node
    child = p->curr;

    pathObj.first();
    while ( (!pathObj.eof()) &&
            (parent != NULL) ) {

        const char *childName = pathObj.type();
        const char *searchChildId = pathObj.id();

        child = Rp_TreeFindChild(parent,childName);
        if (child == NULL) {
            // no nodes with the name childName exist
            // FIXME: use the RPXML_CREATE flag
            if (create) {
                for (size_t i = 1; i <= pathObj.degree(); i++) {
                    child = Rp_ParserXmlCreateNode(p, parent,
                                childName, searchChildId);
                    if (child == NULL) {
                        break;
                    }
                }
            } else {
                fprintf(stderr,"invalid path %s at %s\n",path,childName);
                break;
            }
        }

        //FIXME: rewrite the child id search logic
        const char *id = NULL;
        Rp_TreeGetValue(p->tree,child, Rp_ParserXml_Field_ID,(void **)&id);
        while (searchChildId && id) {

            // check for matching child id and degree
            if ((*searchChildId == *id) &&
                (strcmp(searchChildId,id) == 0)) {
                // found child with matching id
                // check degree
                degree++;
                if (degree == pathObj.degree()) {
                    // degrees match, return result
                    break;
                }
            }

            // check the next child to see if it has the correct id
            child = Rp_TreeNextSibling(child);
            if (child == NULL) {
                // end of the sibling list
                // create new node? or return NULL pointer
                // FIXME: use the RPXML_CREATE flag
                if (create) {
                    child = Rp_ParserXmlCreateNode(p, parent,
                                childName, searchChildId);
                    if (child == NULL) {
                        break;
                    }
                } else {
                    // exit, child not found
                    break;
                }
            }
            id = NULL;
            Rp_TreeGetValue(p->tree,child,Rp_ParserXml_Field_ID,(void **)&id);
        }

        parent = child;
        pathObj.next();
    }

    return child;
}

const char *
Rp_ParserXmlGet(
    Rp_ParserXml *p,
    const char *path)
{
    const char *value = NULL;
    Rp_TreeNode child = NULL;

    child = Rp_ParserXmlSearch(p, path, 0);
    if (child != NULL) {
        Rp_TreeGetValue(p->tree,child,Rp_ParserXml_Field_VALUE,(void **)&value);
    }

    return value;
}

void
Rp_ParserXmlPut(
    Rp_ParserXml *p,
    const char *path,
    const char *val,
    int append)
{
    const char *oldval = NULL;
    char *newval = NULL;
    size_t oldval_len = 0;
    size_t val_len = 0;

    if (val == NULL) {
        // no value, do nothing
        return;
    }

    Rp_TreeNode child = Rp_ParserXmlSearch(p, path, 1);

    if (child != NULL) {
        // check to see if there is already a value
        if (RP_OK == Rp_TreeGetValue(p->tree,child,
                        Rp_ParserXml_Field_VALUE,
                        (void **)&oldval)) {
            if (oldval != NULL) {
                // FIXME: use the RPXML_APPEND flag
                if (append) {
                    oldval_len = strlen(oldval);
                    val_len = strlen(val);
                    newval = new char[oldval_len + val_len + 1];
                    strncpy(newval,oldval,oldval_len);
                }
                // free the old data
                delete(oldval);
                oldval = NULL;
            }
        }

        // allocate space for our new value if needed
        if (newval == NULL) {
            // set the new value for the node
            val_len = strlen(val);
            newval = new char[val_len + 1];
        }

        strcpy(newval+oldval_len,val);

        // set the value of the child node
        if (RP_ERROR == Rp_TreeSetValue(p->tree,child,
                            Rp_ParserXml_Field_VALUE,
                            (void *)newval)) {
            fprintf(stderr,"error while setting value of %s\n",path);
        }
    }
    return;
}

Rp_Tree
Rp_ParserXmlTreeClient(
    Rp_ParserXml *p)
{
    if (p == NULL) {
        return NULL;
    }
    Rp_Tree newTree = NULL;
    // create a new token for the tree and return it to the caller
    Rp_TreeGetTokenFromToken(p->tree,&newTree);
    return newTree;
}

int
printXmlData(
    Rp_TreeNode node,
    ClientData clientData,
    int order)
{
    Rp_ParserXml *p = (Rp_ParserXml *) clientData;

    Rappture::Path labelComp(Rp_TreeNodeLabel(node));
    const char *label = labelComp.type();
    const char *value = NULL;

    size_t width = (Rp_TreeNodeDepth(p->tree,node)-1)*PARSERXML_LEVEL_WIDTH;
    const char *sp = "";
    int *visited = NULL;

    Rp_TreeGetValue(p->tree,node,Rp_ParserXml_Field_VALUE,(void **)&value);
    size_t valLen = strlen(value);

    if (!Rp_TreeValueExists(p->tree,node,Rp_ParserXml_Field_VISITED)) {
        visited = new int();
        *visited = 0;

        p->buf->appendf("%3$*2$s<%1$s",label,width,sp);

        // go through all attributes and place them in the tag
        Rp_TreeKey attrName = NULL;
        Rp_TreeKeySearch search;
        attrName = Rp_TreeFirstKey (p->tree, node, &search);
        while (attrName) {
            if ((*Rp_ParserXml_Field_VALUE != *attrName) &&
                (strcmp(Rp_ParserXml_Field_VALUE,attrName) != 0)) {
                const char *attrValue = NULL;
                Rp_TreeGetValueByKey(p->tree,node,attrName,(void**)&attrValue);
                p->buf->appendf(" %s=\"%s\"",attrName,attrValue);
            }
            attrName = Rp_TreeNextKey(p->tree, &search);
        }

        // close the opening tag
        p->buf->appendf(">");

        // append the value of the tag if any
        if ( (value != NULL ) &&
             (valLen != 0) ) {
            p->buf->appendf("%s",value);
        } else {
            p->buf->appendf("\n");
        }

        // set the "visited" temporary flag so the next time
        // we encounter this node, we know it is as a closing node
        Rp_TreeSetValue(p->tree,node,Rp_ParserXml_Field_VISITED,(void *)visited);

    } else {
        Rp_TreeGetValue(p->tree,node,Rp_ParserXml_Field_VISITED,(void **)&visited);
        delete visited;
        Rp_TreeUnsetValue(p->tree,node,Rp_ParserXml_Field_VISITED);
        if ( (value != NULL ) &&
             (valLen != 0) ) {
            p->buf->appendf("</%s>\n",label);
        } else {
            p->buf->appendf("%3$*2$s</%1$s>\n",label,width,sp);
        }

    }

    return RP_OK;
}

const char *
Rp_ParserXmlXml(
    Rp_ParserXml *p)
{
    p->buf->clear();
    p->buf->appendf("<?xml version=\"1.0\"?>\n");

    Rp_TreeNode root = p->curr;

    Rp_TreeApplyDFS(root, printXmlData, (ClientData)p, TREE_PREORDER|TREE_POSTORDER);

    return p->buf->bytes();
}

int
printPathVal(
    Rp_TreeNode node,
    ClientData clientData,
    int order)
{
    Rp_ParserXml *p = (Rp_ParserXml *) clientData;

    int *visited = NULL;

    if (!Rp_TreeValueExists(p->tree,node,Rp_ParserXml_Field_VISITED)) {
        // update path
        p->path->add(Rp_TreeNodeLabel(node));

        const char *id = NULL;
        if (RP_OK == Rp_TreeGetValue(p->tree,node,Rp_ParserXml_Field_ID,(void **)&id)) {
            p->path->last();
            p->path->id(id);
        }

        const char *value = NULL;
        if (RP_ERROR == Rp_TreeGetValue(p->tree,node,Rp_ParserXml_Field_VALUE,(void **)&value)) {
            // error while getting value, exit fxn gracefully?
            // FIXME: add error code here
        }

        // append the path and value of the node to our result text
        if (value && (strlen(value) != 0)) {
            p->buf->appendf("%s %s\n",p->path->path(),value);
        }

        // set the "visited" temporary flag so the next time
        // we encounter this node, we know it is as a closing node
        visited = new int();
        *visited = 0;
        if (RP_ERROR == Rp_TreeSetValue(p->tree,node,Rp_ParserXml_Field_VISITED,(void *)visited)) {
            // FIXME: error while setting value
        }
    } else {
        // update path
        p->path->del();

        // delete the "visited" temporary flag from the node
        if (RP_ERROR == Rp_TreeGetValue(p->tree,node,Rp_ParserXml_Field_VISITED,(void **)&visited)) {
            // FIXME: error while getting value
        }
        delete visited;
        if (RP_ERROR == Rp_TreeUnsetValue(p->tree,node,Rp_ParserXml_Field_VISITED)) {
            // FIXME: error while unsetting value
        }
    }

    return RP_OK;
}

const char *
Rp_ParserXmlPathVal(
    Rp_ParserXml *p)
{
    p->buf->clear();

    // temporarily store p->path so we can use p as ClientData
    Rappture::Path *tmpPath = p->path;
    p->path = new Rappture::Path();

    Rp_TreeNode root = p->curr;

    Rp_TreeApplyDFS(root, printPathVal, (ClientData)p, TREE_PREORDER|TREE_POSTORDER);

    // restore the original path
    delete p->path;
    p->path = tmpPath;

    return p->buf->bytes();
}

Rp_TreeNode
Rp_ParserXmlElement(
    Rp_ParserXml *p,
    const char *path)
{
    if (p == NULL) {
        return NULL;
    }

    return Rp_ParserXmlSearch(p,path,!RPXML_CREATE);
}

Rp_TreeNode
Rp_ParserXmlParent(
    Rp_ParserXml *p,
    const char *path)
{
    if (p == NULL) {
        return NULL;
    }

    Rp_TreeNode node = Rp_ParserXmlSearch(p,path,!RPXML_CREATE);
    if (node == NULL) {
        return NULL;
    }

    return Rp_TreeNodeParent(node);
}

size_t
Rp_ParserXmlChildren(
    Rp_ParserXml *p,
    const char *path,
    const char *type,
    Rp_Chain *children)
{
    size_t count = 0;

    if (p == NULL) {
        return count;
    }

    if (children == NULL) {
        return count;
    }

    Rp_TreeNode node = Rp_ParserXmlSearch(p,path,!RPXML_CREATE);

    if (node != NULL) {
        node = Rp_TreeFindChild(node,type);
        while (node != NULL) {
            count++;
            Rp_ChainAppend(children,(void*)node);
            node = Rp_TreeFindChildNext(node,type);
        }
    }
    return count;
}

size_t
Rp_ParserXmlNumberChildren(
    Rp_ParserXml *p,
    const char *path,
    const char *type)
{
    size_t count = 0;

    if (p == NULL) {
        return count;
    }

    Rp_TreeNode node = Rp_ParserXmlSearch(p,path,!RPXML_CREATE);
    if (node != NULL) {
        node = Rp_TreeFindChild(node,type);
        while (node != NULL) {
            count++;
            node = Rp_TreeFindChildNext(node,type);
        }
    }

    return count;
}

void
Rp_ParserXmlBaseNode(
    Rp_ParserXml *p,
    Rp_TreeNode node)
{
    if (p != NULL) {
        if (node == NULL) {
            p->curr = Rp_TreeFirstChild(Rp_TreeRootNode(p->tree));
        } else {
            p->curr = node;
        }
    }
}

/*
const char *
Rp_ParserXmlNodePath(
    Rp_ParserXml *p,
    Rp_TreeNode node)
{
    Path pathObj("junk");

    const char *type = NULL;
    const char *id = NULL;

    // FIXME: path's add and del don't work on the current node
    if (p != NULL) {
        while (node != p->curr) {
            type = Rp_TreeNodeLabel(node)
            Rp_TreeGetValue(p->tree,child, Rp_ParserXml_Field_ID,(void **)&id);
            pathObj.first();
            pathObj.add(type);
            pathObj.id(id);
            node = Rp_TreeNodeParent(node);
        }
    }

    pathObj.first();
    pathObj.del();
    return path
}
*/

const char *
Rp_ParserXmlNodeId(
    Rp_ParserXml *p,
    Rp_TreeNode node)
{
    const char *id = NULL;
    if (p != NULL) {
        if (node != NULL) {
            Rp_TreeGetValue(p->tree,node, Rp_ParserXml_Field_ID,(void **)&id);
        }
    }
    return id;
}

// -------------------------------------------------------------------- //

