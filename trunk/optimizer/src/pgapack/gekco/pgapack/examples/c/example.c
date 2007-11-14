#include <pgapack.h>

/* EHW: Testing the addition of Initialization functions */

void Init(PGAContext *ctx);

void Init(PGAContext *ctx)
{
  printf(" Initializaiton User Context Function \n");


}

void Kill(PGAContext *ctx);

void Kill(PGAContext *ctx)
{

  printf(" Kill UserContext Function \n");
}


void EOG(PGAContext *ctx);

void EOG(PGAContext *ctx)
{

  int gen=PGAGetGAIterValue(ctx);
  printf("ENDOFGEN %d \n",gen);

}


#define LEN 5

double evaluate (PGAContext *ctx, int p, int pop);
int myMutation  (PGAContext *, int, int, double);

int main( int argc, char **argv )
{
     PGAContext *ctx; 
     int i, lower[LEN], upper[LEN];

     for (i=0; i<LEN; i++) {
	 lower[i] = 1;
	 upper[i] = LEN;
     }
     ctx = PGACreate (&argc, argv, PGA_DATATYPE_INTEGER, LEN, PGA_MAXIMIZE);
     PGASetUserFunction (ctx, PGA_USERFUNCTION_MUTATION, (void *)myMutation);

     PGASetUserFunction (ctx, PGA_USERFUNCTION_ENDOFGEN, (void *)EOG);

     PGASetUserFunction (ctx, PGA_USERFUNCTION_INITUSERCTX, (void *)Init);
     PGASetUserFunction (ctx, PGA_USERFUNCTION_KILLUSERCTX, (void *)Kill);


     PGASetMaxGAIterValue(ctx,30);

     PGASetIntegerInitRange(ctx, lower, upper);
     PGASetUp              (ctx);
     PGARun                (ctx, evaluate);
     PGADestroy            (ctx);
     return(0);
}
int myMutation(PGAContext *ctx, int p, int pop, double pm)
{
    int stringlen, i, k, count = 0;
    stringlen = PGAGetStringLength(ctx);
    for (i = 0; i < stringlen; i++)
    if (PGARandomFlip(ctx, pm)) {
        k = PGARandomInterval(ctx, 1, stringlen);
        PGASetIntegerAllele(ctx, p, pop, i, k);
        count++;
    }
    return ((double) count);
}
double evaluate(PGAContext *ctx, int p, int pop)
{
     int stringlen, i, sum = 0;
     stringlen = PGAGetStringLength(ctx);
     for (i = 0; i < stringlen; i++)
         sum += PGAGetIntegerAllele(ctx, p, pop, i);
     return ((double)sum);
}
