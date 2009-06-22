#include <string.h>
#include "RpChainHelper.h"

#ifdef __cplusplus
extern "C" {
#endif

Rp_Chain *
Rp_ChainJoin (
    Rp_Chain *c1,
    Rp_Chain *c2)
{
    Rp_ChainLink *l = NULL;

    if (c1 == NULL) {
        return c2;
    }

    if (c2 == NULL) {
        return c1;
    }

    l = Rp_ChainLastLink(c2);
    while (l != NULL) {
        Rp_ChainUnlinkLink(c2,l);
        Rp_ChainPrependLink(c1,l);
        l = Rp_ChainLastLink(c2);
    }

    if (Rp_ChainGetLength(c2) != 0) {
    // FIXME: add error message
    }

    Rp_ChainDestroy(c2);

    return c1;
}

Rp_Chain *
Rp_ChainConcat (
    Rp_Chain *c1,
    Rp_Chain *c2)
{
    Rp_ChainLink *l = NULL;

    if (c1 == NULL) {
        return c2;
    }

    if (c2 == NULL) {
        return c1;
    }

    l = Rp_ChainFirstLink(c2);
    while (l != NULL) {
        Rp_ChainUnlinkLink(c2,l);
        Rp_ChainAppendLink(c1,l);
        l = Rp_ChainFirstLink(c2);
    }

    return c1;
}

int
Rp_ChainCopy (
    Rp_Chain *c1,
    Rp_Chain *c2,
    int (*cpyFxn)(void **to,void *from))
{
    void *origVal = NULL;
    void *copyVal = NULL;
    Rp_ChainLink *lorig = NULL;

    if ( ( (c1 == NULL) && (c2 == NULL) )
        || (cpyFxn == NULL) ) {
        // raise error, improper function call
        return -1;
    }

    if (c1 == NULL) {
        // nothing to copy to
        return 0;
    }

    if (c2 == NULL) {
        // nothing to copy from
        return 0;
    }

    lorig = Rp_ChainFirstLink(c2);
    while (lorig != NULL) {
        origVal = (void*) Rp_ChainGetValue(lorig);
        if (cpyFxn(&copyVal,origVal) != 0) {
            // error while copying
            return -1;
        }
        Rp_ChainAppend(c1,copyVal);
        lorig = Rp_ChainNextLink(lorig);
    }

    return 0;
}

int
Rp_ChainCharCpyFxn (
    void **to,
    void *from)
{
    size_t len = 0;

    len = strlen((char*)from);
    *to = (void*) malloc(len*sizeof(char)+1);
    strncpy((char*)(*to),(char*)from,len+1);
    return 0;
}

int
Rp_ChainCharCmpFxn (
    void *to,
    void *from)
{
    return strcmp(to,from);
}

#ifdef __cplusplus
}
#endif

