#ifndef RP_POOL_H
#define RP_POOL_H

typedef struct Rp_PoolChainStruct {
   struct Rp_PoolChainStruct *nextPtr;
} Rp_PoolChain;

#define RP_STRING_ITEMS         0
#define RP_FIXED_SIZE_ITEMS     1
#define RP_VARIABLE_SIZE_ITEMS  2

typedef struct Rp_PoolStruct *Rp_Pool;

typedef void *(Rp_PoolAllocProc) _ANSI_ARGS_((Rp_Pool pool, size_t size));
typedef void (Rp_PoolFreeProc) _ANSI_ARGS_((Rp_Pool pool, void *item));

struct Rp_PoolStruct {
    Rp_PoolChain *headPtr;  /* Chain of malloc'ed chunks. */
    Rp_PoolChain *freePtr;  /* List of deleted items. This is only used
                             * for fixed size items. */
    size_t poolSize;        /* Log2 of # of items in the current block. */
    size_t itemSize;        /* Size of an item. */
    size_t bytesLeft;       /* # of bytes left in the current chunk. */
    size_t waste;

    Rp_PoolAllocProc *allocProc;
    Rp_PoolFreeProc *freeProc;
};

extern Rp_Pool Rp_PoolCreate _ANSI_ARGS_((int type));
extern void Rp_PoolDestroy _ANSI_ARGS_((Rp_Pool pool));

#define Rp_PoolAllocItem(poolPtr, n) (*((poolPtr)->allocProc))(poolPtr, n)
#define Rp_PoolFreeItem(poolPtr, item) (*((poolPtr)->freeProc))(poolPtr, item)

#endif /* RP_POOL_H */
