#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "RpHashHelper.h"

int
Rp_HashPrint(
    Rp_HashTable *h)
{
    Rp_HashEntry *hEntry = NULL;
    Rp_HashSearch hSearch;
    char *k = NULL;
    char *v = NULL;

    printf("hash table start\n");
    hEntry = Rp_FirstHashEntry(h,&hSearch);
    while (hEntry != NULL) {
        k = (char *) Rp_GetHashKey(h,hEntry);
        v = (char *) Rp_GetHashValue(hEntry);
        printf("hentry = :%s->%s:\n",k,v);
        hEntry = Rp_NextHashEntry(&hSearch);
    }
    printf("hash table end\n");
    return 0;
}

int
Rp_HashAddNode(
    Rp_HashTable *h,
    const char* key,
    const void *n)
{
    int isNew = 0;
    Rp_HashEntry *hEntry = NULL;

    hEntry = Rp_CreateHashEntry(h,key,&isNew);
    if (isNew == 0) {
        // hash entry is not new
        // warn user: two vals with the same name
        // warn("hash entry is not new\n");
    }
    Rp_SetHashValue(hEntry,(ClientData)n);
    return isNew;
}

/*
 *  hRslt = h1 U h2
 */

int
Rp_HashUnion(
    Rp_HashTable *hRslt,
    Rp_HashTable *h1,
    Rp_HashTable *h2,
    int (*cpyFxn)(void **to, void *from),
    int (*cmpFxn) (void *he1, void *he2))
{
    Rp_HashSearch hSearch;
    Rp_HashEntry *hEntry = NULL;
    void *origKey = NULL;
    void *origNode = NULL;
    void *newNode = NULL;
    void *searchRslt = NULL;
    int nodesAddedCnt = 0;

    Rp_HashCopy(hRslt,h1,cpyFxn);

    hEntry = Rp_FirstHashEntry(h2,&hSearch);
    while (hEntry != NULL) {
        origKey = Rp_GetHashKey(h2,hEntry);
        origNode = Rp_GetHashValue(hEntry);
        searchRslt = Rp_HashSearchNode(hRslt,origKey);
        if (searchRslt == NULL) {
            // node not in hRslt, add it
            cpyFxn(&newNode,origNode);
            Rp_HashAddNode(hRslt,origKey,newNode);
            nodesAddedCnt++;
        }
        hEntry = Rp_NextHashEntry(&hSearch);
    }
    return nodesAddedCnt;
}

/*
 *  hRslt = h1 - h2
 */

int
Rp_HashSubrtact(
    Rp_HashTable *hRslt,
    Rp_HashTable *h1,
    Rp_HashTable *h2,
    int (*cpyFxn)(void **to, void *from),
    int (*cmpFxn)(void *he1, void *he2))
{
    Rp_HashSearch hSearch;
    Rp_HashEntry *hEntry = NULL;
    void *origKey = NULL;
    void *origNode = NULL;
    void *newNode = NULL;
    int nodesAddedCnt = 0;

    hEntry = Rp_FirstHashEntry(h1,&hSearch);
    while (hEntry != NULL) {
        origKey = Rp_GetHashKey(h1,hEntry);
        origNode = Rp_GetHashValue(hEntry);
        if (Rp_HashSearchNode(h2,origKey) == NULL) {
            // node not in h2, add it to hRslt
            cpyFxn(&newNode,origNode);
            Rp_HashAddNode(hRslt,origKey,newNode);
            nodesAddedCnt++;
        }
        hEntry = Rp_NextHashEntry(&hSearch);
    }
    return nodesAddedCnt;
}

int
Rp_HashCompare(
    Rp_HashTable *h1,
    Rp_HashTable *h2,
    int (*cmpFxn)(void *n1,void *n2))
{
    Rp_HashSearch hSearch;
    Rp_HashEntry *h1Entry = NULL;
    Rp_HashEntry *h2Entry = NULL;
    void *h1Node = NULL;
    void *h2Key = NULL;
    void *h2Node = NULL;
    int compareResult = 0;

    // check the pointer values
    if ( (h1 == NULL) && (h2 != NULL) ) {
        return -1;
    } else if ( (h1 != NULL) && (h2 == NULL) ) {
        return 1;
    } else if (h1 == h2) {
        return 0;
    }

    // check hash table sizes
    if (h1->numEntries < h2->numEntries) {
        return -1;
    } else if (h1 ->numEntries > h2->numEntries) {
        return 1;
    }

    // equal number of entries, have to check each entry.

    h2Entry = Rp_FirstHashEntry(h2,&hSearch);
    while (h2Entry != NULL) {
        h2Key = Rp_GetHashKey(h2,h2Entry);
        h2Node = Rp_GetHashValue(h2Entry);
        h1Entry = Rp_FindHashEntry(h1,h2Key);
        if (h1Entry == NULL) {
            compareResult = -1;
            break;
        } else {
            h1Node = Rp_GetHashValue(h1Entry);
            compareResult = cmpFxn(h1Node,h2Node);
            if (compareResult != 0) {
                break;
            }
        }
        h2Entry = Rp_NextHashEntry(&hSearch);
    }
    return compareResult;
}


/*
 *  forall(i) h1(i) = h2(i) : i E (nodes(h2))
 *  elements of h2 are copied to h1 regardless of if h1
 *  already has elements in it.
 */

int
Rp_HashCopy(
    Rp_HashTable *h1,
    Rp_HashTable *h2,
    int (*cpyFxn)(void **to,void *from))
{
    Rp_HashSearch hSearch;
    Rp_HashEntry *hEntry = NULL;
    void *origKey = NULL;
    void *origNode = NULL;
    void *newNode = NULL;

    hEntry = Rp_FirstHashEntry(h2,&hSearch);
    while (hEntry != NULL) {
        origKey = Rp_GetHashKey(h2,hEntry);
        origNode = Rp_GetHashValue(hEntry);
        if (cpyFxn(&newNode,origNode) != 0) {
            // error while copying
        }
        Rp_HashAddNode(h1,origKey,newNode);
        hEntry = Rp_NextHashEntry(&hSearch);
    }
    return 0;
}

/*
 * this function will not work for you if you put
 * entries in the hash table with null data value
 */

void *
Rp_HashSearchNode(
    Rp_HashTable *h,
    const char *key)
{
    Rp_HashEntry *hEntry = NULL;
    void *n = NULL;
    hEntry = Rp_FindHashEntry(h,key);
    if (hEntry != NULL) {
        n = Rp_GetHashValue(hEntry);
    }
    return n;
}

void *
Rp_HashRemoveNode(
    Rp_HashTable *h,
    const char *key)
{
    Rp_HashEntry *hEntry = NULL;
    void *n = NULL;
    hEntry = Rp_FindHashEntry(h,key);
    if (hEntry != NULL) {
        n = Rp_GetHashValue(hEntry);
        Rp_DeleteHashEntry(h,hEntry);
    }
    return n;
}

int charCpyFxn(
    void **to,
    void *from)
{
    size_t len = 0;

    len = strlen((char*)from);
    *to = (void*) malloc(len*sizeof(char));
    strcpy((char*)(*to),(char*)from);
    return 0;
}

int charCmpFxn(
    void *to,
    void *from)
{
    return strcmp(to,from);
}

