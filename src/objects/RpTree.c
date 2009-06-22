
/*
 * bltTree.c --
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

#include "RpInt.h"

/* TODO:
 *  traces and notifiers should be in one list in tree object.
 *  notifier is always fired.
 *  incorporate first/next tag routines ?
 */


#ifndef NO_TREE

#include "RpTree.h"

//static Tcl_InterpDeleteProc TreeInterpDeleteProc;
static Rp_TreeApplyProc SizeApplyProc;
//static Tcl_IdleProc NotifyIdleProc;

#define TREE_THREAD_KEY     "BLT Tree Data"
#define TREE_MAGIC          ((unsigned int) 0x46170277)
#define TREE_DESTROYED      (1<<0)

typedef struct Rp_TreeNodeStruct Node;
typedef struct Rp_TreeClientStruct TreeClient;
typedef struct Rp_TreeObjectStruct TreeObject;
typedef struct Rp_TreeValueStruct Value;

/*
 * Rp_TreeValue --
 *
 *  Tree nodes contain heterogeneous data fields, represented as a
 *  chain of these structures.  Each field contains the key of the
 *  field (Rp_TreeKey) and the value (Tcl_Obj) containing the
 *  actual data representations.
 *
 */
struct Rp_TreeValueStruct {
    Rp_TreeKey key;     /* String identifying the data field */
    void *objPtr;       /* Data representation. */
    Rp_Tree owner;      /* Non-NULL if privately owned. */
    Rp_TreeValue next;  /* Next value in the chain. */
};

#include <stdio.h>
#include <string.h>
/* The following header is required for LP64 compilation */
#include <stdlib.h>

#include "RpHash.h"

static void TreeDestroyValues _ANSI_ARGS_((Rp_TreeNode node));

static Value *TreeFindValue _ANSI_ARGS_((Rp_TreeNode node,
    Rp_TreeKey key));
static Value *TreeCreateValue _ANSI_ARGS_((Rp_TreeNode node,
    Rp_TreeKey key, int *newPtr));

static int TreeDeleteValue _ANSI_ARGS_((Rp_TreeNode node,
    Rp_TreeValue value));

static Value *TreeFirstValue _ANSI_ARGS_((Rp_TreeNode,
    Rp_TreeKeySearch *searchPtr));

static Value *TreeNextValue _ANSI_ARGS_((Rp_TreeKeySearch *srchPtr));

/*
 * When there are this many entries per bucket, on average, rebuild
 * the hash table to make it larger.
 */

#define REBUILD_MULTIPLIER  3
#define START_LOGSIZE       5  /* Initial hash table size is 32. */
#define MAX_LIST_VALUES     20 /* Convert to hash table when node
                                * value list gets bigger than this
                                * many values. */

#if (SIZEOF_VOID_P == 8)
#define RANDOM_INDEX(i)     HashOneWord(mask, downshift, i)
#define BITSPERWORD         64
#else

/*
 * The following macro takes a preliminary integer hash value and
 * produces an index into a hash tables bucket list.  The idea is
 * to make it so that preliminary values that are arbitrarily similar
 * will end up in different buckets.  The hash function was taken
 * from a random-number generator.
 */
#define RANDOM_INDEX(i) \
    (((((long) (i))*1103515245) >> downshift) & mask)
#define BITSPERWORD         32
#endif

#define DOWNSHIFT_START     (BITSPERWORD - 2)

/*
 * Procedure prototypes for static procedures in this file:
 */


#if (SIZEOF_VOID_P == 8)
static Rp_Hash HashOneWord _ANSI_ARGS_((uint64_t mask, unsigned int downshift,
    CONST void *key));

#endif /* SIZEOF_VOID_P == 8 */

/*
 * The hash table below is used to keep track of all the Rp_TreeKeys
 * created so far.
 */
static Rp_HashTable keyTable;
static int keyTableInitialized = 0;

typedef struct {
    Rp_HashTable treeTable; /* Table of trees. */
    unsigned int nextId;
//    Tcl_Interp *interp;
} TreeInterpData;

typedef struct {
//    Tcl_Interp *interp;
    ClientData clientData;
    Rp_TreeKey key;
    unsigned int mask;
    Rp_TreeNotifyEventProc *proc;
    Rp_TreeNotifyEvent event;
    int notifyPending;
} EventHandler;

typedef struct {
    ClientData clientData;
    char *keyPattern;
    char *withTag;
    Node *nodePtr;
    unsigned int mask;
    Rp_TreeTraceProc *proc;
    TreeClient *clientPtr;
    Rp_ChainLink *linkPtr;
} TraceHandler;

/*
 * --------------------------------------------------------------
 *
 * GetTreeInterpData --
 *
 *  Creates or retrieves data associated with tree data objects
 *  for a particular thread.  We're using Tcl_GetAssocData rather
 *  than the Tcl thread routines so BLT can work with pre-8.0
 *  Tcl versions that don't have thread support.
 *
 * Results:
 *  Returns a pointer to the tree interpreter data.
 *
 * -------------------------------------------------------------- 
 */
/*
static TreeInterpData *
GetTreeInterpData(Tcl_Interp *interp)
{
    Tcl_InterpDeleteProc *proc;
    TreeInterpData *dataPtr;

    dataPtr = (TreeInterpData *)
        Tcl_GetAssocData(interp, TREE_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
        dataPtr = Rp_Malloc(sizeof(TreeInterpData));
        assert(dataPtr);
        dataPtr->interp = interp;
        Tcl_SetAssocData(interp, TREE_THREAD_KEY, TreeInterpDeleteProc,
                 dataPtr);
        Rp_InitHashTable(&dataPtr->treeTable, BLT_STRING_KEYS);
    }
    return dataPtr;
}
*/

/*
 * --------------------------------------------------------------
 *
 * NewNode --
 *
 *  Creates a new node in the tree without installing it.  The
 *  number of nodes in the tree is incremented and a unique serial
 *  number is generated for the node.
 *
 *  Also, all nodes have a label.  If no label was provided (name
 *  is NULL) then automatically generate one in the form "nodeN"
 *  where N is the serial number of the node.
 *
 * Results:
 *  Returns a pointer to the new node.
 *
 * -------------------------------------------------------------- 
 */
static Node *
NewNode(TreeObject *treeObjPtr, CONST char *name, int inode)
{
    Node *nodePtr;

    /* Create the node structure */
    nodePtr = Rp_PoolAllocItem(treeObjPtr->nodePool, sizeof(Node));
    nodePtr->inode = inode;
    nodePtr->treeObject = treeObjPtr;
    nodePtr->parent = NULL;
    nodePtr->depth = 0;
    nodePtr->flags = 0;
    nodePtr->next = nodePtr->prev = NULL;
    nodePtr->first = nodePtr->last = NULL;
    nodePtr->nChildren = 0;
    nodePtr->values = NULL;
    nodePtr->logSize = 0;
    nodePtr->nValues = 0;
    nodePtr->label = NULL;
    if (name != NULL) {
        nodePtr->label = Rp_TreeGetKey(name);
    }
    treeObjPtr->nNodes++;
    return nodePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ReleaseTagTable --
 *
 *---------------------------------------------------------------------- 
 */
static void
ReleaseTagTable(Rp_TreeTagTable *tablePtr)
{
    tablePtr->refCount--;
    if (tablePtr->refCount <= 0) {
        Rp_HashEntry *hPtr;
        Rp_HashSearch cursor;

        for(hPtr = Rp_FirstHashEntry(&tablePtr->tagTable, &cursor);
            hPtr != NULL; hPtr = Rp_NextHashEntry(&cursor)) {

            Rp_TreeTagEntry *tPtr;

            tPtr = Rp_GetHashValue(hPtr);
            Rp_DeleteHashTable(&tPtr->nodeTable);
            Rp_Free(tPtr);
        }
        Rp_DeleteHashTable(&tablePtr->tagTable);
        Rp_Free(tablePtr);
    }
}

/*
 * ----------------------------------------------------------------------
 *
 * ResetDepths --
 *
 *  Called after moving a node, resets the depths of each node
 *  for the entire branch (node and it's decendants).
 *
 * Results:
 *  None.
 *
 * ---------------------------------------------------------------------- 
 */
static void
ResetDepths(
    Node *nodePtr,      /* Root node. */
    int depth)          /* Depth of the node. */
{
    nodePtr->depth = depth;
    /* Also reset the depth for each descendant node. */
    for (nodePtr = nodePtr->first; nodePtr != NULL; nodePtr = nodePtr->next) {
        ResetDepths(nodePtr, depth + 1);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * LinkBefore --
 *
 *  Inserts a link preceding a given link.
 *
 * Results:
 *  None.
 *
 *----------------------------------------------------------------------
 */
static void
LinkBefore(
    Node *parentPtr,    /* Parent to hold the new entry. */
    Node *nodePtr,      /* New node to be inserted. */
    Node *beforePtr)    /* Node to link before. */
{
    if (parentPtr->first == NULL) {
        parentPtr->last = parentPtr->first = nodePtr;
    } else if (beforePtr == NULL) { /* Append onto the end of the chain */
        nodePtr->next = NULL;
        nodePtr->prev = parentPtr->last;
        parentPtr->last->next = nodePtr;
        parentPtr->last = nodePtr;
    } else {
        nodePtr->prev = beforePtr->prev;
        nodePtr->next = beforePtr;
        if (beforePtr == parentPtr->first) {
            parentPtr->first = nodePtr;
        } else {
            beforePtr->prev->next = nodePtr;
        }
        beforePtr->prev = nodePtr;
    }
    parentPtr->nChildren++;
    nodePtr->parent = parentPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * UnlinkNode --
 *
 *  Unlinks a link from the chain. The link is not deallocated,
 *  but only removed from the chain.
 *
 * Results:
 *  None.
 *
 *----------------------------------------------------------------------
 */
static void
UnlinkNode(Node *nodePtr)
{
    Node *parentPtr;
    int unlinked;       /* Indicates if the link is actually
                         * removed from the chain. */
    parentPtr = nodePtr->parent;
    unlinked = FALSE;
    if (parentPtr->first == nodePtr) {
        parentPtr->first = nodePtr->next;
        unlinked = TRUE;
    }
    if (parentPtr->last == nodePtr) {
        parentPtr->last = nodePtr->prev;
        unlinked = TRUE;
    }
    if (nodePtr->next != NULL) {
        nodePtr->next->prev = nodePtr->prev;
        unlinked = TRUE;
    }
    if (nodePtr->prev != NULL) {
        nodePtr->prev->next = nodePtr->next;
        unlinked = TRUE;
    }
    if (unlinked) {
        parentPtr->nChildren--;
    }
    nodePtr->prev = nodePtr->next = NULL;
}

/*
 * --------------------------------------------------------------
 *
 * FreeNode --
 *
 *  Unlinks a given node from the tree, removes its data, and
 *  frees memory allocated to the node.
 *
 * Results:
 *  None.
 *
 * -------------------------------------------------------------- 
 */
static void
FreeNode(TreeObject *treeObjPtr, Node *nodePtr)
{
    Rp_HashEntry *hPtr;

    /*
     * Destroy any data fields associated with this node.
     */
    TreeDestroyValues(nodePtr);
    UnlinkNode(nodePtr);
    treeObjPtr->nNodes--;
    hPtr = Rp_FindHashEntry(&treeObjPtr->nodeTable, (char *)nodePtr->inode);
    assert(hPtr);
    Rp_DeleteHashEntry(&treeObjPtr->nodeTable, hPtr);
    Rp_PoolFreeItem(treeObjPtr->nodePool, (char *)nodePtr);
}

/*
 * --------------------------------------------------------------
 *
 * NewTreeObject --
 *
 *  Creates and initializes a new tree object. Trees always
 *  contain a root node, so one is allocated here.
 *
 * Results:
 *  Returns a pointer to the new tree object is successful, NULL
 *  otherwise.  If a tree can't be generated, interp->result will
 *  contain an error message.
 *
 * -------------------------------------------------------------- */
static TreeObject *
// NewTreeObject(TreeInterpData *dataPtr, CONST char *treeName)
NewTreeObject(CONST char *treeName)
{
    TreeObject *treeObjPtr;
    int isNew;
    Rp_HashEntry *hPtr;

    treeObjPtr = Rp_Calloc(1, sizeof(TreeObject));
    if (treeObjPtr == NULL) {
        fprintf(stderr, "can't allocate tree");
        return NULL;
    }
    treeObjPtr->name = Rp_Strdup(treeName);
    //treeObjPtr->interp = interp;
    treeObjPtr->valuePool = Rp_PoolCreate(RP_FIXED_SIZE_ITEMS);
    treeObjPtr->nodePool = Rp_PoolCreate(RP_FIXED_SIZE_ITEMS);
    treeObjPtr->clients = Rp_ChainCreate();
    treeObjPtr->depth = 1;
    treeObjPtr->notifyFlags = 0;
    Rp_InitHashTableWithPool(&treeObjPtr->nodeTable, RP_ONE_WORD_KEYS);

    hPtr = Rp_CreateHashEntry(&treeObjPtr->nodeTable, (char *)0, &isNew);
    treeObjPtr->root = NewNode(treeObjPtr, treeName, 0);
    Rp_SetHashValue(hPtr, treeObjPtr->root);

    // treeObjPtr->tablePtr = &dataPtr->treeTable;
    // treeObjPtr->hashPtr = Rp_CreateHashEntry(treeObjPtr->tablePtr, treeName,
    //    &isNew);
    // Rp_SetHashValue(treeObjPtr->hashPtr, treeObjPtr);
    treeObjPtr->tablePtr = NULL;
    treeObjPtr->hashPtr = NULL;

    return treeObjPtr;
}

/*
static TreeObject *
FindTreeInNamespace(
*/
//    TreeInterpData *dataPtr,    /* Interpreter-specific data. */
/*    Tcl_Namespace *nsPtr,
    CONST char *treeName)
{
    Tcl_DString dString;
    char *name;
    Rp_HashEntry *hPtr;

    name = Rp_GetQualifiedName(nsPtr, treeName, &dString);
    hPtr = Rp_FindHashEntry(&dataPtr->treeTable, name);
    Tcl_DStringFree(&dString);
    if (hPtr != NULL) {
        return Rp_GetHashValue(hPtr);
    }
    return NULL;
}
*/

/*
 * ----------------------------------------------------------------------
 *
 * GetTreeObject --
 *
 *  Searches for the tree object associated by the name given.
 *
 * Results:
 *  Returns a pointer to the tree if found, otherwise NULL.
 *
 * ----------------------------------------------------------------------
 */
//static TreeObject *
//GetTreeObject(Tcl_Interp *interp, CONST char *name, int flags)
//{
//    CONST char *treeName;
//    Tcl_Namespace *nsPtr;   /* Namespace associated with the tree object.
//                             * If NULL, indicates to look in first the
//                             * current namespace and then the global
//                             * for the tree. */
//    TreeInterpData *dataPtr;    /* Interpreter-specific data. */
//    TreeObject *treeObjPtr;
//
//    treeObjPtr = NULL;
//    if (Rp_ParseQualifiedName(interp, name, &nsPtr, &treeName) != TCL_OK) {
//        Tcl_AppendResult(interp, "can't find namespace in \"", name, "\"",
//            (char *)NULL);
//        return NULL;
//    }
//    dataPtr = GetTreeInterpData(interp);
//    if (nsPtr != NULL) {
//        treeObjPtr = FindTreeInNamespace(dataPtr, nsPtr, treeName);
//    } else {
//        if (flags & NS_SEARCH_CURRENT) {
//            /* Look first in the current namespace. */
//            nsPtr = Tcl_GetCurrentNamespace(interp);
//            treeObjPtr = FindTreeInNamespace(dataPtr, nsPtr, treeName);
//        }
//        if ((treeObjPtr == NULL) && (flags & NS_SEARCH_GLOBAL)) {
//            nsPtr = Tcl_GetGlobalNamespace(interp);
//            treeObjPtr = FindTreeInNamespace(dataPtr, nsPtr, treeName);
//        }
//    }
//    return treeObjPtr;
//}

/*
 * ----------------------------------------------------------------------
 *
 * TeardownTree --
 *
 *  Destroys an entire branch.  This is a special purpose routine
 *  used to speed up the final clean up of the tree.
 *
 * Results:
 *  None.
 *
 * ---------------------------------------------------------------------- 
 */
static void
TeardownTree(TreeObject *treeObjPtr, Node *nodePtr)
{
    if (nodePtr->first != NULL) {
        Node *childPtr, *nextPtr;

        for (childPtr = nodePtr->first; childPtr != NULL; childPtr = nextPtr) {
            nextPtr = childPtr->next;
            TeardownTree(treeObjPtr, childPtr);
        }
    }
    if (nodePtr->values != NULL) {
        TreeDestroyValues(nodePtr);
    }
    Rp_PoolFreeItem(treeObjPtr->nodePool, (char *)nodePtr);
}

static void
DestroyTreeObject(TreeObject *treeObjPtr)
{
    Rp_ChainLink *linkPtr;
    TreeClient *clientPtr;

    treeObjPtr->flags |= TREE_DESTROYED;
    treeObjPtr->nNodes = 0;

    /* Remove the remaining clients. */
    for (linkPtr = Rp_ChainFirstLink(treeObjPtr->clients); linkPtr != NULL;
        linkPtr = Rp_ChainNextLink(linkPtr)) {

        clientPtr = Rp_ChainGetValue(linkPtr);
        Rp_ChainDestroy(clientPtr->events);
        Rp_ChainDestroy(clientPtr->traces);
        Rp_Free(clientPtr);
    }
    Rp_ChainDestroy(treeObjPtr->clients);

    TeardownTree(treeObjPtr, treeObjPtr->root);
    Rp_PoolDestroy(treeObjPtr->nodePool);
    Rp_PoolDestroy(treeObjPtr->valuePool);
    Rp_DeleteHashTable(&treeObjPtr->nodeTable);

    if (treeObjPtr->hashPtr != NULL) {
        /* Remove the entry from the global tree table. */
        //Rp_DeleteHashEntry(treeObjPtr->tablePtr, treeObjPtr->hashPtr);
        //if ((treeObjPtr->tablePtr->numEntries == 0) && (keyTableInitialized)) {
        //    keyTableInitialized = FALSE;
        //    Rp_DeleteHashTable(&keyTable);
        //}
    }
    if (treeObjPtr->name != NULL) {
        Rp_Free(treeObjPtr->name);
    }
    Rp_Free(treeObjPtr);
}

/*
 * -----------------------------------------------------------------------
 *
 * TreeInterpDeleteProc --
 *
 *  This is called when the interpreter hosting the tree object
 *  is deleted from the interpreter.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Destroys all remaining trees and removes the hash table
 *  used to register tree names.
 *
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
//static void
//TreeInterpDeleteProc(
//    ClientData clientData,  /* Interpreter-specific data. */
//    )
//    Tcl_Interp *interp)
//{
//    TreeInterpData *dataPtr = clientData;
//    Rp_HashEntry *hPtr;
//    Rp_HashSearch cursor;
//    TreeObject *treeObjPtr;
//
//    for (hPtr = Rp_FirstHashEntry(&dataPtr->treeTable, &cursor);
//        hPtr != NULL; hPtr = Rp_NextHashEntry(&cursor)) {
//
//        treeObjPtr = (TreeObject *)Rp_GetHashValue(hPtr);
//        treeObjPtr->hashPtr = NULL;
//        DestroyTreeObject(treeObjPtr);
//    }
//    if (keyTableInitialized) {
//        keyTableInitialized = FALSE;
//        Rp_DeleteHashTable(&keyTable);
//    }
//    Rp_DeleteHashTable(&dataPtr->treeTable);
//    // Tcl_DeleteAssocData(interp, TREE_THREAD_KEY);
//    Rp_Free(dataPtr);
//}

/*
 *----------------------------------------------------------------------
 *
 * NotifyIdleProc --
 *
 *  Used to invoke event handler routines at some idle point.
 *  This routine is called from the Tcl event loop.  Errors
 *  generated by the event handler routines are backgrounded.
 *
 * Results:
 *  None.
 *
 *---------------------------------------------------------------------- 
 */
//static void
//NotifyIdleProc(ClientData clientData)
//{
//    EventHandler *notifyPtr = clientData;
//    int result;
//
//    notifyPtr->notifyPending = FALSE;
//    notifyPtr->mask |= TREE_NOTIFY_ACTIVE;
//    result = (*notifyPtr->proc)(notifyPtr->clientData, &notifyPtr->event);
//    notifyPtr->mask &= ~TREE_NOTIFY_ACTIVE;
//    if (result != TCL_OK) {
//        Tcl_BackgroundError(notifyPtr->interp);
//    }
//}

/*
 *----------------------------------------------------------------------
 *
 * CheckEventHandlers --
 *
 *  Traverses the list of client event callbacks and checks
 *  if one matches the given event.  A client may trigger an
 *  action that causes the tree to notify it.  The can be
 *  prevented by setting the TREE_NOTIFY_FOREIGN_ONLY bit in
 *  the event handler.
 *
 *  If a matching handler is found, a callback may be called either
 *  immediately or at the next idle time depending upon the
 *  TREE_NOTIFY_WHENIDLE bit.
 *
 *  Since a handler routine may trigger yet another call to
 *  itself, callbacks are ignored while the event handler is
 *  executing.
 *
 * Results:
 *  None.
 *
 *---------------------------------------------------------------------- 
 */
//static void
//CheckEventHandlers(
//    TreeClient *clientPtr,
//    int isSource,       /* Indicates if the client is the source
//                         * of the event. */
//    Rp_TreeNotifyEvent *eventPtr)
//{
//    Rp_ChainLink *linkPtr, *nextPtr;
//    EventHandler *notifyPtr;
//
//    eventPtr->tree = clientPtr;
//    for (linkPtr = Rp_ChainFirstLink(clientPtr->events); 
//        linkPtr != NULL; linkPtr = nextPtr) {
//        nextPtr = Rp_ChainNextLink(linkPtr);
//        notifyPtr = Rp_ChainGetValue(linkPtr);
//        if ((notifyPtr->mask & TREE_NOTIFY_ACTIVE) ||
//            (notifyPtr->mask & eventPtr->type) == 0) {
//            continue;       /* Ignore callbacks that are generated
//                             * inside of a notify handler routine. */
//        }
//        if ((isSource) && (notifyPtr->mask & TREE_NOTIFY_FOREIGN_ONLY)) {
//            continue;       /* Don't notify yourself. */
//        }
//        if (notifyPtr->mask & TREE_NOTIFY_WHENIDLE) {
//            if (!notifyPtr->notifyPending) {
//                notifyPtr->notifyPending = TRUE;
//                notifyPtr->event = *eventPtr;
//                Tcl_DoWhenIdle(NotifyIdleProc, notifyPtr);
//            }
//        } else {
//            int result;
//
//            notifyPtr->mask |= TREE_NOTIFY_ACTIVE;
//            result = (*notifyPtr->proc) (notifyPtr->clientData, eventPtr);
//            notifyPtr->mask &= ~TREE_NOTIFY_ACTIVE;
//            if (result != TCL_OK) {
//                Tcl_BackgroundError(notifyPtr->interp);
//            }
//        }
//    }
//}

/*
 *----------------------------------------------------------------------
 *
 * NotifyClients --
 *
 *  Traverses the list of clients for a particular tree and
 *  notifies each client that an event occurred.  Clients 
 *  indicate interest in a particular event through a bit
 *  flag.
 *
 *---------------------------------------------------------------------- 
 */
//static void
//NotifyClients(
//    TreeClient *sourcePtr,
//    TreeObject *treeObjPtr,
//    Node *nodePtr,
//    int eventFlag)
//{
//    Rp_ChainLink *linkPtr;
//    Rp_TreeNotifyEvent event;
//    TreeClient *clientPtr;
//    int isSource;
//
//    event.type = eventFlag;
//    event.inode = nodePtr->inode;
//
//    /* 
//     * Issue callbacks to each client indicating that a new node has
//     * been created.
//     */
//    for (linkPtr = Rp_ChainFirstLink(treeObjPtr->clients);
//        linkPtr != NULL; linkPtr = Rp_ChainNextLink(linkPtr)) {
//        clientPtr = Rp_ChainGetValue(linkPtr);
//        isSource = (clientPtr == sourcePtr);
//        CheckEventHandlers(clientPtr, isSource, &event);
//    }
//}

static void
FreeValue(Node *nodePtr, Value *valuePtr)
{
    if (valuePtr->objPtr != NULL) {
        // Tcl_DecrRefCount(valuePtr->objPtr);
    }
    Rp_PoolFreeItem(nodePtr->treeObject->valuePool, valuePtr);
}


/* Public Routines */
/*
 *----------------------------------------------------------------------
 *
 * Rp_TreeGetKey --
 *
 *  Given a string, returns a unique identifier for the string.
 *
 *----------------------------------------------------------------------
 */
Rp_TreeKey
Rp_TreeGetKey(string)
    CONST char *string;     /* String to convert. */
{
    Rp_HashEntry *hPtr;
    int isNew;

    if (!keyTableInitialized) {
        Rp_InitHashTable(&keyTable, RP_STRING_KEYS);
        keyTableInitialized = 1;
    }
    hPtr = Rp_CreateHashEntry(&keyTable, string, &isNew);
    return (Rp_TreeKey)Rp_GetHashKey(&keyTable, hPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Rp_TreeCreateNode --
 *
 *  Creates a new node in the given parent node.  The name and
 *  position in the parent are also provided.
 *
 *----------------------------------------------------------------------
 */
Rp_TreeNode
Rp_TreeCreateNode(
    TreeClient *clientPtr,  /* The tree client that is creating
                             * this node.  If NULL, indicates to
                             * trigger notify events on behalf of
                             * the initiating client also. */
    Node *parentPtr,        /* Parent node where the new node will
                             * be inserted. */
    CONST char *name,       /* Name of node. */
    int pos)                /* Position in the parent's list of children
                             * where to insert the new node. */
{
    Rp_HashEntry *hPtr;
    Node *beforePtr;
    Node *nodePtr;  /* Node to be inserted. */
    TreeObject *treeObjPtr;
    int inode;
    int isNew;

    treeObjPtr = parentPtr->treeObject;

    /* Generate an unique serial number for this node.  */
    do {
        inode = treeObjPtr->nextInode++;
        hPtr = Rp_CreateHashEntry(&treeObjPtr->nodeTable,(char *)inode,
                   &isNew);
    } while (!isNew);
    nodePtr = NewNode(treeObjPtr, name, inode);
    Rp_SetHashValue(hPtr, nodePtr);

    if ((pos == -1) || (pos >= (int)parentPtr->nChildren)) {
        beforePtr = NULL;
    } else {
        beforePtr = parentPtr->first;
        while ((pos > 0) && (beforePtr != NULL)) {
            pos--;
            beforePtr = beforePtr->next;
        }
    }
    LinkBefore(parentPtr, nodePtr, beforePtr);
    nodePtr->depth = parentPtr->depth + 1;
    /*
     * Issue callbacks to each client indicating that a new node has
     * been created.
     */
    // NotifyClients(clientPtr, treeObjPtr, nodePtr, TREE_NOTIFY_CREATE);
    return nodePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Rp_TreeCreateNodeWithId --
 *
 *  Like Rp_TreeCreateNode, but provides a specific id to use
 *  for the node.  If the tree already contains a node by that
 *  id, NULL is returned.
 *
 *----------------------------------------------------------------------
 */
Rp_TreeNode
Rp_TreeCreateNodeWithId(
    TreeClient *clientPtr,
    Node *parentPtr,        /* Parent node where the new node will
                             * be inserted. */
    CONST char *name,       /* Name of node. */
    int inode,              /* Requested id of the new node. If a
                             * node by this id already exists in the
                             * tree, no node is created. */
    int position)           /* Position in the parent's list of children
                             * where to insert the new node. */
{
    Rp_HashEntry *hPtr;
    Node *beforePtr;
    Node *nodePtr;  /* Node to be inserted. */
    TreeObject *treeObjPtr;
    int isNew;

    treeObjPtr = parentPtr->treeObject;
    hPtr = Rp_CreateHashEntry(&treeObjPtr->nodeTable,(char *)inode, &isNew);
    if (!isNew) {
        return NULL;
    }
    nodePtr = NewNode(treeObjPtr, name, inode);
    Rp_SetHashValue(hPtr, nodePtr);

    if ((position == -1) || (position >= (int)parentPtr->nChildren)) {
        beforePtr = NULL;
    } else {
        beforePtr = parentPtr->first;
        while ((position > 0) && (beforePtr != NULL)) {
            position--;
            beforePtr = beforePtr->next;
        }
    }
    LinkBefore(parentPtr, nodePtr, beforePtr);
    nodePtr->depth = parentPtr->depth + 1;
    /*
     * Issue callbacks to each client indicating that a new node has
     * been created.
     */
    // NotifyClients(clientPtr, treeObjPtr, nodePtr, TREE_NOTIFY_CREATE);
    return nodePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Rp_TreeMoveNode --
 *
 *  Move an entry into a new location in the hierarchy.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
Rp_TreeMoveNode(
    TreeClient *clientPtr,
    Node *nodePtr,
    Node *parentPtr,
    Node *beforePtr)
{
    // TreeObject *treeObjPtr = nodePtr->treeObject;
    int newDepth;

    if (nodePtr == beforePtr) {
        return RP_ERROR;
    }
    if ((beforePtr != NULL) && (beforePtr->parent != parentPtr)) {
        return RP_ERROR;
    }
    if (nodePtr->parent == NULL) {
        return RP_ERROR;   /* Can't move root. */
    }
    /* Verify that the node isn't an ancestor of the new parent. */
    if (Rp_TreeIsAncestor(nodePtr, parentPtr)) {
        return RP_ERROR;
    }
    UnlinkNode(nodePtr);
    /*
     * Relink the node as a child of the new parent.
     */
    LinkBefore(parentPtr, nodePtr, beforePtr);
    newDepth = parentPtr->depth + 1;
    if (nodePtr->depth != newDepth) {
        /* Reset the depths of all descendant nodes. */
        ResetDepths(nodePtr, newDepth);
    }

    /*
     * Issue callbacks to each client indicating that a node has
     * been moved.
     */
    // NotifyClients(clientPtr, treeObjPtr, nodePtr, TREE_NOTIFY_MOVE);
    return RP_OK;
}

int
Rp_TreeDeleteNode(TreeClient *clientPtr, Node *nodePtr)
{
    TreeObject *treeObjPtr = nodePtr->treeObject;
    Node *childPtr, *nextPtr;

    /* In depth-first order, delete each descendant node. */
    for (childPtr = nodePtr->first; childPtr != NULL; childPtr = nextPtr) {
        nextPtr = childPtr->next;
        Rp_TreeDeleteNode(clientPtr, childPtr);
    }
    /*
     * Issue callbacks to each client indicating that the node can
     * no longer be used.
     */
    // NotifyClients(clientPtr, treeObjPtr, nodePtr, TREE_NOTIFY_DELETE);

    /* Now remove the actual node. */
    FreeNode(treeObjPtr, nodePtr);
    return RP_OK;
}

Rp_TreeNode
Rp_TreeGetNode(TreeClient *clientPtr, unsigned int inode)
{
    TreeObject *treeObjPtr = clientPtr->treeObject;
    Rp_HashEntry *hPtr;

    hPtr = Rp_FindHashEntry(&treeObjPtr->nodeTable, (char *)inode);
    if (hPtr != NULL) {
        return (Rp_TreeNode)Rp_GetHashValue(hPtr);
    }
    return NULL;
}

Rp_TreeTrace
Rp_TreeCreateTrace(
    TreeClient *clientPtr,
    Node *nodePtr,
    CONST char *keyPattern,
    CONST char *tagName,
    unsigned int mask,
    Rp_TreeTraceProc *proc,
    ClientData clientData)
{
    TraceHandler *tracePtr;

    tracePtr = Rp_Calloc(1, sizeof (TraceHandler));
    assert(tracePtr);
    tracePtr->linkPtr = Rp_ChainAppend(clientPtr->traces, tracePtr);
    if (keyPattern != NULL) {
        tracePtr->keyPattern = Rp_Strdup(keyPattern);
    }
    if (tagName != NULL) {
        tracePtr->withTag = Rp_Strdup(tagName);
    }
    tracePtr->clientPtr = clientPtr;
    tracePtr->proc = proc;
    tracePtr->clientData = clientData;
    tracePtr->mask = mask;
    tracePtr->nodePtr = nodePtr;
    return (Rp_TreeTrace)tracePtr;
}

void
Rp_TreeDeleteTrace(Rp_TreeTrace trace)
{
    TraceHandler *tracePtr = (TraceHandler *)trace;

    Rp_ChainDeleteLink(tracePtr->clientPtr->traces, tracePtr->linkPtr);
    if (tracePtr->keyPattern != NULL) {
        Rp_Free(tracePtr->keyPattern);
    }
    if (tracePtr->withTag != NULL) {
        Rp_Free(tracePtr->withTag);
    }
    Rp_Free(tracePtr);
}

void
Rp_TreeRelabelNode(TreeClient *clientPtr, Node *nodePtr, CONST char *string)
{
    nodePtr->label = Rp_TreeGetKey(string);
    /* 
     * Issue callbacks to each client indicating that a new node has
     * been created.
     */
    // NotifyClients(clientPtr, clientPtr->treeObject, nodePtr,
    //               TREE_NOTIFY_RELABEL);
}

void
Rp_TreeRelabelNode2(Node *nodePtr, CONST char *string)
{
    nodePtr->label = Rp_TreeGetKey(string);
}

/*
 *----------------------------------------------------------------------
 *
 * Rp_TreeFindChild --
 *
 *  Searches for the named node in a parent's chain of siblings.
 *
 *
 * Results:
 *  If found, the child node is returned, otherwise NULL.
 *
 *----------------------------------------------------------------------
 */
Rp_TreeNode
Rp_TreeFindChild(Node *parentPtr, CONST char *string)
{
    Rp_TreeKey label;
    register Node *nodePtr;

    label = Rp_TreeGetKey(string);
    for (nodePtr = parentPtr->first; nodePtr != NULL; nodePtr = nodePtr->next) {
        if (label == nodePtr->label) {
            return nodePtr;
        }
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Rp_TreeNodePosition --
 *
 *  Returns the position of the node in its parent's list of
 *  children.  The root's position is 0.
 *
 *----------------------------------------------------------------------
 */
int
Rp_TreeNodePosition(Node *nodePtr)
{
    Node *parentPtr;
    int count;

    count = 0;
    parentPtr = nodePtr->parent;
    if (parentPtr != NULL) {
        Node *childPtr;

        for (childPtr = parentPtr->first; childPtr != NULL;
             childPtr = childPtr->next) {
            if (nodePtr == childPtr) {
                break;
            }
            count++;
        }
    }
    return count;
}


/*
 *----------------------------------------------------------------------
 *
 * Rp_TreePrevNode --
 *
 *  Returns the "previous" node in the tree.  This node (in
 *  depth-first order) is its parent, if the node has no siblings
 *  that are previous to it.  Otherwise it is the last descendant
 *  of the last sibling.  In this case, descend the sibling's
 *  hierarchy, using the last child at any ancestor, with we
 *  we find a leaf.
 *
 *----------------------------------------------------------------------
 */
Rp_TreeNode
Rp_TreePrevNode(Node *rootPtr, Node *nodePtr)
{
    Node *prevPtr;

    if (nodePtr == rootPtr) {
        return NULL;        /* The root is the first node. */
    }
    prevPtr = nodePtr->prev;
    if (prevPtr == NULL) {
        /* There are no siblings previous to this one, so pick the parent. */
        return nodePtr->parent;
    }
    /*
     * Traverse down the right-most thread, in order to select the
     * next entry.  Stop when we reach a leaf.
     */
    nodePtr = prevPtr;
    while ((prevPtr = nodePtr->last) != NULL) {
        nodePtr = prevPtr;
    }
    return nodePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Rp_TreeNextNode --
 *
 *  Returns the "next" node in relation to the given node.
 *  The next node (in depth-first order) is either the first
 *  child of the given node the next sibling if the node has
 *  no children (the node is a leaf).  If the given node is the
 *  last sibling, then try it's parent next sibling.  Continue
 *  until we either find a next sibling for some ancestor or
 *  we reach the root node.  In this case the current node is
 *  the last node in the tree.
 *
 *----------------------------------------------------------------------
 */
Rp_TreeNode
Rp_TreeNextNode(Node *rootPtr, Node *nodePtr)
{
    Node *nextPtr;

    /* Pick the first sub-node. */
    nextPtr = nodePtr->first;
    if (nextPtr != NULL) {
        return nextPtr;
    }
    /* 
     * Back up until we can find a level where we can pick a
     * "next sibling".  For the last entry we'll thread our
     * way back to the root.
     */
    while (nodePtr != rootPtr) {
        nextPtr = nodePtr->next;
        if (nextPtr != NULL) {
            return nextPtr;
        }
        nodePtr = nodePtr->parent;
    }
    return NULL;        /* At root, no next node. */
}


int
Rp_TreeIsBefore(Node *n1Ptr, Node *n2Ptr)
{
    int depth;
    register int i;
    Node *nodePtr;

    if (n1Ptr == n2Ptr) {
        return FALSE;
    }
    depth = MIN(n1Ptr->depth, n2Ptr->depth);
    if (depth == 0) {       /* One of the nodes is root. */
        return (n1Ptr->parent == NULL);
    }
    /* 
     * Traverse back from the deepest node, until both nodes are at
     * the same depth.  Check if this ancestor node is the same for
     * both nodes.
     */
    for (i = n1Ptr->depth; i > depth; i--) {
        n1Ptr = n1Ptr->parent;
    }
    if (n1Ptr == n2Ptr) {
        return FALSE;
    }
    for (i = n2Ptr->depth; i > depth; i--) {
        n2Ptr = n2Ptr->parent;
    }
    if (n2Ptr == n1Ptr) {
        return TRUE;
    }

    /*
     * First find the mutual ancestor of both nodes.  Look at each
     * preceding ancestor level-by-level for both nodes.  Eventually
     * we'll find a node that's the parent of both ancestors.  Then
     * find the first ancestor in the parent's list of subnodes.
     */
    for (i = depth; i > 0; i--) {
        if (n1Ptr->parent == n2Ptr->parent) {
            break;
        }
        n1Ptr = n1Ptr->parent;
        n2Ptr = n2Ptr->parent;
    }
    for (nodePtr = n1Ptr->parent->first; nodePtr != NULL;
         nodePtr = nodePtr->next) {
        if (nodePtr == n1Ptr) {
            return TRUE;
        } else if (nodePtr == n2Ptr) {
            return FALSE;
        }
    }
    return FALSE;
}

//static void
//CallTraces(
//    Tcl_Interp *interp,
//    TreeClient *sourcePtr,  /* Client holding a reference to the
//                             * tree.  If NULL, indicates to
//                             * execute all handlers, including
//                             * those of the caller. */
//    TreeObject *treeObjPtr, /* Tree that was changed. */
//    Node *nodePtr,          /* Node that received the event. */
//    Rp_TreeKey key,
//    unsigned int flags)
//{
//    Rp_ChainLink *l1Ptr, *l2Ptr;
//    TreeClient *clientPtr;
//    TraceHandler *tracePtr;
//
//    for(l1Ptr = Rp_ChainFirstLink(treeObjPtr->clients);
//        l1Ptr != NULL; l1Ptr = Rp_ChainNextLink(l1Ptr)) {
//        clientPtr = Rp_ChainGetValue(l1Ptr);
//        for(l2Ptr = Rp_ChainFirstLink(clientPtr->traces);
//            l2Ptr != NULL; l2Ptr = Rp_ChainNextLink(l2Ptr)) {
//            tracePtr = Rp_ChainGetValue(l2Ptr);
//            if ((tracePtr->keyPattern != NULL) &&
//                (!Tcl_StringMatch(key, tracePtr->keyPattern))) {
//                continue;   /* Key pattern doesn't match. */
//            }
//            if ((tracePtr->withTag != NULL) &&
//                (!Rp_TreeHasTag(clientPtr, nodePtr, tracePtr->withTag))) {
//                continue;   /* Doesn't have the tag. */
//            }
//            if ((tracePtr->mask & flags) == 0) {
//                continue;   /* Flags don't match. */
//            }
//            if ((clientPtr == sourcePtr) &&
//                (tracePtr->mask & TREE_TRACE_FOREIGN_ONLY)) {
//                continue;   /* This client initiated the trace. */
//            }
//            if ((tracePtr->nodePtr != NULL) && (tracePtr->nodePtr != nodePtr)) {
//                continue;   /* Nodes don't match. */
//            }
//            nodePtr->flags |= TREE_TRACE_ACTIVE;
//            if ((*tracePtr->proc) (tracePtr->clientData, treeObjPtr->interp,
//                nodePtr, key, flags) != TCL_OK) {
//                if (interp != NULL) {
//                    Tcl_BackgroundError(interp);
//                }
//            }
//            nodePtr->flags &= ~TREE_TRACE_ACTIVE;
//        }
//    }
//}

static Value *
GetTreeValue(
    TreeClient *clientPtr,
    Node *nodePtr,
    Rp_TreeKey key)
{
    register Value *valuePtr;

    valuePtr = TreeFindValue(nodePtr, key);
    if (valuePtr == NULL) {
        // fprintf(stderr, "can't find field \"%s\"", key);
        return NULL;
    }
    if ((valuePtr->owner != NULL) && (valuePtr->owner != clientPtr)) {
        // fprintf(stderr, "can't access private field \"%s\"", key);
        return NULL;
    }
    return valuePtr;
}

int
Rp_TreePrivateValue(
    TreeClient *clientPtr,
    Node *nodePtr,
    Rp_TreeKey key)
{
    Value *valuePtr;

    valuePtr = TreeFindValue(nodePtr, key);
    if (valuePtr == NULL) {
        // fprintf(stderr, "can't find field \"%s\"", key);
        return RP_ERROR;
    }
    valuePtr->owner = clientPtr;
    return RP_OK;
}

int
Rp_TreePublicValue(
    TreeClient *clientPtr,
    Node *nodePtr,
    Rp_TreeKey key)
{
    Value *valuePtr;

    valuePtr = TreeFindValue(nodePtr, key);
    if (valuePtr == NULL) {
        // fprintf(stderr, "can't find field \"%s\"", key,);
        return RP_ERROR;
    }
    if (valuePtr->owner != clientPtr) {
        // fprintf(interp, "not the owner of \"%s\"", key);
        return RP_ERROR;
    }
    valuePtr->owner = NULL;
    return RP_OK;
}

int
Rp_TreeValueExistsByKey(clientPtr, nodePtr, key)
    TreeClient *clientPtr;
    Node *nodePtr;
    Rp_TreeKey key;
{
    register Value *valuePtr;

    valuePtr = GetTreeValue(clientPtr, nodePtr, key);
    if (valuePtr == NULL) {
        return FALSE;
    }
    return TRUE;
}

int
Rp_TreeGetValueByKey(
    TreeClient *clientPtr,
    Node *nodePtr,
    Rp_TreeKey key,
    void **objPtrPtr)
{
    register Value *valuePtr;
    // TreeObject *treeObjPtr = nodePtr->treeObject;

    valuePtr = GetTreeValue(clientPtr, nodePtr, key);
    if (valuePtr == NULL) {
        return RP_ERROR;
    }
    *objPtrPtr = valuePtr->objPtr;
    if (!(nodePtr->flags & TREE_TRACE_ACTIVE)) {
//        CallTraces(interp, clientPtr, treeObjPtr, nodePtr, key,
//               TREE_TRACE_READ);
    }
    return RP_OK;
}

int
Rp_TreeSetValueByKey(
    TreeClient *clientPtr,
    Node *nodePtr,          /* Node to be updated. */
    Rp_TreeKey key,         /* Identifies the field key. */
    void *objPtr)           /* New value of field. */
{
    // TreeObject *treeObjPtr = nodePtr->treeObject;
    Value *valuePtr;
    unsigned int flags;
    int isNew;

    assert(objPtr != NULL);
    valuePtr = TreeCreateValue(nodePtr, key, &isNew);
    if ((valuePtr->owner != NULL) && (valuePtr->owner != clientPtr)) {
//        fprintf(stderr,"can't set private field \"%s\"",key);
        return RP_ERROR;
    }
    if (objPtr != valuePtr->objPtr) {
//        Tcl_IncrRefCount(objPtr);
//        if (valuePtr->objPtr != NULL) {
//            Tcl_DecrRefCount(valuePtr->objPtr);
//        }
        valuePtr->objPtr = objPtr;
    }
    flags = TREE_TRACE_WRITE;
    if (isNew) {
        flags |= TREE_TRACE_CREATE;
    }
    if (!(nodePtr->flags & TREE_TRACE_ACTIVE)) {
//        CallTraces(interp, clientPtr, treeObjPtr, nodePtr, valuePtr->key,
//                flags);
    }
    return RP_OK;
}

int
Rp_TreeUnsetValueByKey(
    TreeClient *clientPtr,
    Node *nodePtr,      /* Node to be updated. */
    Rp_TreeKey key)     /* Name of field in node. */
{
    // TreeObject *treeObjPtr = nodePtr->treeObject;
    Value *valuePtr;

    valuePtr = TreeFindValue(nodePtr, key);
    if (valuePtr == NULL) {
        return RP_OK;      /* It's okay to unset values that don't
                             * exist in the node. */
    }
    if ((valuePtr->owner != NULL) && (valuePtr->owner != clientPtr)) {
        // fprintf(stderr, "can't unset private field \"%s\"", key);
        return RP_ERROR;
    }
    TreeDeleteValue(nodePtr, valuePtr);
//    CallTraces(interp, clientPtr, treeObjPtr, nodePtr, key, TREE_TRACE_UNSET);
    return RP_OK;
}

static int
ParseParentheses(
    CONST char *string,
    char **leftPtr,
    char **rightPtr)
{
    register char *p;
    char *left, *right;

    left = right = NULL;
    for (p = (char *)string; *p != '\0'; p++) {
        if (*p == '(') {
            left = p;
        } else if (*p == ')') {
            right = p;
        }
    }
    if (left != right) {
        if (((left != NULL) && (right == NULL)) ||
            ((left == NULL) && (right != NULL)) ||
            (left > right) || (right != (p - 1))) {
            fprintf(stderr, "bad array specification \"%s\"", string);
            return RP_ERROR;
        }
    }
    *leftPtr = left;
    *rightPtr = right;
    return RP_OK;
}

//FIXME: commented out because it calls Rp_TreeGetArrayValue
//int
//Rp_TreeGetValue(
//    TreeClient *clientPtr,
//    Node *nodePtr,
//    CONST char *string,     /* String identifying the field in node. */
//    void **objPtrPtr)
//{
//    char *left, *right;
//    int result;
//
//    if (ParseParentheses(string, &left, &right) != TCL_OK) {
//        return TCL_ERROR;
//    }
//    if (left != NULL) {
//        *left = *right = '\0';
//        result = Rp_TreeGetArrayValue(clientPtr, nodePtr, string, left + 1, objPtrPtr);
//        *left = '(', *right = ')';
//    } else {
//        result = Rp_TreeGetValueByKey(clientPtr, nodePtr,
//                    Rp_TreeGetKey(string), objPtrPtr);
//    }
//    return result;
//}

//FIXME: commented out because it calls Rp_TreeSetArrayValue
//int
//Rp_TreeSetValue(
//    TreeClient *clientPtr,
//    Node *nodePtr,          /* Node to be updated. */
//    CONST char *string,     /* String identifying the field in node. */
//    void *valueObjPtr)      /* New value of field. If NULL, field
//                             * is deleted. */
//{
//    char *left, *right;
//    int result;
//
//    if (ParseParentheses(string, &left, &right) != RP_OK) {
//        return RP_ERROR;
//    }
//    if (left != NULL) {
//        *left = *right = '\0';
//        result = Rp_TreeSetArrayValue(clientPtr, nodePtr, string, left + 1, valueObjPtr);
//        fprintf(stderr, "Rp_TreeSetArrayValue not supported\n", name);
//
//        *left = '(', *right = ')';
//    } else {
//        result = Rp_TreeSetValueByKey(clientPtr, nodePtr,
//                    Rp_TreeGetKey(string), valueObjPtr);
//    }
//    return result;
//}

//FIXME: commented out because it calls Rp_TreeUnsetArrayValue
//int
//Rp_TreeUnsetValue(
//    TreeClient *clientPtr,
//    Node *nodePtr,          /* Node to be updated. */
//    CONST char *string)     /* String identifying the field in node. */
//{
//    char *left, *right;
//    int result;
//
//    if (ParseParentheses(string, &left, &right) != RP_OK) {
//        return RP_ERROR;
//    }
//    if (left != NULL) {
//        *left = *right = '\0';
//        result = Rp_TreeUnsetArrayValue(clientPtr, nodePtr, string, left + 1);
//        *left = '(', *right = ')';
//    } else {
//        result = Rp_TreeUnsetValueByKey(clientPtr, nodePtr, Rp_TreeGetKey(string));
//    }
//    return result;
//}

//FIXME: commented out because it calls Rp_TreeArrayValueExists
//int
//Rp_TreeValueExists(TreeClient *clientPtr, Node *nodePtr, CONST char *string)
//{
//    char *left, *right;
//    int result;
//
//    if (ParseParentheses(string, &left, &right) != RP_OK) {
//        return FALSE;
//    }
//    if (left != NULL) {
//        *left = *right = '\0';
//        result = Rp_TreeArrayValueExists(clientPtr, nodePtr, string, left + 1);
//        *left = '(', *right = ')';
//    } else {
//        result = Rp_TreeValueExistsByKey(clientPtr, nodePtr,
//                Rp_TreeGetKey(string));
//    }
//    return result;
//}

Rp_TreeKey
Rp_TreeFirstKey(
    TreeClient *clientPtr,
    Node *nodePtr,
    Rp_TreeKeySearch *iterPtr)
{
    Value *valuePtr;

    valuePtr = TreeFirstValue(nodePtr, iterPtr);
    if (valuePtr == NULL) {
        return NULL;
    }
    while ((valuePtr->owner != NULL) && (valuePtr->owner != clientPtr)) {
        valuePtr = TreeNextValue(iterPtr);
        if (valuePtr == NULL) {
            return NULL;
        }
    }
    return valuePtr->key;
}

Rp_TreeKey
Rp_TreeNextKey(TreeClient *clientPtr, Rp_TreeKeySearch *iterPtr)
{
    Value *valuePtr;

    valuePtr = TreeNextValue(iterPtr);
    if (valuePtr == NULL) {
        return NULL;
    }
    while ((valuePtr->owner != NULL) && (valuePtr->owner != clientPtr)) {
        valuePtr = TreeNextValue(iterPtr);
        if (valuePtr == NULL) {
            return NULL;
        }
    }
    return valuePtr->key;
}

int
Rp_TreeIsAncestor(Node *n1Ptr, Node *n2Ptr)
{
    if (n2Ptr != NULL) {
        n2Ptr = n2Ptr->parent;
        while (n2Ptr != NULL) {
            if (n2Ptr == n1Ptr) {
                return TRUE;
            }
            n2Ptr = n2Ptr->parent;
        }
    }
    return FALSE;
}

/*
 *----------------------------------------------------------------------
 *
 * Rp_TreeSortNode --
 *
 *  Sorts the subnodes at a given node.
 *
 * Results:
 *  Always returns TCL_OK.
 *
 *----------------------------------------------------------------------
 */
int
Rp_TreeSortNode(
    TreeClient *clientPtr,
    Node *nodePtr,
    Rp_TreeCompareNodesProc *proc)
{
    Node **nodeArr;
    Node *childPtr;
    int nNodes;
    register Node **p;

    nNodes = nodePtr->nChildren;
    if (nNodes < 2) {
        return RP_OK;
    }
    nodeArr = Rp_Malloc((nNodes + 1) * sizeof(Node *));
    if (nodeArr == NULL) {
        return RP_ERROR;   /* Out of memory. */
    }
    for (p = nodeArr, childPtr = nodePtr->first; childPtr != NULL; 
         childPtr = childPtr->next, p++) {
        *p = childPtr;
    }
    *p = NULL;

    qsort((char *)nodeArr, nNodes, sizeof(Node *), (QSortCompareProc *)proc);
    for (p = nodeArr; *p != NULL; p++) {
        UnlinkNode(*p);
        LinkBefore(nodePtr, *p, (Rp_TreeNode)NULL);
    }
    Rp_Free(nodeArr);
    // NotifyClients(clientPtr, nodePtr->treeObject, nodePtr, TREE_NOTIFY_SORT);
    return RP_OK;
}

#define TEST_RESULT(result) \
    switch (result) { \
    case RP_CONTINUE: \
        return RP_OK; \
    case RP_OK: \
        break; \
    default: \
        return (result); \
    }

int
Rp_TreeApply(
    Node *nodePtr,          /* Root node of subtree. */
    Rp_TreeApplyProc *proc, /* Procedure to call for each node. */
    ClientData clientData)  /* One-word of data passed when calling
                             * proc. */
{
    Node *childPtr, *nextPtr;
    int result;

    for (childPtr = nodePtr->first; childPtr != NULL; childPtr = nextPtr) {

        /*
         * Get the next link in the chain before calling Rp_TreeApply
         * recursively.  This is because the apply callback may delete
         * the node and its link.
         */

        nextPtr = childPtr->next;
        result = Rp_TreeApply(childPtr, proc, clientData);
        TEST_RESULT(result);
    }
    return (*proc) (nodePtr, clientData, TREE_POSTORDER);
}

int
Rp_TreeApplyDFS(
    Node *nodePtr,          /* Root node of subtree. */
    Rp_TreeApplyProc *proc, /* Procedure to call for each node. */
    ClientData clientData,  /* One-word of data passed when calling
                             * proc. */
    int order)              /* Order of traversal. */
{
    Node *childPtr, *nextPtr;
    int result;

    if (order & TREE_PREORDER) {
        result = (*proc) (nodePtr, clientData, TREE_PREORDER);
        TEST_RESULT(result);
    }
    childPtr = nodePtr->first;
    if (order & TREE_INORDER) {
        if (childPtr != NULL) {
            result = Rp_TreeApplyDFS(childPtr, proc, clientData, order);
            TEST_RESULT(result);
            childPtr = childPtr->next;
        }
        result = (*proc) (nodePtr, clientData, TREE_INORDER);
        TEST_RESULT(result);
    }
    for (/* empty */; childPtr != NULL; childPtr = nextPtr) {
        /*
         * Get the next link in the chain before calling
         * Rp_TreeApply recursively.  This is because the
         * apply callback may delete the node and its link.
         */
        nextPtr = childPtr->next;
        result = Rp_TreeApplyDFS(childPtr, proc, clientData, order);
        TEST_RESULT(result);
    }
    if (order & TREE_POSTORDER) {
        return (*proc) (nodePtr, clientData, TREE_POSTORDER);
    }
    return RP_OK;
}

int
Rp_TreeApplyBFS(nodePtr, proc, clientData)
    Node *nodePtr;          /* Root node of subtree. */
    Rp_TreeApplyProc *proc; /* Procedure to call for each node. */
    ClientData clientData;  /* One-word of data passed when calling
                             * proc. */
{
    Rp_Chain *queuePtr;
    Rp_ChainLink *linkPtr, *nextPtr;
    Node *childPtr;
    int result;

    queuePtr = Rp_ChainCreate();
    linkPtr = Rp_ChainAppend(queuePtr, nodePtr);
    while (linkPtr != NULL) {
        nodePtr = Rp_ChainGetValue(linkPtr);
        /* Add the children to the queue. */
        for (childPtr = nodePtr->first; childPtr != NULL;
             childPtr = childPtr->next) {
            Rp_ChainAppend(queuePtr, childPtr);
        }
        /* Process the node. */
        result = (*proc) (nodePtr, clientData, TREE_BREADTHFIRST);
        switch (result) {
        case RP_CONTINUE:
            Rp_ChainDestroy(queuePtr);
            return RP_OK;
        case RP_OK:
            break;
        default:
            Rp_ChainDestroy(queuePtr);
            return result;
        }
        /* Remove the node from the queue. */
        nextPtr = Rp_ChainNextLink(linkPtr);
        Rp_ChainDeleteLink(queuePtr, linkPtr);
        linkPtr = nextPtr;
    }
    Rp_ChainDestroy(queuePtr);
    return RP_OK;
}

static TreeClient *
NewTreeClient(TreeObject *treeObjPtr)
{
    TreeClient *clientPtr;

    clientPtr = Rp_Calloc(1, sizeof(TreeClient));
    if (clientPtr != NULL) {
        Rp_TreeTagTable *tablePtr;

        clientPtr->magic = TREE_MAGIC;
        clientPtr->linkPtr = Rp_ChainAppend(treeObjPtr->clients, clientPtr);
        clientPtr->events = Rp_ChainCreate();
        clientPtr->traces = Rp_ChainCreate();
        clientPtr->treeObject = treeObjPtr;
        clientPtr->root = treeObjPtr->root;
        tablePtr = Rp_Malloc(sizeof(Rp_TreeTagTable));
        Rp_InitHashTable(&tablePtr->tagTable, RP_STRING_KEYS);
        tablePtr->refCount = 1;
        clientPtr->tagTablePtr = tablePtr;
    }
    return clientPtr;
}

int
Rp_TreeCreate(
//    Tcl_Interp *interp,         /* Interpreter to report errors back to. */
    CONST char *name,           /* Name of tree in namespace.  Tree
                                 * must not already exist. */
    TreeClient **clientPtrPtr)  /* (out) Client token of newly created
                                 * tree.  Releasing the token will
                                 * free the tree.  If NULL, no token
                                 * is generated. */
{
//    Tcl_DString dString;
//    Tcl_Namespace *nsPtr;
//    TreeInterpData *dataPtr;
    TreeObject *treeObjPtr;
    // CONST char *treeName;
//    char string[200];

// we do not allow unnamed trees
//    // dataPtr = GetTreeInterpData(interp);
//    if (name != NULL) {
//        // FIXME: because dsk broke it
//        /* Check if this tree already exists the current namespace. */
//        /*
//        treeObjPtr = GetTreeObject(interp, name, NS_SEARCH_CURRENT);
//        if (treeObjPtr != NULL) {
//            Tcl_AppendResult(interp, "a tree object \"", name,
//                             "\" already exists", (char *)NULL);
//            return TCL_ERROR;
//        }
//        */
//    } else {
//        // FIXME: because dsk broke it
//        /* Generate a unique tree name in the current namespace. */
//        /*
//        do  {
//            sprintf(string, "tree%d", dataPtr->nextId++);
//        } while (GetTreeObject(interp, name, NS_SEARCH_CURRENT) != NULL);
//        */
//        sprintf(string, "tree%d", dataPtr->nextId++);
//        name = string;
//    }
    /*
     * Tear apart and put back together the namespace-qualified name
     * of the tree. This is to ensure that naming is consistent.
     */
//    treeName = name;
//    if (Rp_ParseQualifiedName(name, &nsPtr, &treeName) != RP_OK) {
//        Tcl_AppendResult(interp, "can't find namespace in \"", name, "\"",
//                (char *)NULL);
//        return TCL_ERROR;
//    }
//    if (nsPtr == NULL) {
//        /*
//         * Note: Unlike Tcl_CreateCommand, an unqualified name 
//         * doesn't imply the global namespace, but the current one.
//         */
//        nsPtr = Tcl_GetCurrentNamespace(interp);
//    }
//    name = Rp_GetQualifiedName(nsPtr, treeName, &dString);
    // treeObjPtr = NewTreeObject(dataPtr, name);
    treeObjPtr = NewTreeObject(name);
    if (treeObjPtr == NULL) {
        fprintf(stderr, "can't allocate tree \"%s\"", name);
//        Tcl_DStringFree(&dString);
        return RP_ERROR;
    }
//    Tcl_DStringFree(&dString);
    if (clientPtrPtr != NULL) {
        TreeClient *clientPtr;

        clientPtr = NewTreeClient(treeObjPtr);
        if (clientPtr == NULL) {
            fprintf(stderr, "can't allocate tree token");
            return RP_ERROR;
        }
        *clientPtrPtr = clientPtr;
    }
    return RP_OK;
}

//int
//Rp_TreeGetToken(
//    CONST char *name,       /* Name of tree in namespace. */
//    TreeClient **clientPtrPtr)
//{
//    TreeClient *clientPtr;
//    TreeObject *treeObjPtr;
//
//    // FIXME: still need to find the tree where ever we store it.
//    // possibly store them in a global hashtable?
//    // or remove this function all together
//    /* find the tree */
//    // treeObjPtr = GetTreeObject(interp, name, NS_SEARCH_BOTH);
//    if (treeObjPtr == NULL) {
//        // fprintf(stderr, "can't find a tree object \"%s\"", name);
//        return RP_ERROR;
//    }
//    clientPtr = NewTreeClient(treeObjPtr);
//    if (clientPtr == NULL) {
//        // fprintf(stderr, "can't allocate token for tree \"%s\"", name);
//        return RP_ERROR;
//    }
//    *clientPtrPtr = clientPtr;
//    return RP_OK;
//}

void
Rp_TreeReleaseToken(TreeClient *clientPtr)
{
    TreeObject *treeObjPtr;
    Rp_ChainLink *linkPtr;
    EventHandler *notifyPtr;
    TraceHandler *tracePtr;

    if (clientPtr->magic != TREE_MAGIC) {
        fprintf(stderr, "invalid tree object token 0x%lx\n",
                (unsigned long)clientPtr);
        return;
    }
    /* Remove any traces that may be set. */
    for (linkPtr = Rp_ChainFirstLink(clientPtr->traces); linkPtr != NULL;
         linkPtr = Rp_ChainNextLink(linkPtr)) {
        tracePtr = Rp_ChainGetValue(linkPtr);
        if (tracePtr->keyPattern != NULL) {
            Rp_Free(tracePtr->keyPattern);
        }
        Rp_Free(tracePtr);
    }
    Rp_ChainDestroy(clientPtr->traces);
    /* And any event handlers. */
    for(linkPtr = Rp_ChainFirstLink(clientPtr->events);
        linkPtr != NULL; linkPtr = Rp_ChainNextLink(linkPtr)) {
        notifyPtr = Rp_ChainGetValue(linkPtr);
        // FIXME: need to cancel any waiting calls
        /*
        if (notifyPtr->notifyPending) {
            Tcl_CancelIdleCall(NotifyIdleProc, notifyPtr);
        }
        */
        Rp_Free(notifyPtr);
    }
    if (clientPtr->tagTablePtr != NULL) {
        ReleaseTagTable(clientPtr->tagTablePtr);
    }
    Rp_ChainDestroy(clientPtr->events);
    treeObjPtr = clientPtr->treeObject;
    if (treeObjPtr != NULL) {
        /* Remove the client from the server's list */
        Rp_ChainDeleteLink(treeObjPtr->clients, clientPtr->linkPtr);
        if (Rp_ChainGetLength(treeObjPtr->clients) == 0) {
            DestroyTreeObject(treeObjPtr);
        }
    }
    clientPtr->magic = 0;
    Rp_Free(clientPtr);
}

//int
//Rp_TreeExists(interp, name)
//    Tcl_Interp *interp;     /* Interpreter to report errors back to. */
//    CONST char *name;       /* Name of tree in designated namespace. */
//{
//    TreeObject *treeObjPtr;
//
//    treeObjPtr = GetTreeObject(interp, name, NS_SEARCH_BOTH);
//    if (treeObjPtr == NULL) {
//        Tcl_ResetResult(interp);
//        return 0;
//    }
//    return 1;
//}

/*ARGSUSED*/
static int
SizeApplyProc(
    Node *nodePtr,      /* Not used. */
    ClientData clientData,
    int order)          /* Not used. */
{
    int *sumPtr = clientData;
    *sumPtr = *sumPtr + 1;
    return RP_OK;
}

int
Rp_TreeSize(Node *nodePtr)
{
    int sum;

    sum = 0;
    Rp_TreeApply(nodePtr, SizeApplyProc, &sum);
    return sum;
}


void
Rp_TreeCreateEventHandler(
    TreeClient *clientPtr,
    unsigned int mask,
    Rp_TreeNotifyEventProc *proc,
    ClientData clientData)
{
    Rp_ChainLink *linkPtr;
    EventHandler *notifyPtr;

    notifyPtr = NULL;       /* Suppress compiler warning. */

    /* Check if the event is already handled. */
    for(linkPtr = Rp_ChainFirstLink(clientPtr->events);
        linkPtr != NULL; linkPtr = Rp_ChainNextLink(linkPtr)) {
        notifyPtr = Rp_ChainGetValue(linkPtr);
        if ((notifyPtr->proc == proc) &&
            (notifyPtr->mask == mask) &&
            (notifyPtr->clientData == clientData)) {
            break;
        }
    }
    if (linkPtr == NULL) {
        notifyPtr = Rp_Malloc(sizeof (EventHandler));
        assert(notifyPtr);
        linkPtr = Rp_ChainAppend(clientPtr->events, notifyPtr);
    }
    if (proc == NULL) {
        Rp_ChainDeleteLink(clientPtr->events, linkPtr);
        Rp_Free(notifyPtr);
    } else {
        notifyPtr->proc = proc;
        notifyPtr->clientData = clientData;
        notifyPtr->mask = mask;
        notifyPtr->notifyPending = FALSE;
        // notifyPtr->interp = clientPtr->treeObject->interp;
    }
}

void
Rp_TreeDeleteEventHandler(
    TreeClient *clientPtr,
    unsigned int mask,
    Rp_TreeNotifyEventProc *proc,
    ClientData clientData)
{
    Rp_ChainLink *linkPtr;
    EventHandler *notifyPtr;

    for(linkPtr = Rp_ChainFirstLink(clientPtr->events);
        linkPtr != NULL; linkPtr = Rp_ChainNextLink(linkPtr)) {
        notifyPtr = Rp_ChainGetValue(linkPtr);
        if ((notifyPtr->proc == proc) && (notifyPtr->mask == mask) &&
            (notifyPtr->clientData == clientData)) {
            // FIXME: Cancel waiting calls
            /*
            if (notifyPtr->notifyPending) {
                Tcl_CancelIdleCall(NotifyIdleProc, notifyPtr);
            }
            */
            Rp_ChainDeleteLink(clientPtr->events, linkPtr);
            Rp_Free(notifyPtr);
            return;
        }
    }
}

// FIXME: rewrite this function to use a RpSimpleCharBuffer
/*
 *----------------------------------------------------------------------
 *
 * Rp_TreeNodePath --
 *
 *---------------------------------------------------------------------- 
 */
//char *
//Rp_TreeNodePath(Node *nodePtr, Tcl_DString *resultPtr)
//{
//    char **nameArr;     /* Used to stack the component names. */
//    char *staticSpace[64];
//    int nLevels;
//    register int i;
//
//    nLevels = nodePtr->depth;
//    if (nLevels > 64) {
//        nameArr = Rp_Malloc(nLevels * sizeof(char *));
//        assert(nameArr);
//    } else {
//        nameArr = staticSpace;
//    }
//    for (i = nLevels - 1; i >= 0; i--) {
//        /* Save the name of each ancestor in the name array.
//         * Note that we ignore the root. */
//        nameArr[i] = nodePtr->label;
//        nodePtr = nodePtr->parent;
//    }
//    // FIXME: add substitute for TCL DSTRING
//    /* Append each the names in the array. */
//    /*
//    Tcl_DStringInit(resultPtr);
//    for (i = 0; i < nLevels; i++) {
//        Tcl_DStringAppendElement(resultPtr, nameArr[i]);
//    }
//    if (nameArr != staticSpace) {
//        Rp_Free(nameArr);
//    }
//    return Tcl_DStringValue(resultPtr);
//    */
//
//    return NULL;
//}

//int
//Rp_TreeArrayValueExists(
//    TreeClient *clientPtr,
//    Node *nodePtr,
//    CONST char *arrayName,
//    CONST char *elemName)
//{
//    Rp_TreeKey key;
//    Rp_HashEntry *hPtr;
//    Rp_HashTable *tablePtr;
//    register Value *valuePtr;
//
//    key = Rp_TreeGetKey(arrayName);
//    valuePtr = GetTreeValue(nodePtr, key);
//    if (valuePtr == NULL) {
//        return FALSE;
//    }
//    // FIXME: fix object count
//    /*
//    if (Tcl_IsShared(valuePtr->objPtr)) {
//        Tcl_DecrRefCount(valuePtr->objPtr);
//        valuePtr->objPtr = Tcl_DuplicateObj(valuePtr->objPtr);
//        Tcl_IncrRefCount(valuePtr->objPtr);
//    }
//    */
//    if (Rp_GetArrayFromObj(valuePtr->objPtr, &tablePtr) != RP_OK) {
//        return FALSE;
//    }
//    hPtr = Rp_FindHashEntry(tablePtr, elemName);
//    return (hPtr != NULL);
//}
//
//int
//Rp_TreeGetArrayValue(
//    TreeClient *clientPtr,
//    Node *nodePtr,
//    CONST char *arrayName,
//    CONST char *elemName,
//    void **valueObjPtrPtr)
//{
//    Rp_TreeKey key;
//    Rp_HashEntry *hPtr;
//    Rp_HashTable *tablePtr;
//    register Value *valuePtr;
//
//    key = Rp_TreeGetKey(arrayName);
//    valuePtr = GetTreeValue(nodePtr, key);
//    if (valuePtr == NULL) {
//        return RP_ERROR;
//    }
//    // FIXME: fix object count
//    /*
//    if (Tcl_IsShared(valuePtr->objPtr)) {
//        Tcl_DecrRefCount(valuePtr->objPtr);
//        valuePtr->objPtr = Tcl_DuplicateObj(valuePtr->objPtr);
//        Tcl_IncrRefCount(valuePtr->objPtr);
//    }
//    */
//    if (Rp_GetArrayFromObj(valuePtr->objPtr, &tablePtr) != RP_OK) {
//        return RP_ERROR;
//    }
//    hPtr = Rp_FindHashEntry(tablePtr, elemName);
//    if (hPtr == NULL) {
//        fprintf(stderr, "can't find \"%s(%s)\"", arrayName, elemName);
//        return RP_ERROR;
//    }
//    *valueObjPtrPtr = (void *)Rp_GetHashValue(hPtr);
//
//    /* Reading any element of the array can cause a trace to fire. */
//    if (!(nodePtr->flags & TREE_TRACE_ACTIVE)) {
//        // CallTraces(interp, clientPtr, nodePtr->treeObject, nodePtr, key,
//        //            TREE_TRACE_READ);
//    }
//    return RP_OK;
//}
//
//int
//Rp_TreeSetArrayValue(
//    TreeClient *clientPtr,
//    Node *nodePtr,      /* Node to be updated. */
//    CONST char *arrayName,
//    CONST char *elemName,
//    Tcl_Obj *valueObjPtr)   /* New value of element. */
//{
//    Rp_TreeKey key;
//    Rp_HashEntry *hPtr;
//    Rp_HashTable *tablePtr;
//    register Value *valuePtr;
//    unsigned int flags;
//    int isNew;
//
//    assert(valueObjPtr != NULL);
//
//    /*
//     * Search for the array in the list of data fields.  If one
//     * doesn't exist, create it.
//     */
//    key = Rp_TreeGetKey(arrayName);
//    valuePtr = TreeCreateValue(nodePtr, key, &isNew);
//    if ((valuePtr->owner != NULL) && (valuePtr->owner != clientPtr)) {
//        fprintf(stderr, "can't set private field \"%s\"", key);
//        return RP_ERROR;
//    }
//    flags = TREE_TRACE_WRITE;
//    if (isNew) {
//        // FIXME: where is this function located?
//        //valuePtr->objPtr = Rp_NewArrayObj(0, (Tcl_Obj **)NULL);
//        // FIXME: increment ref counts
//        //Tcl_IncrRefCount(valuePtr->objPtr);
//        flags |= TREE_TRACE_CREATE;
//    } else if (Tcl_IsShared(valuePtr->objPtr)) {
//        // FIXME: ref counts and duplicateobj
//        //Tcl_DecrRefCount(valuePtr->objPtr);
//        //valuePtr->objPtr = Tcl_DuplicateObj(valuePtr->objPtr);
//        //Tcl_IncrRefCount(valuePtr->objPtr);
//    }
//    if (Rp_GetArrayFromObj(valuePtr->objPtr, &tablePtr) != RP_OK) {
//        return RP_ERROR;
//    }
//    // Tcl_InvalidateStringRep(valuePtr->objPtr);
//    hPtr = Rp_CreateHashEntry(tablePtr, elemName, &isNew);
//    assert(hPtr);
//
//    // FIXME: increment ref counts
////    Tcl_IncrRefCount(valueObjPtr);
////    if (!isNew) {
////        Tcl_Obj *oldValueObjPtr;
////
////        /* An element by the same name already exists. Decrement the
////         * reference count of the old value. */
////
////        oldValueObjPtr = (Tcl_Obj *)Rp_GetHashValue(hPtr);
////        if (oldValueObjPtr != NULL) {
////            Tcl_DecrRefCount(oldValueObjPtr);
////        }
////    }
//    Rp_SetHashValue(hPtr, valueObjPtr);
//
//    /*
//     * We don't handle traces on a per array element basis.  Setting
//     * any element can fire traces for the value.
//     */
//    if (!(nodePtr->flags & TREE_TRACE_ACTIVE)) {
////        CallTraces(interp, clientPtr, nodePtr->treeObject, nodePtr, 
////                valuePtr->key, flags);
//    }
//    return RP_OK;
//}
//
//int
//Rp_TreeUnsetArrayValue(
//    TreeClient *clientPtr,
//    Node *nodePtr,      /* Node to be updated. */
//    CONST char *arrayName,
//    CONST char *elemName)
//{
//    Rp_TreeKey key;     /* Name of field in node. */
//    Rp_HashEntry *hPtr;
//    Rp_HashTable *tablePtr;
//    Tcl_Obj *valueObjPtr;
//    Value *valuePtr;
//
//    key = Rp_TreeGetKey(arrayName);
//    valuePtr = TreeFindValue(nodePtr, key);
//    if (valuePtr == NULL) {
//        return RP_OK;
//    }
//    if ((valuePtr->owner != NULL) && (valuePtr->owner != clientPtr)) {
//        fprintf(stderr, "can't unset private field \"%s\"", key);
//        return RP_ERROR;
//    }
//    // FIXME: increment ref counts
//    /*
//    if (Tcl_IsShared(valuePtr->objPtr)) {
//        Tcl_DecrRefCount(valuePtr->objPtr);
//        valuePtr->objPtr = Tcl_DuplicateObj(valuePtr->objPtr);
//        Tcl_IncrRefCount(valuePtr->objPtr);
//    }
//    */
//    if (Rp_GetArrayFromObj(valuePtr->objPtr, &tablePtr) != RP_OK) {
//        return RP_ERROR;
//    }
//    hPtr = Rp_FindHashEntry(tablePtr, elemName);
//    if (hPtr == NULL) {
//        return RP_OK;      /* Element doesn't exist, Ok. */
//    }
//    // FIXME: increment ref counts
//    //valueObjPtr = (Tcl_Obj *)Rp_GetHashValue(hPtr);
//    //Tcl_DecrRefCount(valueObjPtr);
//    Rp_DeleteHashEntry(tablePtr, hPtr);
//
//    /*
//     * Un-setting any element in the array can cause the trace on the value
//     * to fire.
//     */
//    if (!(nodePtr->flags & TREE_TRACE_ACTIVE)) {
////        CallTraces(interp, clientPtr, nodePtr->treeObject, nodePtr,
////                valuePtr->key, TREE_TRACE_WRITE);
//    }
//    return RP_OK;
//}
//
//int
//Rp_TreeArrayNames(
//    TreeClient *clientPtr,
//    Node *nodePtr,
//    CONST char *arrayName,
//    Tcl_Obj *listObjPtr)
//{
//    Rp_HashEntry *hPtr;
//    Rp_HashSearch cursor;
//    Rp_HashTable *tablePtr;
//    Tcl_Obj *objPtr;
//    Value *valuePtr;
//    char *key;
//
//    key = Rp_TreeGetKey(arrayName);
//    valuePtr = GetTreeValue(nodePtr, key);
//    if (valuePtr == NULL) {
//        return TCL_ERROR;
//    }
//    // FIXME: increment ref counts
//    /*
//    if (Tcl_IsShared(valuePtr->objPtr)) {
//        Tcl_DecrRefCount(valuePtr->objPtr);
//        valuePtr->objPtr = Tcl_DuplicateObj(valuePtr->objPtr);
//        Tcl_IncrRefCount(valuePtr->objPtr);
//    }
//    */
//    if (Rp_GetArrayFromObj(valuePtr->objPtr, &tablePtr) != RP_OK) {
//        return RP_ERROR;
//    }
//    tablePtr = (Rp_HashTable *)valuePtr->objPtr;
//    for (hPtr = Rp_FirstHashEntry(tablePtr, &cursor); hPtr != NULL;
//         hPtr = Rp_NextHashEntry(&cursor)) {
//    // FIXME: place names in array
//    //    objPtr = Tcl_NewStringObj(Rp_GetHashKey(tablePtr, hPtr), -1);
//    //    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
//    }
//    return RP_OK;
//}
//
/*
 *----------------------------------------------------------------------
 *
 * Rp_TreeShareTagTable --
 *
 *---------------------------------------------------------------------- 
 */
int
Rp_TreeShareTagTable(
    TreeClient *sourcePtr,
    TreeClient *targetPtr)
{
    sourcePtr->tagTablePtr->refCount++;
    if (targetPtr->tagTablePtr != NULL) {
        ReleaseTagTable(targetPtr->tagTablePtr);
    }
    targetPtr->tagTablePtr = sourcePtr->tagTablePtr;
    return RP_OK;
}

int
Rp_TreeTagTableIsShared(TreeClient *clientPtr)
{
    return (clientPtr->tagTablePtr->refCount > 1);
}

void
Rp_TreeClearTags(TreeClient *clientPtr, Rp_TreeNode node)
{
    Rp_HashEntry *hPtr, *h2Ptr;
    Rp_HashSearch cursor;

    for (hPtr = Rp_FirstHashEntry(&clientPtr->tagTablePtr->tagTable, &cursor);
        hPtr != NULL; hPtr = Rp_NextHashEntry(&cursor)) {
        Rp_TreeTagEntry *tPtr;

        tPtr = Rp_GetHashValue(hPtr);
        h2Ptr = Rp_FindHashEntry(&tPtr->nodeTable, (char *)node);
        if (h2Ptr != NULL) {
            Rp_DeleteHashEntry(&tPtr->nodeTable, h2Ptr);
        }
    }
}

int
Rp_TreeHasTag(
    TreeClient *clientPtr,
    Rp_TreeNode node,
    CONST char *tagName)
{
    Rp_HashEntry *hPtr;
    Rp_TreeTagEntry *tPtr;

    if (strcmp(tagName, "all") == 0) {
        return TRUE;
    }
    if ((strcmp(tagName, "root") == 0) &&
        (node == Rp_TreeRootNode(clientPtr))) {
        return TRUE;
    }
    hPtr = Rp_FindHashEntry(&clientPtr->tagTablePtr->tagTable, tagName);
    if (hPtr == NULL) {
        return FALSE;
    }
    tPtr = Rp_GetHashValue(hPtr);
    hPtr = Rp_FindHashEntry(&tPtr->nodeTable, (char *)node);
    if (hPtr == NULL) {
        return FALSE;
    }
    return TRUE;
}

void
Rp_TreeAddTag(
    TreeClient *clientPtr,
    Rp_TreeNode node,
    CONST char *tagName)
{
    int isNew;
    Rp_HashEntry *hPtr;
    Rp_HashTable *tablePtr;
    Rp_TreeTagEntry *tPtr;

    if ((strcmp(tagName, "all") == 0) || (strcmp(tagName, "root") == 0)) {
        return;
    }
    tablePtr = &clientPtr->tagTablePtr->tagTable;
    hPtr = Rp_CreateHashEntry(tablePtr, tagName, &isNew);
    assert(hPtr);
    if (isNew) {

        tPtr = Rp_Malloc(sizeof(Rp_TreeTagEntry));
        Rp_InitHashTable(&tPtr->nodeTable, RP_ONE_WORD_KEYS);
        Rp_SetHashValue(hPtr, tPtr);
        tPtr->hashPtr = hPtr;
        tPtr->tagName = Rp_GetHashKey(tablePtr, hPtr);
    } else {
        tPtr = Rp_GetHashValue(hPtr);
    }
    hPtr = Rp_CreateHashEntry(&tPtr->nodeTable, (char *)node, &isNew);
    assert(hPtr);
    if (isNew) {
        Rp_SetHashValue(hPtr, node);
    }
}

void
Rp_TreeForgetTag(TreeClient *clientPtr, CONST char *tagName)
{
    if ((strcmp(tagName, "all") != 0) && (strcmp(tagName, "root") != 0)) {
        Rp_HashEntry *hPtr;

        hPtr = Rp_FindHashEntry(&clientPtr->tagTablePtr->tagTable, tagName);
        if (hPtr != NULL) {
            Rp_TreeTagEntry *tPtr;

            Rp_DeleteHashEntry(&clientPtr->tagTablePtr->tagTable, hPtr);
            tPtr = Rp_GetHashValue(hPtr);
            Rp_DeleteHashTable(&tPtr->nodeTable);
            Rp_Free(tPtr);
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Rp_TreeTagHashTable --
 *
 *---------------------------------------------------------------------- 
 */
Rp_HashTable *
Rp_TreeTagHashTable(TreeClient *clientPtr, CONST char *tagName)
{
    Rp_HashEntry *hPtr;

    hPtr = Rp_FindHashEntry(&clientPtr->tagTablePtr->tagTable, tagName);
    if (hPtr != NULL) {
        Rp_TreeTagEntry *tPtr;

        tPtr = Rp_GetHashValue(hPtr);
        return &tPtr->nodeTable;
    }
    return NULL;
}

Rp_HashEntry *
Rp_TreeFirstTag(TreeClient *clientPtr, Rp_HashSearch *cursorPtr)
{
    return Rp_FirstHashEntry(&clientPtr->tagTablePtr->tagTable, cursorPtr);
}

#if (SIZEOF_VOID_P == 8)
/*
 *----------------------------------------------------------------------
 *
 * HashOneWord --
 *
 *  Compute a one-word hash value of a 64-bit word, which then can
 *  be used to generate a hash index.
 *
 *  From Knuth, it's a multiplicative hash.  Multiplies an unsigned
 *  64-bit value with the golden ratio (sqrt(5) - 1) / 2.  The
 *  downshift value is 64 - n, when n is the log2 of the size of
 *  the hash table.
 *
 * Results:
 *  The return value is a one-word summary of the information in
 *  64 bit word.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */
static Rp_Hash
HashOneWord(
    uint64_t mask,
    unsigned int downshift,
    CONST void *key)
{
    uint64_t a0, a1;
    uint64_t y0, y1;
    uint64_t y2, y3;
    uint64_t p1, p2;
    uint64_t result;

    /* Compute key * GOLDEN_RATIO in 128-bit arithmetic */
    a0 = (uint64_t)key & 0x00000000FFFFFFFF;
    a1 = (uint64_t)key >> 32;

    y0 = a0 * 0x000000007f4a7c13;
    y1 = a0 * 0x000000009e3779b9;
    y2 = a1 * 0x000000007f4a7c13;
    y3 = a1 * 0x000000009e3779b9;
    y1 += y0 >> 32;     /* Can't carry */
    y1 += y2;           /* Might carry */
    if (y1 < y2) {
        y3 += (1LL << 32);  /* Propagate */
    }

    /* 128-bit product: p1 = loword, p2 = hiword */
    p1 = ((y1 & 0x00000000FFFFFFFF) << 32) + (y0 & 0x00000000FFFFFFFF);
    p2 = y3 + (y1 >> 32);

    /* Left shift the value downward by the size of the table */
    if (downshift > 0) {
        if (downshift < 64) {
            result = ((p2 << (64 - downshift)) | (p1 >> (downshift & 63)));
        } else {
            result = p2 >> (downshift & 63);
        }
    } else { 
        result = p1;
    }
    /* Finally mask off the high bits */
    return (Rp_Hash)(result & mask);
}

#endif /* SIZEOF_VOID_P == 8 */

/*
 *----------------------------------------------------------------------
 *
 * RebuildTable --
 *
 *  This procedure is invoked when the ratio of entries to hash
 *  buckets becomes too large.  It creates a new table with a
 *  larger bucket array and moves all of the entries into the
 *  new table.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Memory gets reallocated and entries get re-hashed to new
 *  buckets.
 *
 *----------------------------------------------------------------------
 */
static void
RebuildTable(Node *nodePtr)     /* Table to enlarge. */
{
    Value **newBucketPtr, **oldBuckets;
    register Value **bucketPtr, **endPtr;
    register Value *valuePtr, *nextPtr;
    unsigned int downshift;
    unsigned long mask;
    Value **buckets;
    size_t nBuckets;

    oldBuckets = (Value **)nodePtr->values;
    nBuckets = (1 << nodePtr->logSize);
    endPtr = oldBuckets + nBuckets;

    /*
     * Allocate and initialize the new bucket array, and set up
     * hashing constants for new array size.
     */
    nodePtr->logSize += 2;
    nBuckets = (1 << nodePtr->logSize);
    buckets = Rp_Calloc(nBuckets, sizeof(Value *));

    /*
     * Move all of the existing entries into the new bucket array,
     * based on their hash values.
     */
    mask = nBuckets - 1;
    downshift = DOWNSHIFT_START - nodePtr->logSize;
    for (bucketPtr = oldBuckets; bucketPtr < endPtr; bucketPtr++) {
        for (valuePtr = *bucketPtr; valuePtr != NULL; valuePtr = nextPtr) {
            nextPtr = valuePtr->next;
            newBucketPtr = buckets + RANDOM_INDEX(valuePtr->key);
            valuePtr->next = *newBucketPtr;
            *newBucketPtr = valuePtr;
        }
    }
    nodePtr->values = (Value *)buckets;
    Rp_Free(oldBuckets);
}

static void
ConvertValues(Node *nodePtr)
{
    unsigned int nBuckets;
    Value **buckets;
    unsigned int mask;
    int downshift;
    Value *valuePtr, *nextPtr, **bucketPtr;

    /*
     * Convert list of values into a hash table.
     */
    nodePtr->logSize = START_LOGSIZE;
    nBuckets = 1 << nodePtr->logSize;
    buckets = Rp_Calloc(nBuckets, sizeof(Value *));
    mask = nBuckets - 1;
    downshift = DOWNSHIFT_START - nodePtr->logSize;
    for (valuePtr = nodePtr->values; valuePtr != NULL; valuePtr = nextPtr) {
        nextPtr = valuePtr->next;
        bucketPtr = buckets + RANDOM_INDEX(valuePtr->key);
        valuePtr->next = *bucketPtr;
        *bucketPtr = valuePtr;
    }
    nodePtr->values = (Value *)buckets;
}

/*
 *----------------------------------------------------------------------
 *
 * TreeDeleteValue --
 *
 *  Remove a single entry from a hash table.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  The entry given by entryPtr is deleted from its table and
 *  should never again be used by the caller.  It is up to the
 *  caller to free the clientData field of the entry, if that
 *  is relevant.
 *
 *----------------------------------------------------------------------
 */
static int
TreeDeleteValue(Node *nodePtr, Rp_TreeValue value)
{
    Value *valuePtr = value;
    register Value *prevPtr;

    if (nodePtr->logSize > 0) {
        Value **bucketPtr;
        unsigned int downshift;
        unsigned long mask;

        mask = (1 << nodePtr->logSize) - 1;
        downshift = DOWNSHIFT_START - nodePtr->logSize;

        bucketPtr = (Value **)nodePtr->values + RANDOM_INDEX(valuePtr->key);
        if (*bucketPtr == valuePtr) {
            *bucketPtr = valuePtr->next;
        } else {
            for (prevPtr = *bucketPtr; /*empty*/; prevPtr = prevPtr->next) {
                if (prevPtr == NULL) {
                    return RP_ERROR; /* Can't find value in hash bucket. */
                }
                if (prevPtr->next == valuePtr) {
                    prevPtr->next = valuePtr->next;
                    break;
                }
            }
        }
    } else {
        prevPtr = NULL;
        for (valuePtr = nodePtr->values; valuePtr != NULL;
             valuePtr = valuePtr->next) {
            if (valuePtr == value) {
                break;
            }
            prevPtr = valuePtr;
        }
        if (valuePtr == NULL) {
            return RP_ERROR;   /* Can't find value in list. */
        }
        if (prevPtr == NULL) {
            nodePtr->values = valuePtr->next;
        } else {
            prevPtr->next = valuePtr->next;
        }
    }
    nodePtr->nValues--;
    FreeValue(nodePtr, valuePtr);
    return RP_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TreeDestroyValues --
 *
 *  Free up everything associated with a hash table except for
 *  the record for the table itself.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  The hash table is no longer useable.
 *
 *----------------------------------------------------------------------
 */
static void
TreeDestroyValues(Node *nodePtr)
{
    register Value *valuePtr;
    Value *nextPtr;

    /*
     * Free up all the entries in the table.
     */
    if (nodePtr->values != NULL) {
        return;
    }
    if (nodePtr->logSize > 0) {
        Value **buckets;
        int nBuckets;
        int i;

        buckets = (Value **)nodePtr->values;
        nBuckets = (1 << nodePtr->logSize);
        for (i = 0; i < nBuckets; i++) {
            for (valuePtr = buckets[i]; valuePtr != NULL; valuePtr = nextPtr) {
                nextPtr = valuePtr->next;
                FreeValue(nodePtr, valuePtr);
            }
        }
        Rp_Free(buckets);
    } else {
        for (valuePtr = nodePtr->values; valuePtr != NULL; valuePtr = nextPtr) {
            nextPtr = valuePtr->next;
            FreeValue(nodePtr, valuePtr);
        }
    }
    nodePtr->values = NULL;
    nodePtr->nValues = 0;
    nodePtr->logSize = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TreeFirstValue --
 *
 *  Locate the first entry in a hash table and set up a record
 *  that can be used to step through all the remaining entries
 *  of the table.
 *
 * Results:
 *  The return value is a pointer to the first value in tablePtr,
 *  or NULL if tablePtr has no entries in it.  The memory at
 *  *searchPtr is initialized so that subsequent calls to
 *  Rp_TreeNextValue will return all of the values in the table,
 *  one at a time.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */
static Value *
TreeFirstValue(
    Node *nodePtr,
    Rp_TreeKeySearch *searchPtr)    /* Place to store information about
                                     * progress through the table. */
{
    searchPtr->node = nodePtr;
    searchPtr->nextIndex = 0;
    if (nodePtr->logSize > 0) {
        searchPtr->nextValue = NULL;
    } else {
        searchPtr->nextValue = nodePtr->values;
    }
    return TreeNextValue(searchPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TreeNextValue --
 *
 *  Once a hash table enumeration has been initiated by calling
 *  Rp_TreeFirstValue, this procedure may be called to return
 *  successive elements of the table.
 *
 * Results:
 *  The return value is the next entry in the hash table being
 *  enumerated, or NULL if the end of the table is reached.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */
static Value *
TreeNextValue(
    Rp_TreeKeySearch *searchPtr)  /* Place to store information about
                                   * progress through the table.  Must
                                   * have been initialized by calling
                                   * Rp_TreeFirstValue. */
{
    Value *valuePtr;

    if (searchPtr->node->logSize > 0) {
        size_t nBuckets;
        Value **buckets;

        nBuckets = (1 << searchPtr->node->logSize);
        buckets = (Value **)searchPtr->node->values;
        while (searchPtr->nextValue == NULL) {
            if (searchPtr->nextIndex >= nBuckets) {
                return NULL;
            }
            searchPtr->nextValue = buckets[searchPtr->nextIndex];
            searchPtr->nextIndex++;
        }
    }
    valuePtr = searchPtr->nextValue;
    if (valuePtr != NULL) {
        searchPtr->nextValue = valuePtr->next;
    }
    return valuePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TreeFindValue --
 *
 *  Given a hash table with one-word keys, and a one-word key, find
 *  the entry with a matching key.
 *
 * Results:
 *  The return value is a token for the matching entry in the
 *  hash table, or NULL if there was no matching entry.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */
static Value *
TreeFindValue(
    Node *nodePtr,
    Rp_TreeKey key)     /* Key to use to find matching entry. */
{
    register Value *valuePtr;
    Value *bucket;

    if (nodePtr->logSize > 0) {
        unsigned int downshift;
        unsigned long mask;

        mask = (1 << nodePtr->logSize) - 1;
        downshift = DOWNSHIFT_START - nodePtr->logSize;
        bucket = ((Value **)(nodePtr->values))[RANDOM_INDEX((void *)key)];
    } else {
        bucket = nodePtr->values; /* Single chain list. */
    }
    /*
     * Search all of the entries in the appropriate bucket.
     */
    for (valuePtr = bucket; valuePtr != NULL; valuePtr = valuePtr->next) {
        if (valuePtr->key == key) {
            return valuePtr;
        }
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Rp_TreeCreateValue --
 *
 *  Find the value with a matching key.  If there is no matching 
 *  value, then create a new one.
 *
 * Results:
 *  The return value is a pointer to the matching value.  If this
 *  is a newly-created value, then *newPtr will be set to a non-zero
 *  value;  otherwise *newPtr will be set to 0.
 *
 * Side effects:
 *  A new value may be added to the hash table.
 *
 *----------------------------------------------------------------------
 */
static Value *
TreeCreateValue(
    Node *nodePtr,
    Rp_TreeKey key,     /* Key to use to find or create matching
                         * entry. */
    int *newPtr)        /* Store info here telling whether a new
                         * entry was created. */
{
    register Value *valuePtr;

    /*
     * Check if there as so many values that storage should be
     * converted from a hash table instead of a list.
     */
    if ((nodePtr->logSize == 0) && (nodePtr->nValues > MAX_LIST_VALUES)) {
        ConvertValues(nodePtr);
    }
    if (nodePtr->logSize > 0) {
        Value **bucketPtr;
        size_t nBuckets;
        unsigned int downshift;
        unsigned long mask;

        nBuckets = (1 << nodePtr->logSize);
        mask = nBuckets - 1;
        downshift = DOWNSHIFT_START - nodePtr->logSize;
        bucketPtr = (Value **)nodePtr->values + RANDOM_INDEX((void *)key);

        *newPtr = FALSE;
        for (valuePtr = *bucketPtr; valuePtr != NULL;
             valuePtr = valuePtr->next) {
            if (valuePtr->key == key) {
                return valuePtr;
            }
        }

        /* Value not found. Add a new value to the bucket. */
        *newPtr = TRUE;
        valuePtr = Rp_PoolAllocItem(nodePtr->treeObject->valuePool, 
                sizeof(Value));
        valuePtr->key = key;
        valuePtr->owner = NULL;
        valuePtr->next = *bucketPtr;
        valuePtr->objPtr = NULL;
        *bucketPtr = valuePtr;
        nodePtr->nValues++;

        /*
         * If the table has exceeded a decent size, rebuild it with many
         * more buckets.
         */
        if ((unsigned int)nodePtr->nValues >= (nBuckets * 3)) {
            RebuildTable(nodePtr);
        }
        } else {
        Value *prevPtr;

        prevPtr = NULL;
        *newPtr = FALSE;
        for (valuePtr = nodePtr->values; valuePtr != NULL;
             valuePtr = valuePtr->next) {
            if (valuePtr->key == key) {
                return valuePtr;
            }
            prevPtr = valuePtr;
        }
        /* Value not found. Add a new value to the list. */
        *newPtr = TRUE;
        valuePtr = Rp_PoolAllocItem(nodePtr->treeObject->valuePool,
                sizeof(Value));
        valuePtr->key = key;
        valuePtr->owner = NULL;
        valuePtr->next = NULL;
        valuePtr->objPtr = NULL;
        if (prevPtr == NULL) {
            nodePtr->values = valuePtr;
        } else {
            prevPtr->next = valuePtr;
        }
        nodePtr->nValues++;
    }
    return valuePtr;
}

#endif /* NO_TREE */
