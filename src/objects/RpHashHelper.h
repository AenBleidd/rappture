#ifndef _RP_HASH_HELPER_H
#define _RP_HASH_HELPER_H 1

#include "RpInt.h"
#include "RpHash.h"

int Rp_HashPrint        ( Rp_HashTable *h);

int Rp_HashAddNode      ( Rp_HashTable *h, const char *key, const void *n);

int Rp_HashUnion        ( Rp_HashTable *hRslt,
                          Rp_HashTable *h1,
                          Rp_HashTable *h2,
                          int (*cpyFxn)(void **to, void *from),
                          int (*cmpFxn) (void *he1, void *he2));

int Rp_HashSubrtact     ( Rp_HashTable *hRslt,
                          Rp_HashTable *h1,
                          Rp_HashTable *h2,
                          int (*cpyFxn)(void **to, void *from),
                          int (*cmpFxn)(void *he1, void *he2));

int Rp_HashCompare      ( Rp_HashTable *h1,
                          Rp_HashTable *h2,
                          int (*cmpFxn)(void *n1,void *n2));

int Rp_HashCopy         ( Rp_HashTable *h1,
                          Rp_HashTable *h2,
                          int (*cpyFxn)(void **to,void *from));

void *Rp_HashSearchNode ( Rp_HashTable *h, const char *key);

void *Rp_HashRemoveNode ( Rp_HashTable *h, const char *key);

int charCpyFxn(void **to, void *from);
int charCmpFxn( void *to, void *from);

#endif // _RP_HASH_HELPER_H
