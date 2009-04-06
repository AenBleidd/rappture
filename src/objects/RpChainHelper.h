#ifndef _RP_CHAIN_HELPER_H
#define _RP_CHAIN_HELPER_H 1

#include "RpInt.h"
#include "RpChain.h"

#ifdef __cplusplus
extern "C" {
#endif


// int chain2hash(Rp_Chain *c, Rp_HashTable *h);

Rp_Chain * Rp_ChainJoin     ( Rp_Chain *c1,
                              Rp_Chain *c2);

Rp_Chain * Rp_ChainConcat   ( Rp_Chain *c1,
                              Rp_Chain *c2);

int Rp_ChainCopy            ( Rp_Chain *c1,
                              Rp_Chain *c2,
                              int (*cpyFxn)(void **to,void *from));

#ifdef __cplusplus
}
#endif

#endif // _RP_CHAIN_HELPER_H
