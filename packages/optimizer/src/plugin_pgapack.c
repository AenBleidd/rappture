/*
 * ----------------------------------------------------------------------
 *  OPTIMIZER PLUG-IN:  Pgapack
 *
 *  This code connects Pgapack into the Rappture Optimization
 *  infrastructure.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "pgapack.h"
#include "rp_optimizer.h"

typedef struct PgapackData {
    int operation;       /* operation <=> PGA_MINIMIZE/PGA_MAXIMIZE */
    int maxRuns;         /* maximum runs <=> PGASetMaxGAIterValue() */
    int popSize;         /* population size <=> PGASetPopSize() */
    int popRepl;         /* replacement <=> PGASetPopReplacementType() */
    int numReplPerPop;   /* number of new strings created per population, the rest are the best strings from the previous population*/
    double tgtFitness;   /* target fitness for stoppage --either best or average*/ 
    double fitnessTol;   /* %diff betn tgt fit. and current population fit. -- best or avg*/
    double tgtVariance;  /* target fitness variance*/
    double varianceTol;  /* tolerance : %diff betn tgt and current pop. fitness variance*/
    double tgtElapsedTime;   /* target stoppage time from start of execution*/
    int stpcriteria;     /*stoppage criteria <=> PGASetStoppingRuleType()*/
    int randnumseed;  /*Random Number Seed <=> PGASetRandomSeed()*/
    double mutnrate;     /*Mutation Rate <=> PGASetMutatuionProb()*/
    double mutnValue;     /*use this value while mutating*/
    double crossovrate;  /*Crossover Rate <=> PGASetCrossoverProb();*/
    int crossovtype;	/*Crossover Type <=> UNIFORM/SBX/TRIANGULAR (SBX Defined from Deb and Kumar 1995)*/
    int allowdup;        /*Allow duplicate strings in the population or not*/
    int mutnandcrossover;/*By default strings that do not undergo crossover undergo mutation, this option allows strings to crossover and be mutated*/
    double randReplProp; /*By default, random replacement is off, therefore randReplaceProp is zero by default, */
    						/*a nonzero replacement value causes random generation of individuals in later generations*/
} PgapackData;

RpCustomTclOptionGet RpOption_GetStpCriteria;
RpCustomTclOptionParse RpOption_ParseStpCriteria;
RpTclOptionType RpOption_StpCriteria = {
	"pga_stpcriteria", RpOption_ParseStpCriteria,RpOption_GetStpCriteria,NULL
};

RpCustomTclOptionParse RpOption_ParseOper;
RpCustomTclOptionGet RpOption_GetOper;
RpTclOptionType RpOption_Oper = {
    "pga_operation", RpOption_ParseOper, RpOption_GetOper, NULL
};

RpCustomTclOptionParse RpOption_ParseCrossovType;
RpCustomTclOptionGet RpOption_GetCrossovType;
RpTclOptionType RpOption_CrossovType = {
	"pga_crossovtype", RpOption_ParseCrossovType,RpOption_GetCrossovType,NULL
};

RpCustomTclOptionParse RpOption_ParsePopRepl;
RpCustomTclOptionGet RpOption_GetPopRepl;
RpTclOptionType RpOption_PopRepl = {
    "pga_poprepl", RpOption_ParsePopRepl, RpOption_GetPopRepl, NULL
};


typedef struct PgapackRuntimeDataTable{
	double **data;				/*Actual data per sample, like values of the genes, fitness of a sample, etc*/
	int num_of_rows;			/*Number of rows alloced..should be constant for a run*/
	int no_of_samples_evaled;	/*Number of samples evaluated so far*/
	int no_of_columns;					/*Number of columns allocated to the data table so far*/
}PgapackRuntimeDataTable;

RpTclOption PgapackOptions[] = {
  {"-maxruns", RP_OPTION_INT, Rp_Offset(PgapackData,maxRuns)},
  {"-operation", &RpOption_Oper, Rp_Offset(PgapackData,operation)},
  {"-poprepl", &RpOption_PopRepl, Rp_Offset(PgapackData,popRepl)},
  {"-numReplPerPop",RP_OPTION_INT,Rp_Offset(PgapackData,numReplPerPop)},
  {"-popsize", RP_OPTION_INT, Rp_Offset(PgapackData,popSize)},
  {"-mutnrate",RP_OPTION_DOUBLE,Rp_Offset(PgapackData,mutnrate)},
  {"-mutnValue",RP_OPTION_DOUBLE,Rp_Offset(PgapackData,mutnValue)},
  {"-crossovrate",RP_OPTION_DOUBLE,Rp_Offset(PgapackData,crossovrate)},
  {"-crossovtype",&RpOption_CrossovType,Rp_Offset(PgapackData,crossovtype)},
  {"-randnumseed",RP_OPTION_INT,Rp_Offset(PgapackData,randnumseed)},
  {"-tgtFitness",RP_OPTION_DOUBLE,Rp_Offset(PgapackData,tgtFitness)},
  {"-fitnessTol",RP_OPTION_DOUBLE,Rp_Offset(PgapackData,fitnessTol)},
  {"-tgtVariance",RP_OPTION_DOUBLE,Rp_Offset(PgapackData,tgtVariance)},
  {"-varianceTol",RP_OPTION_DOUBLE,Rp_Offset(PgapackData,varianceTol)},
  {"-tgtElapsedTime",RP_OPTION_DOUBLE,Rp_Offset(PgapackData,tgtElapsedTime)},
  {"-stpcriteria",&RpOption_StpCriteria,Rp_Offset(PgapackData,stpcriteria)},
  {"-allowdup",RP_OPTION_BOOLEAN,Rp_Offset(PgapackData,allowdup)},
  {"-mutnandcrossover",RP_OPTION_BOOLEAN,Rp_Offset(PgapackData,mutnandcrossover)},
  {"-randReplProp",RP_OPTION_DOUBLE,Rp_Offset(PgapackData,randReplProp)},
  {NULL, NULL, 0}
};

static double PgapEvaluate _ANSI_ARGS_((PGAContext *ctx, int p, int pop));
static void PgapCreateString _ANSI_ARGS_((PGAContext *ctx, int, int, int));
static int PgapMutation _ANSI_ARGS_((PGAContext *ctx, int, int, double));
static void PgapCrossover _ANSI_ARGS_((PGAContext *ctx, int, int, int,
    int, int, int));
static void PgapPrintString _ANSI_ARGS_((PGAContext *ctx, FILE*, int, int));
static void PgapCopyString _ANSI_ARGS_((PGAContext *ctx, int, int, int, int));
static int PgapDuplicateString _ANSI_ARGS_((PGAContext *ctx, int, int, int, int));
static MPI_Datatype PgapBuildDT _ANSI_ARGS_((PGAContext *ctx, int, int));

static void PgapLinkContext2Env _ANSI_ARGS_((PGAContext *ctx,
    RpOptimEnv *envPtr));
static RpOptimEnv* PgapGetEnvForContext _ANSI_ARGS_((PGAContext *ctx));
static void PgapUnlinkContext2Env _ANSI_ARGS_((PGAContext *ctx));
void PGARuntimeDataTableInit _ANSI_ARGS_((RpOptimEnv *envPtr));
void PGARuntimeDataTableDeInit();
void GetSampleInformation _ANSI_ARGS_((char *buffer, int sampleNumber));
void PGARuntimeDataTableSetSampleValue _ANSI_ARGS_((RpOptimParam *chrom, double fitness));
static PgapackRuntimeDataTable table;
/*
 * ----------------------------------------------------------------------
 * PgapackInit()
 *
 * This routine is called whenever a new optimization object is created
 * to initialize Pgapack.  Returns a pointer to PgapackData that is
 * used in later routines.
 * ----------------------------------------------------------------------
 */
ClientData
PgapackInit()
{
    PgapackData *dataPtr;

    dataPtr = (PgapackData*)malloc(sizeof(PgapackData));
    dataPtr->operation = PGA_MINIMIZE;
    dataPtr->maxRuns = 10000;
    dataPtr->popRepl = PGA_POPREPL_BEST;
    dataPtr->crossovtype = PGA_CROSSOVER_UNIFORM;
    dataPtr->popSize = 200;
    dataPtr->numReplPerPop = (dataPtr->popSize)/10; /*10% replaced by default, change to whatever value you need*/
    dataPtr->crossovrate = 0.85;
    dataPtr->mutnrate = 0.05; /*by default in PGAPack 1/stringlength*/
    dataPtr->mutnValue = 0.01;/*value of this number will be changed by plus/minus hundredth of its current value*/
    dataPtr->randnumseed = 1; /*should be a number greater than one, PGAPack requires it*/
    dataPtr->tgtFitness = PGA_UNINITIALIZED_DOUBLE;  
    dataPtr->fitnessTol = PGA_UNINITIALIZED_DOUBLE;  /*by default stop only if desired fitness is exactly achieved*/
    dataPtr->tgtVariance = PGA_UNINITIALIZED_DOUBLE; 
    dataPtr->varianceTol = PGA_UNINITIALIZED_DOUBLE; /* by default stop only if desired variance is exactly achieved*/
    dataPtr->tgtElapsedTime = PGA_UNINITIALIZED_DOUBLE; /* stop if 100 minutes have elapsed since start of optim run*/ 
    dataPtr->stpcriteria = PGA_STOP_NOCHANGE;
    dataPtr->allowdup = PGA_FALSE; /*Do not allow duplicate strings by default*/
    dataPtr->mutnandcrossover = PGA_FALSE;/*do not allow mutation and crossover to take place on the same string by default*/
    dataPtr->randReplProp = 0; /*0 randomly generated individuals after initialization, per generation*/
    return (ClientData)dataPtr;
}

int pgapack_abort = 0;
int pgapack_restart_user_action = 0;

/*
 * ----------------------------------------------------------------------
 * PgapackRun()
 *
 * This routine is called to kick off an optimization run.  Sets up
 * a PGApack context and starts invoking runs.
 * ----------------------------------------------------------------------
 */
RpOptimStatus
PgapackRun(envPtr, evalProc, fitnessExpr)
    RpOptimEnv *envPtr;           /* optimization environment */
    RpOptimEvaluator *evalProc;   /* call this proc to run tool */
    char *fitnessExpr;            /* fitness function in string form */
{
    PgapackData *dataPtr =(PgapackData*)envPtr->pluginData;
    PGAContext *ctx;

    /* pgapack requires at least one arg -- the executable name */
    /* fake it here by just saying something like "rappture" */
    int argc = 1; char *argv[] = {"rappture"};

    pgapack_abort = 0;		/* FALSE */
    PGASetAbortVar(&pgapack_abort);
    PGASetRestartUserAction(&pgapack_restart_user_action);

    ctx = PGACreate(&argc, argv, PGA_DATATYPE_USER, envPtr->numParams,
        dataPtr->operation);

    PGASetMaxGAIterValue(ctx, dataPtr->maxRuns);
    PGASetPopSize(ctx, dataPtr->popSize);
    PGASetPopReplaceType(ctx, dataPtr->popRepl);
    PGASetTgtFitnessVal(ctx,dataPtr->tgtFitness);
    PGASetFitnessTol(ctx,dataPtr->fitnessTol);
    PGASetTgtFitnessVariance(ctx,dataPtr->tgtVariance);
    PGASetVarTol(ctx,dataPtr->varianceTol);
    PGASetTgtElapsedTime(ctx,dataPtr->tgtElapsedTime);
    PGASetStoppingRuleType(ctx, dataPtr->stpcriteria);
    PGASetMutationProb(ctx,dataPtr->mutnrate);
    PGASetMutationRealValue(ctx,dataPtr->mutnValue);
    PGASetCrossoverProb(ctx,dataPtr->crossovrate);
    PGASetRandomSeed(ctx,dataPtr->randnumseed);
    PGASetCrossoverType(ctx, dataPtr->crossovtype);
    PGASetNoDuplicatesFlag(ctx,!(dataPtr->allowdup));
    PGASetMutationAndCrossoverFlag(ctx,dataPtr->mutnandcrossover);
    PGASetNumReplaceValue(ctx,dataPtr->numReplPerPop);
    PGASetRandReplProp(ctx,dataPtr->randReplProp);


    PGASetUserFunction(ctx, PGA_USERFUNCTION_CREATESTRING, PgapCreateString);
    PGASetUserFunction(ctx, PGA_USERFUNCTION_MUTATION, PgapMutation);
    PGASetUserFunction(ctx, PGA_USERFUNCTION_CROSSOVER, PgapCrossover);
    PGASetUserFunction(ctx, PGA_USERFUNCTION_PRINTSTRING, PgapPrintString);
    PGASetUserFunction(ctx, PGA_USERFUNCTION_COPYSTRING, PgapCopyString);
    PGASetUserFunction(ctx, PGA_USERFUNCTION_DUPLICATE, PgapDuplicateString);
    PGASetUserFunction(ctx, PGA_USERFUNCTION_BUILDDATATYPE, PgapBuildDT);

    envPtr->evalProc = evalProc;   /* plug these in for later during eval */
    envPtr->fitnessExpr = fitnessExpr;

    /*
     * We need a way to convert from a PGAContext to our RpOptimEnv
     * data.  This happens when Pgapack calls routines like
     * PgapCreateString, passing in the PGAContext, but nothing else.
     * Call PgapLinkContext2Env() here, so later on we can figure
     * out how many parameters, names, types, etc.
     */
    PgapLinkContext2Env(ctx, envPtr);

    PGASetUp(ctx);
    PGARun(ctx, PgapEvaluate);
    PGADestroy(ctx);
    PgapUnlinkContext2Env(ctx);

    if (pgapack_abort) {
	return RP_OPTIM_ABORTED;
    }
    return RP_OPTIM_SUCCESS;
}

/*
 * ----------------------------------------------------------------------
 * PgapackEvaluate()
 *
 * Called by PGApack whenever a set of input values needs to be
 * evaluated.  Passes the values on to the underlying Rappture tool,
 * launches a run, and computes the value of the fitness function.
 * Returns the value for the fitness function.
 * ----------------------------------------------------------------------
 */
double
PgapEvaluate(ctx, p, pop)
    PGAContext *ctx;  /* pgapack context for this optimization */
    int p;            /* sample #p being run */
    int pop;          /* identifier for this population */
    
{
    double fit = 0.0;
    RpOptimEnv *envPtr;
    RpOptimParam *paramPtr;
    RpOptimStatus status;
    envPtr = PgapGetEnvForContext(ctx);
    paramPtr = (RpOptimParam*)PGAGetIndividual(ctx, p, pop)->chrom;
    status = (*envPtr->evalProc)(envPtr, paramPtr, envPtr->numParams, &fit);
	
    if (pgapack_abort) {
        fprintf(stderr, "==WARNING: run aborted!");
        return 0.0;
    }
	
    if (status != RP_OPTIM_SUCCESS) {
        fprintf(stderr, "==WARNING: run failed!");
        PgapPrintString(ctx, stderr, p, pop);
    }
	
	/*populate the table with this sample*/
	PGARuntimeDataTableSetSampleValue(paramPtr,fit);
    return fit;
}

/*
 * ----------------------------------------------------------------------
 * PgapackCleanup()
 *
 * This routine is called whenever an optimization object is deleted
 * to clean up data associated with the object.  Frees the data
 * allocated in PgapackInit.
 * ----------------------------------------------------------------------
 */
void
PgapackCleanup(cdata)
    ClientData cdata;  /* data from to be cleaned up */
{
    PgapackData *dataPtr = (PgapackData*)cdata;
    free(dataPtr);
}

/*
 * ======================================================================
 *  ROUTINES FOR MANAGING DATA STRINGS
 * ======================================================================
 * PgapCreateString()
 *
 * Called by pgapack to create the so-called "string" of data used for
 * an evaluation.
 * ----------------------------------------------------------------------
 */
void
PgapCreateString(ctx, p, pop, initFlag)
    PGAContext *ctx;  /* pgapack context for this optimization */
    int p;            /* sample #p being run */
    int pop;          /* identifier for this population */
    int initFlag;     /* non-zero => fields should be initialized */
{
    int n, ival;
    double dval;
    RpOptimEnv *envPtr;
    RpOptimParam *oldParamPtr, *newParamPtr;
    PGAIndividual *newData;
    RpOptimParamNumber *numPtr;
    RpOptimParamString *strPtr;

    envPtr = PgapGetEnvForContext(ctx);

    newData = PGAGetIndividual(ctx, p, pop);
    newData->chrom = malloc(envPtr->numParams*sizeof(RpOptimParam));
    newParamPtr = (RpOptimParam*)newData->chrom;

    for (n=0; n < envPtr->numParams; n++) {
        oldParamPtr = envPtr->paramList[n];
        newParamPtr[n].name = oldParamPtr->name;
        newParamPtr[n].type = oldParamPtr->type;
        switch (oldParamPtr->type) {
        case RP_OPTIMPARAM_NUMBER:
            newParamPtr[n].value.dval = 0.0;
            break;
        case RP_OPTIMPARAM_STRING:
            newParamPtr[n].value.sval.num = -1;
            newParamPtr[n].value.sval.str = NULL;
            break;
        default:
            panic("bad parameter type in PgapCreateString()");
        }
    }

    if (initFlag) {
        for (n=0; n < envPtr->numParams; n++) {
            switch (newParamPtr[n].type) {
            case RP_OPTIMPARAM_NUMBER:
                numPtr = (RpOptimParamNumber*)envPtr->paramList[n];
                if(numPtr->randdist == RAND_NUMBER_DIST_UNIFORM){
                	dval = PGARandom01(ctx,0);
                	newParamPtr[n].value.dval = (numPtr->max - numPtr->min)*dval + numPtr->min;
                }else if(numPtr->randdist == RAND_NUMBER_DIST_GAUSSIAN){
	                	dval = PGARandomGaussian(ctx,numPtr->mean,numPtr->stddev);
	            		if(numPtr->strictmax){
	            			if(dval>numPtr->max){
	            				dval = numPtr->max;
	            			}
	            		}
	            		if(numPtr->strictmin){
	            			if(dval<numPtr->min){
	            				dval = numPtr->min;
	            			}
	            		}
	            		newParamPtr[n].value.dval = dval;
                }else{
                	panic("Incorrect Random Number distribution option in PgapcreateString()");
                }
                break;
            case RP_OPTIMPARAM_STRING:
                strPtr = (RpOptimParamString*)envPtr->paramList[n];
                ival = (int)floor(PGARandom01(ctx,0) * strPtr->numValues);
                envPtr->paramList[n]->value.sval.num = ival;
                envPtr->paramList[n]->value.sval.str = strPtr->values[ival];
                break;
            default:
                panic("bad parameter type in PgapCreateString()");
            }
        }
    }
}

/*
 * ----------------------------------------------------------------------
 * PgapMutation()
 *
 * Called by pgapack to perform random mutations on the input data
 * used for evaluation.
 * ----------------------------------------------------------------------
 */
int
PgapMutation(ctx, p, pop, mr)
    PGAContext *ctx;  /* pgapack context for this optimization */
    int p;            /* sample #p being run */
    int pop;          /* identifier for this population */
    double mr;        /* probability of mutation for each gene */
{
    int count = 0;    /* number of mutations */

    int n, ival,tempmr;
    double mutnVal;
    RpOptimEnv *envPtr;
    RpOptimParam *paramPtr;
    RpOptimParamNumber *numPtr;
    RpOptimParamString *strPtr;

    envPtr = PgapGetEnvForContext(ctx);
    paramPtr = (RpOptimParam*)PGAGetIndividual(ctx, p, pop)->chrom;

    for (n=0; n < envPtr->numParams; n++) {


            switch (paramPtr[n].type) {
            case RP_OPTIMPARAM_NUMBER:
                numPtr = (RpOptimParamNumber*)envPtr->paramList[n];
                if(numPtr->mutnrate!=PARAM_NUM_UNSPEC_MUTN_RATE){
                	tempmr = numPtr->mutnrate;
                	mutnVal = numPtr->mutnValue;
                }else{
                	tempmr = mr;
                	mutnVal = PGAGetMutationRealValue(ctx);
                }
                if (PGARandomFlip(ctx, tempmr)) {
		            /* won the coin toss -- change this parameter */
		            	count++;
		            	
	                /* bump the value up/down a little, randomly */
	                if (PGARandomFlip(ctx, 0.5)) {
	                    paramPtr[n].value.dval += mutnVal*paramPtr[n].value.dval; /*Made the mutation amount configurable*/
	                } else {
	                    paramPtr[n].value.dval -= mutnVal*paramPtr[n].value.dval;
	                }
	                /* make sure the resulting value is still in bounds */
	                if(numPtr->randdist == RAND_NUMBER_DIST_UNIFORM ||
	                 (numPtr->randdist == RAND_NUMBER_DIST_GAUSSIAN && numPtr->strictmax)){
		                if (paramPtr[n].value.dval > numPtr->max) {
		                    paramPtr[n].value.dval = numPtr->max;
		                }
	                 }
	                 /*also make sure it obeys configured parameters when gaussian*/
	                 if(numPtr->randdist == RAND_NUMBER_DIST_UNIFORM ||
	                 (numPtr->randdist == RAND_NUMBER_DIST_GAUSSIAN && numPtr->strictmin)){
		                if (paramPtr[n].value.dval < numPtr->min) {
		                    paramPtr[n].value.dval = numPtr->min;
		                }
	                 }
	                
                }
                break;
                

            case RP_OPTIMPARAM_STRING:
	            if (PGARandomFlip(ctx, mr)) {
	            /* won the coin toss -- change this parameter */
	            	count++;
	        
	                ival = paramPtr[n].value.sval.num;
	                if (PGARandomFlip(ctx, 0.5)) {
	                    ival += 1;
	                } else {
	                    ival -= 1;
	                }
	                strPtr = (RpOptimParamString*)envPtr->paramList[n];
	                if (ival < 0) ival = 0;
	                if (ival >= strPtr->numValues) ival = strPtr->numValues-1;
	                paramPtr[n].value.sval.num = ival;
	                paramPtr[n].value.sval.str = strPtr->values[ival];
	            }
	            
	            break;
	            
            default:
                panic("bad parameter type in PgapMutation()");
            }
        
    }
    return count;
}

/*
 * ----------------------------------------------------------------------
 * PgapCrossover()
 *
 * Called by pgapack to perform cross-over mutations on the input data
 * used for evaluation.
 * ----------------------------------------------------------------------
 */
void
PgapCrossover(ctx, p1, p2, pop1, c1, c2, pop2)
    PGAContext *ctx;  /* pgapack context for this optimization */
    int p1;           /* sample # for parent of input string1 */
    int p2;           /* sample # for parent of input string2 */
    int pop1;         /* population containing p1 and p2 */
    int c1;           /* sample # for child of input string1 */
    int c2;           /* sample # for child of input string2 */
    int pop2;         /* population containing c1 and c2 */
{
    int n;
    RpOptimEnv *envPtr;
    RpOptimParam *parent1, *parent2, *child1, *child2;
    double pu;
    PgapackData *dataPtr;
    /*declare variables for SBX*/
    double ui,beta,eta = 1.5,powVal;
    double slope = 3,xi;

    envPtr = PgapGetEnvForContext(ctx);
    parent1 = (RpOptimParam*)PGAGetIndividual(ctx, p1, pop1)->chrom;
    parent2 = (RpOptimParam*)PGAGetIndividual(ctx, p2, pop1)->chrom;
    child1  = (RpOptimParam*)PGAGetIndividual(ctx, c1, pop2)->chrom;
    child2  = (RpOptimParam*)PGAGetIndividual(ctx, c2, pop2)->chrom;
	
	pu = PGAGetCrossoverProb(ctx);
	dataPtr =(PgapackData*)envPtr->pluginData;
	
	for (n=0; n < envPtr->numParams; n++) {
        if (PGARandomFlip(ctx, pu)) {
            /* crossover */
            switch(dataPtr->crossovtype){
            	case PGA_CROSSOVER_UNIFORM:
            		memcpy(&child1[n], &parent2[n], sizeof(RpOptimParam));
            		memcpy(&child2[n], &parent1[n], sizeof(RpOptimParam));
            		break;
            	case PGA_CROSSOVER_SBX:
            		/*Implement a Simulated Binary Crossover for Real Encoding*/
            		/*From Deb and Agrawal, 1995; Deb and Kumar, 1995)*/
            		switch(parent1[n].type){
            			case RP_OPTIMPARAM_NUMBER:
            				ui = PGARandom01(ctx,0);
	            			powVal = 1/(eta+1);
            				if(ui<=0.5){
	            				beta = pow(2*ui,powVal);
            				}else{
            					beta = pow(0.5/(1-ui),powVal);
            				}
            				child1[n].value.dval = 0.5*((1+beta)*(parent1[n].value.dval) + (1-beta)*(parent2[n].value.dval));
            				child2[n].value.dval = 0.5*((1-beta)*(parent1[n].value.dval) + (1+beta)*(parent2[n].value.dval));
            				break;
            			default:
            				panic("Bad Optim Param Type in PgapCrossover()");
            		}
            		break;
            		
            		
            		case PGA_CROSSOVER_TRIANGULAR:
            		xi = 1/sqrt(slope);
            		switch(parent1[n].type){
            			case RP_OPTIMPARAM_NUMBER:
            				ui = PGARandom01(ctx,0);
	            			
            				if(ui<=0.5){
                				beta = sqrt(2*ui/slope);
        					}else{
                				beta = 2*xi-sqrt(2*(1-ui)/slope);
        					}
            				child1[n].value.dval = beta + (parent1[n].value.dval - xi) ;
            				child2[n].value.dval = beta + (parent2[n].value.dval - xi) ;
            				break;
            			default:
            				panic("Bad Optim Param Type in PgapCrossover()");
            		}
            		break;
            		
            		
            	default:
            		panic("bad parameter type in PgapCrossover()");
            }
        } else {
            /* child inherits from parent */
            memcpy(&child1[n], &parent1[n], sizeof(RpOptimParam));
            memcpy(&child2[n], &parent2[n], sizeof(RpOptimParam));
        }
    }
			
		
}

/*
 * ----------------------------------------------------------------------
 * PgapPrintString()
 *
 * Called by pgapack to format the values for a particular string of
 * input data.
 * ----------------------------------------------------------------------
 */
void
PgapPrintString(ctx, fp, p, pop)
    PGAContext *ctx;  /* pgapack context for this optimization */
    FILE *fp;         /* write to this file pointer */
    int p;            /* sample #p being run */
    int pop;          /* identifier for this population */
{
    int n;
    RpOptimEnv *envPtr;
    RpOptimParam *paramPtr;

    envPtr = PgapGetEnvForContext(ctx);
    paramPtr = (RpOptimParam*)PGAGetIndividual(ctx, p, pop)->chrom;

    for (n=0; n < envPtr->numParams; n++) {
        fprintf(fp, "#%4d: ", n);
        switch (paramPtr[n].type) {
        case RP_OPTIMPARAM_NUMBER:
            fprintf(fp, "[%11.7g] (%s)\n", paramPtr[n].value.dval,
                paramPtr[n].name);
            break;
        case RP_OPTIMPARAM_STRING:
            fprintf(fp, "[%d]=\"%s\" (%s)\n", paramPtr[n].value.sval.num,
                paramPtr[n].value.sval.str, paramPtr[n].name);
            break;
        default:
            panic("bad parameter type in PgapPrintString()");
        }
    }
}

/*
 * ----------------------------------------------------------------------
 * PgapCopyString()
 *
 * Called by pgapack to copy one input string to another.
 * ----------------------------------------------------------------------
 */
void
PgapCopyString(ctx, p1, pop1, p2, pop2)
    PGAContext *ctx;  /* pgapack context for this optimization */
    int p1;           /* source sample # being run */
    int pop1;         /* population containing p1 */
    int p2;           /* destination sample # being run */
    int pop2;         /* population containing p1 */
{
    int n;
    RpOptimEnv *envPtr;
    RpOptimParam *src, *dst;

    envPtr = PgapGetEnvForContext(ctx);
    src = (RpOptimParam*)PGAGetIndividual(ctx, p1, pop1)->chrom;
    dst = (RpOptimParam*)PGAGetIndividual(ctx, p2, pop2)->chrom;

    for (n=0; n < envPtr->numParams; n++) {
        dst[n].type = src[n].type;
        switch (src[n].type) {
        case RP_OPTIMPARAM_NUMBER:
            dst[n].value.dval = src[n].value.dval;
            break;
        case RP_OPTIMPARAM_STRING:
            dst[n].value.sval.num = src[n].value.sval.num;
            dst[n].value.sval.str = src[n].value.sval.str;
            break;
        default:
            panic("bad parameter type in PgapCopyString()");
        }
    }
}

/*
 * ----------------------------------------------------------------------
 * PgapDuplicateString()
 *
 * Called by pgapack to compare two input strings.  Returns non-zero if
 * the two are duplicates and 0 otherwise.
 * ----------------------------------------------------------------------
 */
int
PgapDuplicateString(ctx, p1, pop1, p2, pop2)
    PGAContext *ctx;  /* pgapack context for this optimization */
    int p1;           /* sample #p being run */
    int pop1;         /* population containing p1 */
    int p2;           /* sample #p being run */
    int pop2;         /* population containing p1 */
{
    int n;
    RpOptimEnv *envPtr;
    RpOptimParam *param1, *param2;

    envPtr = PgapGetEnvForContext(ctx);
    param1 = (RpOptimParam*)PGAGetIndividual(ctx, p1, pop1)->chrom;
    param2 = (RpOptimParam*)PGAGetIndividual(ctx, p2, pop2)->chrom;

    for (n=0; n < envPtr->numParams; n++) {
        if (param1[n].type != param2[n].type) {
            return 0;  /* different! */
        }
        switch (param1[n].type) {
        case RP_OPTIMPARAM_NUMBER:
            if (param1[n].value.dval != param2[n].value.dval) {
                return 0;  /* different! */
            }
            break;
        case RP_OPTIMPARAM_STRING:
            if (param1[n].value.sval.num != param2[n].value.sval.num) {
                return 0;  /* different! */
            }
            break;
        default:
            panic("bad parameter type in PgapDuplicateString()");
        }
    }
    return 1;
}



/*
 * ----------------------------------------------------------------------
 * PgapCopyString()
 *
 * Called by pgapack to copy one input string to another.
 * ----------------------------------------------------------------------
 */
MPI_Datatype
PgapBuildDT(ctx, p, pop)
    PGAContext *ctx;  /* pgapack context for this optimization */
    int p;            /* sample # being run */
    int pop;          /* population containing sample */
{
    panic("MPI support not implemented!");
    return NULL;
}

/*
 * ======================================================================
 *  OPTION:  -operation <=> PGA_MINIMIZE / PGA_MAXIMIZE
 * ======================================================================
 */
int
RpOption_ParseOper(interp, valObj, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    Tcl_Obj *valObj;     /* set option to this new value */
    ClientData cdata;    /* save in this data structure */
    int offset;          /* save at this offset in cdata */
{
    int *ptr = (int*)(cdata+offset);
    char *val = Tcl_GetStringFromObj(valObj, (int*)NULL);
    if (strcmp(val,"minimize") == 0) {
        *ptr = PGA_MINIMIZE;
    }
    else if (strcmp(val,"maximize") == 0) {
        *ptr = PGA_MAXIMIZE;
    }
    else {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "bad value \"", val, "\": should be minimize, maximize",
            (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

int
RpOption_GetOper(interp, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    ClientData cdata;    /* get from this data structure */
    int offset;          /* get from this offset in cdata */
{
    int *ptr = (int*)(cdata+offset);
    switch (*ptr) {
    case PGA_MINIMIZE:
        Tcl_SetResult(interp, "minimize", TCL_STATIC);
        break;
    case PGA_MAXIMIZE:
        Tcl_SetResult(interp, "maximize", TCL_STATIC);
        break;
    default:
        Tcl_SetResult(interp, "???", TCL_STATIC);
        break;
    }
    return TCL_OK;
}


/*
 * ======================================================================
 *  OPTION:  -crossovtype <=> PGA_CROSSOVER_UNIFORM / PGA_CROSSOVER_SBX
 * ======================================================================
 */
int
RpOption_ParseCrossovType(interp, valObj, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    Tcl_Obj *valObj;     /* set option to this new value */
    ClientData cdata;    /* save in this data structure */
    int offset;          /* save at this offset in cdata */
{
    int *ptr = (int*)(cdata+offset);
    char *val = Tcl_GetStringFromObj(valObj, (int*)NULL);
    if (strcmp(val,"uniform") == 0) {
        *ptr = PGA_CROSSOVER_UNIFORM;
    }
    else if (strcmp(val,"sbx") == 0) {
        *ptr = PGA_CROSSOVER_SBX;
    }
    else if (strcmp(val,"triangular")== 0 ){
    	*ptr = PGA_CROSSOVER_TRIANGULAR;
    }
    else {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "bad value \"", val, "\": should be either 'uniform' or 'sbx'",
            (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

int
RpOption_GetCrossovType(interp, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    ClientData cdata;    /* get from this data structure */
    int offset;          /* get from this offset in cdata */
{
    int *ptr = (int*)(cdata+offset);
    switch (*ptr) {
    case PGA_CROSSOVER_UNIFORM:
        Tcl_SetResult(interp, "uniform", TCL_STATIC);
        break;
    case PGA_CROSSOVER_SBX:
        Tcl_SetResult(interp, "sbx", TCL_STATIC);
        break;
    case PGA_CROSSOVER_TRIANGULAR:
    	Tcl_SetResult(interp,"triangular",TCL_STATIC);
    	break;    
    default:
        Tcl_SetResult(interp, "???", TCL_STATIC);
        break;
    }
    return TCL_OK;
}




/*
 * ======================================================================
 *  OPTION:  -poprepl <=> PGASetPopReplacementType()
 * ======================================================================
 */
int
RpOption_ParsePopRepl(interp, valObj, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    Tcl_Obj *valObj;     /* set option to this new value */
    ClientData cdata;    /* save in this data structure */
    int offset;          /* save at this offset in cdata */
{
    int *ptr = (int*)(cdata+offset);
    char *val = Tcl_GetStringFromObj(valObj, (int*)NULL);
    if (*val == 'b' && strcmp(val,"best") == 0) {
        *ptr = PGA_POPREPL_BEST;
    }
    else if (*val == 'r' && strcmp(val,"random-repl") == 0) {
        *ptr = PGA_POPREPL_RANDOM_REP;
    }
    else if (*val == 'r' && strcmp(val,"random-norepl") == 0) {
        *ptr = PGA_POPREPL_RANDOM_NOREP;
    }
    else {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "bad value \"", val, "\": should be best, random-norepl,"
            " or random-repl", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

int
RpOption_GetPopRepl(interp, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    ClientData cdata;    /* get from this data structure */
    int offset;          /* get from this offset in cdata */
{
    int *ptr = (int*)(cdata+offset);
    switch (*ptr) {
    case PGA_POPREPL_BEST:
        Tcl_SetResult(interp, "best", TCL_STATIC);
        break;
    case PGA_POPREPL_RANDOM_REP:
        Tcl_SetResult(interp, "random-repl", TCL_STATIC);
        break;
    case PGA_POPREPL_RANDOM_NOREP:
        Tcl_SetResult(interp, "random-norepl", TCL_STATIC);
        break;
    default:
        Tcl_SetResult(interp, "???", TCL_STATIC);
        break;
    }
    return TCL_OK;
}

/*
 * ==================================================================================================================
 *  OPTION:  -stpcriteria <=> PGA_STOP_MAXITER / PGA_STOP_NOCHANGE / PGA_STOP_TOOSIMILAR /
 * 	PGA_STOP_AV_FITNESS / PGA_STOP_BEST_FITNESS / PGA_STOP_VARIANCE / PGA_STOP_TIMEELAPSED
 * ==================================================================================================================
 */
int
RpOption_ParseStpCriteria(interp, valObj, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    Tcl_Obj *valObj;     /* set option to this new value */
    ClientData cdata;    /* save in this data structure */
    int offset;          /* save at this offset in cdata */
{
    int *ptr = (int*)(cdata+offset);
    char *val = Tcl_GetStringFromObj(valObj, (int*)NULL);
    if (strcmp(val,"maxiter") == 0) {
        *ptr = PGA_STOP_MAXITER;
    }
    else if (strcmp(val,"nochange") == 0) {
        *ptr = PGA_STOP_NOCHANGE;
    }
    else if (strcmp(val,"toosimilar") == 0){
    	*ptr = PGA_STOP_TOOSIMILAR;
    }
    else if (strcmp(val,"avfitness") == 0){
    	*ptr = PGA_STOP_AV_FITNESS;
    }
    else if (strcmp(val,"bestfitness") == 0){
    	*ptr = PGA_STOP_BEST_FITNESS;
    }
    else if (strcmp(val,"varoffitness") == 0){
    	*ptr = PGA_STOP_VARIANCE;
    }
    else if (strcmp(val,"timeelapsed") == 0){
    	*ptr = PGA_STOP_TIMEELAPSED;
    }
    
    else {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "bad value \"", val, "\": should be one of the following: maxiter, nochange, toosimilar, avfitness, bestfitness, varoffitness or timeelapsed",
            (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

int
RpOption_GetStpCriteria(interp, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    ClientData cdata;    /* get from this data structure */
    int offset;          /* get from this offset in cdata */
{
    int *ptr = (int*)(cdata+offset);
    switch (*ptr) {
    case PGA_STOP_MAXITER:
        Tcl_SetResult(interp, "maxiter", TCL_STATIC);
        break;
    case PGA_STOP_NOCHANGE:
        Tcl_SetResult(interp, "nochange", TCL_STATIC);
        break;
    case PGA_STOP_TOOSIMILAR:
    	Tcl_SetResult(interp, "toosimilar", TCL_STATIC);
    	break;
    case PGA_STOP_AV_FITNESS:
        Tcl_SetResult(interp, "avfitness", TCL_STATIC);
        break;
    case PGA_STOP_BEST_FITNESS:
        Tcl_SetResult(interp, "bestfitness", TCL_STATIC);
        break;
    case PGA_STOP_VARIANCE:
        Tcl_SetResult(interp, "varoffitness", TCL_STATIC);
        break;
    case PGA_STOP_TIMEELAPSED:
        Tcl_SetResult(interp, "timeelapsed", TCL_STATIC);
        break;        
    default:
        Tcl_SetResult(interp, "???", TCL_STATIC);
        break;
    }
    return TCL_OK;
}

/*
 * ======================================================================
 *  ROUTINES FOR CONNECTING PGACONTEXT <=> RPOPTIMENV
 * ======================================================================
 * PgapLinkContext2Env()
 *   This routine is used internally to establish a relationship between
 *   a PGAContext token and its corresponding RpOptimEnv data.  The
 *   PGA routines don't provide a way to pass the RpOptimEnv data along,
 *   so we use these routines to find the correspondence.
 *
 * PgapGetEnvForContext()
 *   Returns the RpOptimEnv associated with a given PGAContext.  If the
 *   link has not been established via PgapLinkContext2Env(), then this
 *   routine returns NULL.
 *
 * PgapUnlinkContext2Env()
 *   Breaks the link between a PGAContext and its RpOptimEnv.  Should
 *   be called when the PGAContext is destroyed and is no longer valid.
 * ----------------------------------------------------------------------
 */
static Tcl_HashTable *Pgacontext2Rpenv = NULL;

void
PgapLinkContext2Env(ctx, envPtr)
    PGAContext *ctx;      /* pgapack context for this optimization */
    RpOptimEnv *envPtr;   /* corresponding Rappture optimization data */
{
    Tcl_HashEntry *ctxEntry;
    int newEntry;

    if (Pgacontext2Rpenv == NULL) {
        Pgacontext2Rpenv = (Tcl_HashTable*)malloc(sizeof(Tcl_HashTable));
        Tcl_InitHashTable(Pgacontext2Rpenv, TCL_ONE_WORD_KEYS);
    }
    ctxEntry = Tcl_CreateHashEntry(Pgacontext2Rpenv, (char*)ctx, &newEntry);
    Tcl_SetHashValue(ctxEntry, (ClientData)envPtr);
}

RpOptimEnv*
PgapGetEnvForContext(ctx)
    PGAContext *ctx;
{
    Tcl_HashEntry *entryPtr;

    if (Pgacontext2Rpenv) {
        entryPtr = Tcl_FindHashEntry(Pgacontext2Rpenv, (char*)ctx);
        if (entryPtr) {
            return (RpOptimEnv*)Tcl_GetHashValue(entryPtr);
        }
    }
    return NULL;
}

void
PgapUnlinkContext2Env(ctx)
    PGAContext *ctx;
{
    Tcl_HashEntry *entryPtr;

    if (Pgacontext2Rpenv) {
        entryPtr = Tcl_FindHashEntry(Pgacontext2Rpenv, (char*)ctx);
        if (entryPtr) {
            Tcl_DeleteHashEntry(entryPtr);
        }
    }
}
/*---------------------------------------------------------------------------------
 * PGARuntimeDTInit(): It initializes the runtime data table.
 * The table is organized slightly counter-intuitively
 * Instead of a
 *  param1|param2|param3  |param4...
 * 	val11 |val12 |val13   |val14...
 * 	val12 |val22 |val23   |val24....
 * orientation, it is organized as
 * 	param1|val11|val12
 * 	param2|val21|val22
 * 	param3|val31|val32
 * 	param4|val41|val42
 * Reallocating for additional columns is easier than reallocating additional rows and then
 * reallocating for columns 
 * --------------------------------------------------------------------------------
 */

void PGARuntimeDataTableInit(envPtr)
RpOptimEnv *envPtr;
{    
	int i;
	if(envPtr != NULL){
		table.num_of_rows = (envPtr->numParams)+1;
		table.data = malloc((table.num_of_rows)*sizeof(double*));
		if(table.data == NULL){
			panic("\nAllocation for Runtime Data Table failed\n");
		}
		for(i=0;i<table.num_of_rows;i++){
			table.data[i] = malloc(PGAPACK_RUNTIME_TABLE_DEFAULT_SIZE*sizeof(double));
			if(table.data[i] == NULL){
				panic("\nAllocation for Runtime Data Table failed\n");
			}			
		}
		table.no_of_samples_evaled = 0;
		table.no_of_columns = PGAPACK_RUNTIME_TABLE_DEFAULT_SIZE;
		
	}else{
		panic("\nError: NULL Environment variable OR Table pointer passed to Data Table Init\n");
	}
}

void PGARuntimeDataTableDeInit()
{	
	int i;
	if((&table) == NULL){
		panic("Error: Table not present, therefore cannot free memory..");
	}
	for(i=0;i<table.num_of_rows;i++){
		free(table.data[i]);
	}
	free(table.data);
}

void PGARuntimeDataTableSetSampleValue(chrom,fitness)
RpOptimParam *chrom;
double fitness;
{
	int i;
	//printf("\nSetting sample value.......................\n");
	if(chrom!=NULL && (&table)!=NULL){
		(table.no_of_samples_evaled)+=1;
		if((table.no_of_samples_evaled) > table.no_of_columns){
			/* then Reallocate space for more columns)*/
			(table.no_of_columns)+=(table.no_of_columns);
				//TODO GTG: Delete printing stuff
			for(i=0;i<(table.num_of_rows);i++){
				table.data[i] = realloc(table.data[i],table.no_of_columns);
				if(table.data[i]==NULL){
					panic("\nError: Could not Reallocate more space for the table");
				}				
			}
		}else{
			if(chrom->type == RP_OPTIMPARAM_NUMBER){
				for(i=0;i<(table.num_of_rows);i++){
					if(i==0){
						table.data[i][(table.no_of_samples_evaled)-1] = fitness;
						//printf("\nSample Number %d:- Fitness: %lf\t",table.no_of_samples_evaled,fitness);
					}else{
						table.data[i][(table.no_of_samples_evaled)-1] = chrom[i-1].value.dval;
						//printf("Param %d %lf\t",i,table.data[i][(table.no_of_samples_evaled)-1]);
					}
                }
			}else{
				panic("\n Chromosome value is RP_OPTIMPARAM_STRING\n");
				//GTG TODO: find out what happens in this case. Will we be better off handling Tcl_objects?
			}	
		}
	}else{
		panic("\nError:Either Chromosome, or table passed to PGARuntimeDataTableSetSampleValue() is NULL\n"); 
	}
	
}

void GetSampleInformation(buffer,sampleNumber)
	char *buffer;
	int sampleNumber;
{
	int i;
	char tempBuff[50];
	printf("\nFetching sample information.........................\n");
	if((&table) == NULL){
		panic("Table uninitialized");
	}
	if(sampleNumber<=0){
		sprintf(buffer,"\nNumber of Samples Evaluated so far: %d\n",(table.no_of_samples_evaled)+1);
		return;
	}
	if(((table.num_of_rows)-1)*10>SINGLE_SAMPLE_DATA_BUFFER_DEFAULT_SIZE){
		buffer = realloc(buffer,50+25*(table.num_of_rows));
		//resizing the buffer, keeping 50 for display related jazz, around 12-15 characs for param names
		//and 10 characs for Fl.pt. display of the value
		if(buffer == NULL){
			panic("\nError: Could not reallocate space for sample data buffer");
		}
	}
	for(i=0;i<(table.num_of_rows);i++){
		if(i==0){
			sprintf(buffer,"\nSample Number %d ----> Fitness: %lf  ",sampleNumber,table.data[i][sampleNumber-1]);
		}else{
			sprintf(tempBuff,"Param %d: %lf  ",i,table.data[i][sampleNumber-1]);
			strcat(buffer,tempBuff);
		}
	}
	strcat(buffer,"\n");
}
