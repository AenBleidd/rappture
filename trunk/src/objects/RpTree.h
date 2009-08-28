
/*
 * bltTree.h --
 *
 * Copyright 1998-1999 Lucent Technologies, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and warranty
 * disclaimer appear in supporting documentation, and that the names
 * of Lucent Technologies or any of their entities not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 *
 * Lucent Technologies disclaims all warranties with regard to this
 * software, including all implied warranties of merchantability and
 * fitness.  In no event shall Lucent Technologies be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether in
 * an action of contract, negligence or other tortuous action, arising
 * out of or in connection with the use or performance of this
 * software.
 *
 * The "tree" data object was created by George A. Howlett.
 */

#ifndef _RP_TREE_H
#define _RP_TREE_H

#include <RpInt.h>
#include <RpChain.h>
#include <RpHash.h>
#include <RpPool.h>

typedef struct Rp_TreeNodeStruct *Rp_TreeNode;
typedef struct Rp_TreeObjectStruct *Rp_TreeObject;
typedef struct Rp_TreeClientStruct *Rp_Tree;
typedef struct Rp_TreeTraceStruct *Rp_TreeTrace;
typedef struct Rp_TreeValueStruct *Rp_TreeValue;
typedef struct Rp_TreeTagEntryStruct Rp_TreeTagEntry;
typedef struct Rp_TreeTagTableStruct Rp_TreeTagTable;

typedef char *Rp_TreeKey;

#define TREE_PREORDER       (1<<0)
#define TREE_POSTORDER      (1<<1)
#define TREE_INORDER        (1<<2)
#define TREE_BREADTHFIRST   (1<<3)

#define TREE_TRACE_UNSET    (1<<3)
#define TREE_TRACE_WRITE    (1<<4)
#define TREE_TRACE_READ     (1<<5)
#define TREE_TRACE_CREATE   (1<<6)
#define TREE_TRACE_ALL      \
    (TREE_TRACE_UNSET | TREE_TRACE_WRITE | TREE_TRACE_READ | TREE_TRACE_CREATE)
#define TREE_TRACE_MASK     (TREE_TRACE_ALL)

#define TREE_TRACE_FOREIGN_ONLY (1<<8)
#define TREE_TRACE_ACTIVE   (1<<9)

#define TREE_NOTIFY_CREATE  (1<<0)
#define TREE_NOTIFY_DELETE  (1<<1)
#define TREE_NOTIFY_MOVE    (1<<2)
#define TREE_NOTIFY_SORT    (1<<3)
#define TREE_NOTIFY_RELABEL (1<<4)
#define TREE_NOTIFY_ALL     \
    (TREE_NOTIFY_CREATE | TREE_NOTIFY_DELETE | TREE_NOTIFY_MOVE | \
    TREE_NOTIFY_SORT | TREE_NOTIFY_RELABEL)
#define TREE_NOTIFY_MASK    (TREE_NOTIFY_ALL)

#define TREE_NOTIFY_WHENIDLE    (1<<8)
#define TREE_NOTIFY_FOREIGN_ONLY (1<<9)
#define TREE_NOTIFY_ACTIVE      (1<<10)

typedef struct {
    int type;
    Rp_Tree tree;
    int inode;          /* Node of event */
    // Tcl_Interp *interp;
} Rp_TreeNotifyEvent;

typedef struct {
    Rp_TreeNode node;           /* Node being searched. */
    unsigned long nextIndex;    /* Index of next bucket to be
                                 * enumerated after present one. */
    Rp_TreeValue nextValue;     /* Next entry to be enumerated in the
                                 * the current bucket. */
} Rp_TreeKeySearch;

/*
 * Rp_TreeObject --
 *
 *  Structure providing the internal representation of the tree
 *  object. A tree is uniquely identified by a combination of
 *  its name and originating namespace.  Two trees in the same
 *  interpreter can have the same names but reside in different
 *  namespaces.
 *
 *  The tree object represents a general-ordered tree of nodes.
 *  Each node may contain a heterogeneous collection of data
 *  values. Each value is identified by a field name and nodes
 *  do not need to contain the same data fields. Data field
 *  names are saved as reference counted strings and can be
 *  shared among nodes.
 *
 *  The tree is threaded.  A node contains both a pointer to
 *  back its parents and another to its siblings.  Therefore
 *  the tree maybe traversed non-recursively.
 *
 *  A tree object can be shared by several clients.  When a
 *  client wants to use a tree object, it is given a token
 *  that represents the tree.  The tree object uses the tokens
 *  to keep track of its clients.  When all clients have
 *  released their tokens the tree is automatically destroyed.
 */
struct Rp_TreeObjectStruct {
//    Tcl_Interp *interp;     /* Interpreter associated with this tree. */

    char *name;

//    Tcl_Namespace *nsPtr;

    Rp_HashEntry *hashPtr;  /* Pointer to this tree object in tree
                             * object hash table. */
    Rp_HashTable *tablePtr;

    Rp_TreeNode root;       /* Root of the entire tree. */

    char *sortNodesCmd;     /* Tcl command to invoke to sort entries */

    Rp_Chain *clients;      /* List of clients using this tree */

    Rp_Pool nodePool;
    Rp_Pool valuePool;

    Rp_HashTable nodeTable; /* Table of node identifiers. Used to
                             * search for a node pointer given an inode.*/
    unsigned int nextInode;

    unsigned int nNodes;    /* Always counts root node. */

    unsigned int depth;     /* Maximum depth of the tree. */

    unsigned int flags;     /* Internal flags. See definitions
                             * below. */
    unsigned int notifyFlags;   /* Notification flags. See definitions
                                 * below. */

};

/*
 * Rp_TreeNodeStruct --
 *
 *  Structure representing a node in a general ordered tree.
 *  Nodes are identified by their index, or inode.  Nodes also
 *  have names, but nodes names are not unique and can be 
 *  changed.  Inodes are valid even if the node is moved.
 *
 *  Each node can contain a list of data fields.  Fields are
 *  name-value pairs.  The values are represented by Tcl_Objs.
 *
 */
struct Rp_TreeNodeStruct {
    Rp_TreeNode parent; /* Parent node. If NULL, then this is
                           the root node. */
    Rp_TreeNode next;       /* Next sibling node. */
    Rp_TreeNode prev;       /* Previous sibling node. */
    Rp_TreeNode first;      /* First child node. */
    Rp_TreeNode last;       /* Last child node. */

    Rp_TreeKey label;       /* Node label (doesn't have to be
                             * unique). */

    Rp_TreeObject treeObject;

    Rp_TreeValue values;    /* Depending upon the number of values
                             * stored, this is either a chain or
                             * hash table of Rp_TreeValue
                             * structures.  (Note: if logSize is 
                             * 0, then this is a list).  Each
                             * value contains key/value data
                             * pair.  The data is a Tcl_Obj. */

    unsigned short nValues; /* # of values for this node. */

    unsigned short logSize; /* Size of hash table indicated as a
                             * power of 2 (e.g. if logSize=3, then
                             * table size is 8). If 0, this
                             * indicates that the node's values
                             * are stored as a list. */

    unsigned int nChildren; /* # of children for this node. */
    // unsigned int inode;     /* Serial number of the node. */
    size_t inode;           /* Serial number of the node. */

    unsigned short depth;   /* The depth of this node in the tree. */

    unsigned short flags;
};

struct Rp_TreeTagEntryStruct {
    char *tagName;
    Rp_HashEntry *hashPtr;
    Rp_HashTable nodeTable;
};

struct Rp_TreeTagTableStruct {
    Rp_HashTable tagTable;
    int refCount;
};

/*
 * Rp_TreeClientStruct --
 *
 *  A tree can be shared by several clients.  Each client allocates
 *  this structure which acts as a ticket for using the tree.  Clients
 *  can designate notifier routines that are automatically invoked
 *  by the tree object whenever the tree is changed is specific
 *  ways by other clients.
 */

struct Rp_TreeClientStruct {
    unsigned int magic;     /* Magic value indicating whether a
                             * generic pointer is really a tree
                             * token or not. */
    Rp_ChainLink *linkPtr;  /* Pointer into the server's chain of
                             * clients. */
    Rp_TreeObject treeObject;   /* Pointer to the structure containing
                                 * the master information about the
                                 * tree used by the client.  If NULL,
                                 * this indicates that the tree has
                                 * been destroyed (but as of yet, this
                                 * client hasn't recognized it). */

    Rp_Chain *events;       /* Chain of node event handlers. */
    Rp_Chain *traces;       /* Chain of data field callbacks. */
    Rp_TreeNode root;       /* Designated root for this client */
    Rp_TreeTagTable *tagTablePtr;
};


typedef int (Rp_TreeNotifyEventProc) _ANSI_ARGS_((ClientData clientData,
        Rp_TreeNotifyEvent *eventPtr));

typedef int (Rp_TreeTraceProc) _ANSI_ARGS_((ClientData clientData,
        Rp_TreeNode node, Rp_TreeKey key, unsigned int flags));

typedef int (Rp_TreeEnumProc) _ANSI_ARGS_((Rp_TreeNode node, Rp_TreeKey key,
        // Tcl_Obj *valuePtr));
        void *valuePtr));

typedef int (Rp_TreeCompareNodesProc) _ANSI_ARGS_((Rp_TreeNode *n1Ptr,
        Rp_TreeNode *n2Ptr));

typedef int (Rp_TreeApplyProc) _ANSI_ARGS_((Rp_TreeNode node,
        ClientData clientData, int order));

struct Rp_TreeTraceStruct {
    ClientData clientData;
    Rp_TreeKey key;
    Rp_TreeNode node;
    unsigned int mask;
    Rp_TreeTraceProc *proc;
};

/*
 * Structure definition for information used to keep track of searches
 * through hash tables:p
 */
struct Rp_TreeKeySearchStruct {
    Rp_TreeNode node;           /* Table being searched. */
    unsigned long nextIndex;    /* Index of next bucket to be
                                 * enumerated after present one. */
    Rp_TreeValue nextValue;     /* Next entry to be enumerated in the
                                 * the current bucket. */
};

EXTERN Rp_TreeKey Rp_TreeGetKey _ANSI_ARGS_((CONST char *string));

EXTERN Rp_TreeNode Rp_TreeCreateNode _ANSI_ARGS_((Rp_Tree tree,
        Rp_TreeNode parent, CONST char *name, int position));
EXTERN Rp_TreeNode Rp_TreeCreateNodeWithId _ANSI_ARGS_((Rp_Tree tree,
        Rp_TreeNode parent, CONST char *name, size_t inode, int position));

EXTERN int Rp_TreeDeleteNode _ANSI_ARGS_((Rp_Tree tree, Rp_TreeNode node));

EXTERN int Rp_TreeMoveNode _ANSI_ARGS_((Rp_Tree tree, Rp_TreeNode node,
        Rp_TreeNode parent, Rp_TreeNode before));

EXTERN Rp_TreeNode Rp_TreeGetNode _ANSI_ARGS_((Rp_Tree tree, size_t inode));

EXTERN Rp_TreeNode Rp_TreeFindChild _ANSI_ARGS_((Rp_TreeNode parent,
        CONST char *name));

EXTERN Rp_TreeNode Rp_TreeFindChildNext _ANSI_ARGS_((Rp_TreeNode child,
        CONST char *name));

EXTERN Rp_TreeNode Rp_TreeFirstChild _ANSI_ARGS_((Rp_TreeNode parent));

EXTERN Rp_TreeNode Rp_TreeNextSibling _ANSI_ARGS_((Rp_TreeNode node));

EXTERN Rp_TreeNode Rp_TreeLastChild _ANSI_ARGS_((Rp_TreeNode parent));

EXTERN Rp_TreeNode Rp_TreePrevSibling _ANSI_ARGS_((Rp_TreeNode node));

EXTERN Rp_TreeNode Rp_TreeNextNode _ANSI_ARGS_((Rp_TreeNode root,
        Rp_TreeNode node));

EXTERN Rp_TreeNode Rp_TreePrevNode _ANSI_ARGS_((Rp_TreeNode root,
        Rp_TreeNode node));

EXTERN Rp_TreeNode Rp_TreeChangeRoot _ANSI_ARGS_((Rp_Tree tree,
        Rp_TreeNode node));

EXTERN Rp_TreeNode Rp_TreeEndNode _ANSI_ARGS_((Rp_TreeNode node,
        unsigned int nodeFlags));

EXTERN int Rp_TreeIsBefore _ANSI_ARGS_((Rp_TreeNode node1,
        Rp_TreeNode node2));

EXTERN int Rp_TreeIsAncestor _ANSI_ARGS_((Rp_TreeNode node1,
        Rp_TreeNode node2));

EXTERN int Rp_TreePrivateValue _ANSI_ARGS_((Rp_Tree tree,
        Rp_TreeNode node, Rp_TreeKey key));

EXTERN int Rp_TreePublicValue _ANSI_ARGS_((Rp_Tree tree,
        Rp_TreeNode node, Rp_TreeKey key));

EXTERN int Rp_TreeGetValue _ANSI_ARGS_((Rp_Tree tree,
        // Rp_TreeNode node, CONST char *string, Tcl_Obj **valuePtr));
        Rp_TreeNode node, CONST char *string, void **valuePtr));

EXTERN int Rp_TreeValueExists _ANSI_ARGS_((Rp_Tree tree, Rp_TreeNode node,
        CONST char *string));

EXTERN int Rp_TreeSetValue _ANSI_ARGS_((Rp_Tree tree,
        // Rp_TreeNode node, CONST char *string, Tcl_Obj *valuePtr));
        Rp_TreeNode node, CONST char *string, void *valuePtr));

EXTERN int Rp_TreeUnsetValue _ANSI_ARGS_((Rp_Tree tree,
        Rp_TreeNode node, CONST char *string));

EXTERN int Rp_TreeGetArrayValue _ANSI_ARGS_((
        Rp_Tree tree, Rp_TreeNode node, CONST char *arrayName,
        // CONST char *elemName, Tcl_Obj **valueObjPtrPtr));
        CONST char *elemName, void **valueObjPtrPtr));

EXTERN int Rp_TreeSetArrayValue _ANSI_ARGS_((
        Rp_Tree tree, Rp_TreeNode node, CONST char *arrayName,
        //CONST char *elemName, Tcl_Obj *valueObjPtr));
        CONST char *elemName, void *valueObjPtr));

EXTERN int Rp_TreeUnsetArrayValue _ANSI_ARGS_((
        Rp_Tree tree, Rp_TreeNode node, CONST char *arrayName,
        CONST char *elemName));

EXTERN int Rp_TreeArrayValueExists _ANSI_ARGS_((Rp_Tree tree,
        Rp_TreeNode node, CONST char *arrayName, CONST char *elemName));

EXTERN int Rp_TreeArrayNames _ANSI_ARGS_((Rp_Tree tree,
        // Rp_TreeNode node, CONST char *arrayName, Tcl_Obj *listObjPtr));
        Rp_TreeNode node, CONST char *arrayName, void *listObjPtr));

EXTERN int Rp_TreeGetValueByKey _ANSI_ARGS_((
        Rp_Tree tree, Rp_TreeNode node, Rp_TreeKey key,
        // Tcl_Obj **valuePtr));
        void **valuePtr));

EXTERN int Rp_TreeSetValueByKey _ANSI_ARGS_((
        // Rp_Tree tree, Rp_TreeNode node, Rp_TreeKey key, Tcl_Obj *valuePtr));
        Rp_Tree tree, Rp_TreeNode node, Rp_TreeKey key, void *valuePtr));

EXTERN int Rp_TreeUnsetValueByKey _ANSI_ARGS_((
        Rp_Tree tree, Rp_TreeNode node, Rp_TreeKey key));

EXTERN int Rp_TreeValueExistsByKey _ANSI_ARGS_((Rp_Tree tree,
        Rp_TreeNode node, Rp_TreeKey key));

EXTERN Rp_TreeKey Rp_TreeFirstKey _ANSI_ARGS_((Rp_Tree tree,
        Rp_TreeNode node, Rp_TreeKeySearch *cursorPtr));

EXTERN Rp_TreeKey Rp_TreeNextKey _ANSI_ARGS_((Rp_Tree tree,
        Rp_TreeKeySearch *cursorPtr));

EXTERN int Rp_TreeApply _ANSI_ARGS_((Rp_TreeNode root,
        Rp_TreeApplyProc *proc, ClientData clientData));

EXTERN int Rp_TreeApplyDFS _ANSI_ARGS_((Rp_TreeNode root,
        Rp_TreeApplyProc *proc, ClientData clientData, int order));

EXTERN int Rp_TreeApplyBFS _ANSI_ARGS_((Rp_TreeNode root,
        Rp_TreeApplyProc *proc, ClientData clientData));

EXTERN int Rp_TreeApplyXML _ANSI_ARGS_((Rp_TreeNode root,
        Rp_TreeApplyProc *proc, ClientData clientData));

EXTERN int Rp_TreeSortNode _ANSI_ARGS_((Rp_Tree tree, Rp_TreeNode node,
        Rp_TreeCompareNodesProc *proc));

EXTERN int Rp_TreeCreate _ANSI_ARGS_((CONST char *name,
        Rp_Tree *treePtr));

// EXTERN int Rp_TreeExists _ANSI_ARGS_((CONST char *name));

// EXTERN int Rp_TreeGetToken _ANSI_ARGS_((CONST char *name,
//         Rp_Tree *treePtr));

EXTERN int Rp_TreeGetTokenFromToken _ANSI_ARGS_((
        CONST Rp_Tree tree, Rp_Tree *newToken));

EXTERN void Rp_TreeReleaseToken _ANSI_ARGS_((Rp_Tree tree));

EXTERN int Rp_TreeSize _ANSI_ARGS_((Rp_TreeNode node));

EXTERN Rp_TreeTrace Rp_TreeCreateTrace _ANSI_ARGS_((Rp_Tree tree,
        Rp_TreeNode node, CONST char *keyPattern, CONST char *tagName,
        unsigned int mask, Rp_TreeTraceProc *proc, ClientData clientData));

EXTERN void Rp_TreeDeleteTrace _ANSI_ARGS_((Rp_TreeTrace token));

EXTERN void Rp_TreeCreateEventHandler _ANSI_ARGS_((Rp_Tree tree,
        unsigned int mask, Rp_TreeNotifyEventProc *proc,
        ClientData clientData));

EXTERN void Rp_TreeDeleteEventHandler _ANSI_ARGS_((Rp_Tree tree,
        unsigned int mask, Rp_TreeNotifyEventProc *proc,
        ClientData clientData));

EXTERN void Rp_TreeRelabelNode _ANSI_ARGS_((Rp_Tree tree, Rp_TreeNode node,
        CONST char *string));
EXTERN void Rp_TreeRelabelNode2 _ANSI_ARGS_((Rp_TreeNode node,
        CONST char *string));
//EXTERN char *Rp_TreeNodePath _ANSI_ARGS_((Rp_TreeNode node,
//        Tcl_DString *resultPtr));
EXTERN int Rp_TreeNodePosition _ANSI_ARGS_((Rp_TreeNode node));

EXTERN void Rp_TreeClearTags _ANSI_ARGS_((Rp_Tree tree, Rp_TreeNode node));
EXTERN int Rp_TreeHasTag _ANSI_ARGS_((Rp_Tree tree, Rp_TreeNode node,
        CONST char *tagName));
EXTERN void Rp_TreeAddTag _ANSI_ARGS_((Rp_Tree tree, Rp_TreeNode node,
        CONST char *tagName));
EXTERN void Rp_TreeForgetTag _ANSI_ARGS_((Rp_Tree tree, CONST char *tagName));
EXTERN Rp_HashTable *Rp_TreeTagHashTable _ANSI_ARGS_((Rp_Tree tree,
        CONST char *tagName));
EXTERN int Rp_TreeTagTableIsShared _ANSI_ARGS_((Rp_Tree tree));
EXTERN int Rp_TreeShareTagTable _ANSI_ARGS_((Rp_Tree src, Rp_Tree target));
EXTERN Rp_HashEntry *Rp_TreeFirstTag _ANSI_ARGS_((Rp_Tree tree,
        Rp_HashSearch *searchPtr));

#define Rp_TreeName(token)              ((token)->treeObject->name)
#define Rp_TreeRootNode(token)          ((token)->root)
#define Rp_TreeChangeRoot(token, node)  ((token)->root = (node))

#define Rp_TreeNodeDepth(token, node)   ((node)->depth - (token)->root->depth)
#define Rp_TreeNodeLabel(node)          ((node)->label)
#define Rp_TreeNodeId(node)             ((node)->inode)
#define Rp_TreeNodeParent(node)         ((node)->parent)
#define Rp_TreeNodeDegree(node)         ((node)->nChildren)

#define Rp_TreeIsLeaf(node)             ((node)->nChildren == 0)
#define Rp_TreeNextNodeId(token)        ((token)->treeObject->nextInode)

#define Rp_TreeFirstChild(node)         ((node)->first)
#define Rp_TreeLastChild(node)          ((node)->last)
#define Rp_TreeNextSibling(node)        (((node) == NULL) ? NULL : (node)->next)
#define Rp_TreePrevSibling(node)        (((node) == NULL) ? NULL : (node)->prev)

#endif /* _RP_TREE_H */
