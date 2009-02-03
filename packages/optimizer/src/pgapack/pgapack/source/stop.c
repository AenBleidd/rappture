/*
 *  
 *  ********************************************************************* 
 *  (C) COPYRIGHT 1995 UNIVERSITY OF CHICAGO 
 *  *********************************************************************
 *  
 *  This software was authored by
 *  
 *  D. Levine
 *  Mathematics and Computer Science Division Argonne National Laboratory
 *  Argonne IL 60439
 *  levine@mcs.anl.gov
 *  (708) 252-6735
 *  (708) 252-5986 (FAX)
 *  
 *  with programming assistance of participants in Argonne National 
 *  Laboratory's SERS program.
 *  
 *  This program contains material protectable under copyright laws of the 
 *  United States.  Permission is hereby granted to use it, reproduce it, 
 *  to translate it into another language, and to redistribute it to 
 *  others at no charge except a fee for transferring a copy, provided 
 *  that you conspicuously and appropriately publish on each copy the 
 *  University of Chicago's copyright notice, and the disclaimer of 
 *  warranty and Government license included below.  Further, permission 
 *  is hereby granted, subject to the same provisions, to modify a copy or 
 *  copies or any portion of it, and to distribute to others at no charge 
 *  materials containing or derived from the material.
 *  
 *  The developers of the software ask that you acknowledge its use in any 
 *  document referencing work based on the  program, such as published 
 *  research.  Also, they ask that you supply to Argonne National 
 *  Laboratory a copy of any published research referencing work based on 
 *  the software.
 *  
 *  Any entity desiring permission for further use must contact:
 *  
 *  J. Gleeson
 *  Industrial Technology Development Center Argonne National Laboratory
 *  Argonne IL 60439
 *  gleesonj@smtplink.eid.anl.gov
 *  (708) 252-6055
 *  
 *  ******************************************************************** 
 *  DISCLAIMER
 *  
 *  THIS PROGRAM WAS PREPARED AS AN ACCOUNT OF WORK SPONSORED BY AN AGENCY 
 *  OF THE UNITED STATES GOVERNMENT.  NEITHER THE UNIVERSITY OF CHICAGO, 
 *  THE UNITED STATES GOVERNMENT NOR ANY OF THEIR EMPLOYEES MAKE ANY 
 *  WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL LIABILITY OR 
 *  RESPONSIBILITY FOR THE ACCURACY, COMPLETENESS, OR USEFULNESS OF ANY 
 *  INFORMATION OR PROCESS DISCLOSED, OR REPRESENTS THAT ITS USE WOULD NOT 
 *  INFRINGE PRIVATELY OWNED RIGHTS.
 *  
 *  ********************************************************************** 
 *  GOVERNMENT LICENSE
 *  
 *  The Government is granted for itself and others acting on its behalf a 
 *  paid-up, non-exclusive, irrevocable worldwide license in this computer 
 *  software to reproduce, prepare derivative works, and perform publicly 
 *  and display publicly.
 */

/*****************************************************************************
*     FILE: stop.c: This file contains routines related to the stopping
*                   conditions for the GA.
*
*     Authors: David M. Levine, Philip L. Hallstrom, David M. Noelle,
*              Brian P. Walenz
*****************************************************************************/

#include "pgapack.h"

int *pgapack_abortPtr = NULL;

/*U****************************************************************************
  PGADone - Returns PGA_TRUE if the stopping conditions have been met,
  otherwise returns false.  Calls exactly one of the user defined C or
  fortran or system (PGACheckStoppingConditions) stopping condition functions.

  Category: Generation

  Inputs:
     ctx  - context variable
     comm - an MPI communicator

  Outputs:
     returns PGA_TRUE if at least one of the termination conditions has been
     met.  Otherwise, returns PGA_FALSE

  Example:
    PGAContext *ctx;
    :
    PGADone(ctx, comm);

****************************************************************************U*/
int PGADone(PGAContext *ctx, MPI_Comm comm)
{
    int rank, size, done;

    PGADebugEntered("PGADone");

    rank = PGAGetRank(ctx, comm);
    size = PGAGetNumProcs(ctx, comm);

    if (rank == 0) {
		if (ctx->fops.StopCond)
		    done = (*ctx->fops.StopCond)(&ctx);
		else if (ctx->cops.StopCond)
		    done = (*ctx->cops.StopCond)(ctx);
		else
		    done = PGACheckStoppingConditions(ctx);
    }

    if (size > 1)
	MPI_Bcast(&done, 1, MPI_INT, 0, comm);

    PGADebugExited("PGADone");

    return(done);
}

/*U****************************************************************************
  PGACheckStoppingConditions - returns boolean to indicate if the PGAPack
  termination conditions -- PGA_STOP_MAXITER, PGA_STOP_TOOSIMILAR, 
  PGA_STOP_NOCHANGE -- have been met.

  Category: Generation

  Inputs:
     ctx  - context variable

  Outputs:
     returns PGA_TRUE if at least one of the termination conditions has been
     met.  Otherwise, returns PGA_FALSE

  Example:
    PGAContext *ctx;
    :
    PGACheckStoppingConditions(ctx);

****************************************************************************U*/
int PGACheckStoppingConditions( PGAContext *ctx)
{
    int done = PGA_FALSE;
    double diff;

    PGADebugEntered("PGACheckStoppingConditions");

    if (((ctx->ga.StoppingRule & PGA_STOP_MAXITER) == PGA_STOP_MAXITER) &&
	(ctx->ga.iter > ctx->ga.MaxIter))
	done |= PGA_TRUE;
    
    if (((ctx->ga.StoppingRule & PGA_STOP_NOCHANGE) == PGA_STOP_NOCHANGE) &&
	(ctx->ga.ItersOfSame >= ctx->ga.MaxNoChange))
	done |= PGA_TRUE;
	
    if (((ctx->ga.StoppingRule & PGA_STOP_TOOSIMILAR) == PGA_STOP_TOOSIMILAR) &&
	(ctx->ga.PercentSame >= ctx->ga.MaxSimilarity))
	done |= PGA_TRUE;
	
	if(((ctx->ga.StoppingRule & PGA_STOP_AV_FITNESS) == PGA_STOP_AV_FITNESS)){
		if(ctx->ga.TgtFitnessVal == PGA_UNINITIALIZED_DOUBLE){
			fprintf(stderr,"Uninitialized Value 'Target Average'");
			done |= PGA_TRUE;
		}else{
			diff = ((ctx->ga.TgtFitnessVal - ctx->rep.Average)/(ctx->ga.TgtFitnessVal))*100;
			if(fabs(diff) <= ctx->ga.FitnessTol){
				done |= PGA_TRUE;
			}
		}
	}
	
	if(((ctx->ga.StoppingRule & PGA_STOP_BEST_FITNESS) == PGA_STOP_BEST_FITNESS)){
		if(ctx->ga.TgtFitnessVal == PGA_UNINITIALIZED_DOUBLE){
			fprintf(stderr,"Uninitialized Value 'Target Best Fitness'");
			done |= PGA_TRUE;
		}else{
			diff =(((double)ctx->ga.TgtFitnessVal -(double)ctx->rep.Best)/((double)ctx->ga.TgtFitnessVal))*100;
			if(fabs(diff) <= ctx->ga.FitnessTol){
				done |= PGA_TRUE;
			}
		}
	}
	
	if(((ctx->ga.StoppingRule & PGA_STOP_VARIANCE) == PGA_STOP_VARIANCE)){
		if(ctx->ga.TgtFitnessVar == PGA_UNINITIALIZED_DOUBLE){
			fprintf(stderr,"Uninitialized Value 'Target Fitness Variance'");
			done |= PGA_TRUE;
		}else{
			diff =(((double)ctx->ga.TgtFitnessVar -(double)ctx->rep.Variance)/((double)ctx->ga.TgtFitnessVar))*100;
			if(fabs(diff) <= ctx->ga.VarTol){
				done |= PGA_TRUE;
			}
		}
	}
	
	if(((ctx->ga.StoppingRule & PGA_STOP_TIMEELAPSED) == PGA_STOP_TIMEELAPSED)){
		//TODO :GTG Add code for setting initial time and current time and comparing
		done |= PGA_TRUE;
	}

    PGADebugExited("PGACheckStoppingConditions");
    return(done);
}

/*U****************************************************************************
   PGASetStoppingRuleType - specify a stopping criterion.  If called more than
   once the different stopping criterion are ORed together.  Valid choices
   are PGA_STOP_MAXITER, PGA_STOP_TOOSIMILAR, or PGA_STOP_NOCHANGE to
   specify iteration limit reached, population too similar, or no change in
   the best solution found in a given number of iterations, respectively.
   The default is to stop when a maximum iteration limit is reached (by
   default, 1000 iterations).

   Category: Generation

   Inputs:
      ctx      - context variable
      stoprule - symbolic constant to specify stopping rule

   Outputs:
      None

   Example:
      PGAContext *ctx;
      :
      PGASetStoppingRuleType(ctx, PGA_STOP_TOOSIMILAR);

****************************************************************************U*/
void PGASetStoppingRuleType (PGAContext *ctx, int stoprule)
{

    PGADebugEntered("PGASetStoppingRuleType");
    PGAFailIfSetUp("PGASetStoppingRuleType");

    switch (stoprule) {
	case PGA_STOP_MAXITER  :
    case PGA_STOP_NOCHANGE :
	case PGA_STOP_TOOSIMILAR :
	case PGA_STOP_AV_FITNESS :
	case PGA_STOP_BEST_FITNESS :
	case PGA_STOP_VARIANCE :
	case PGA_STOP_TIMEELAPSED :
	    ctx->ga.StoppingRule |= stoprule;
	    break;
	default:
	    PGAError( ctx,
		     "PGASetStoppingRuleType: Invalid value of stoprule:",
		     PGA_FATAL, PGA_INT, (void *) &stoprule );
    }

    PGADebugExited("PGASetStoppingRuleType");
}

/*U***************************************************************************
   PGAGetStoppingRuleType - Returns a symbolic constant that defines the
   termination criteria.

   Category: Generation

   Inputs:
      ctx - context variable

   Outputs:
      Returns an integer which is an ORed mask of the symbolic constants
      used to specify the stopping rule(s).

   Example:
      PGAContext *ctx;
      int stop;
      :
      stop = PGAGetStoppingRuleType(ctx);
      if (stop & PGA_STOP_MAXITER)
          printf("Stopping Rule = PGA_STOP_MAXITER\n");
      if (stop & PGA_STOP_NOCHANGE)
          printf("Stopping Rule = PGA_STOP_NOCHANGE\n");
      if (stop & PGA_STOP_TOOSIMILAR)
          printf("Stopping Rule = PGA_STOP_TOOSIMILAR\n");

***************************************************************************U*/
int PGAGetStoppingRuleType (PGAContext *ctx)
{
    PGADebugEntered("PGAGetStoppingRuleType");
    PGAFailIfNotSetUp("PGAGetStoppingRuleType");

    PGADebugExited("PGAGetStoppingRuleType");

    return(ctx->ga.StoppingRule);
}

/*U****************************************************************************
   PGASetMaxGAIterValue - specify the maximum number of iterations for the
   stopping rule PGA_STOP_MAXITER (which, by itself, is the default stopping
   rule and is always in effect).  The default value is 1000 iterations.

   Category: Generation

   Inputs:
      ctx     - context variable
      maxiter - the maximum number of GA iterations to run before stopping

   Outputs:
      None

   Example:
      PGAContext *ctx;
      :
      PGASetMaxGAIterValue(ctx,5000);

****************************************************************************U*/
void PGASetMaxGAIterValue(PGAContext *ctx, int maxiter)
{

    PGADebugEntered("PGASetMaxGAIterValue");
    PGAFailIfSetUp("PGASetMaxGAIterValue");

    if (maxiter < 1)
	PGAError( ctx, "PGASetMaxGAIterValue: Invalid value of maxiter:",
		 PGA_FATAL, PGA_INT, (void *) &maxiter );
    else
	ctx->ga.MaxIter = maxiter;
    
    PGADebugExited("PGASetMaxGAIterValue");
}

/*U***************************************************************************
   PGAGetMaxGAIterValue - Returns the maximum number of iterations to run

   Category: Generation

   Inputs:
      ctx - context variable

   Outputs:
      The maximum number of iterations to run

   Example:
      PGAContext *ctx;
      int maxiter;
      :
      maxiter = PGAGetMaxGAIterValue(ctx);

***************************************************************************U*/
int PGAGetMaxGAIterValue (PGAContext *ctx)
{
    PGADebugEntered("PGAGetMaxGAIterValue");
    PGAFailIfNotSetUp("PGAGetMaxGAIterValue");

    PGADebugExited("PGAGetMaxGAIterValue");

    return(ctx->ga.MaxIter);
}

/*U****************************************************************************
   PGASetMaxNoChangeValue - specifiy maximum number of iterations of no change
   in the evaluation function value of the best string before stopping.  The
   default value is 50.  The stopping rule PGA_STOP_NOCHANGE must have been
   set by PGASetStoppingRuleType for this function call to have any effect.

   Category: Generation

   Inputs:
      ctx     - context variable
      maxiter - the maximum number of GA iterations allowed with no change
                in the best evaluation function value.

   Outputs:
      None

   Example:
      PGAContext *ctx;
      :
      PGASetMaxGAIterValue(ctx,5000);

****************************************************************************U*/
void PGASetMaxNoChangeValue(PGAContext *ctx, int max_no_change)
{
    PGADebugEntered("PGASetMaxNoChangeValue");
    PGAFailIfSetUp("PGASetMaxNoChangeValue");

    if (max_no_change <= 0)
	PGAError(ctx, "PGASetMaxNoChangeValue: max_no_change invalid",
		 PGA_FATAL, PGA_INT, (void *)&max_no_change);
    
    ctx->ga.MaxNoChange = max_no_change;
    
    PGADebugExited("PGASetMaxNoChangeValue");
}

/*U****************************************************************************
   PGASetMaxSimilarityValue - Specifiy the maximum percent of homogeneity of
   the population before stopping.  The similarity measure is the same
   evaluation function value.  The default value is 95 percent.  The stopping
   rule PGA_STOP_TOOSIMILAR must have been set by PGASetStoppingRuleType for
   this function call to have any effect.

   Category: Generation

   Inputs:
      ctx            - context variable
      max_similarity - the maximum percent of the population that can share
                       the same evaluation function value

   Outputs:
      None

   Example:
      PGAContext *ctx;
      :
      PGASetMaxSimilarityValue(ctx,99);

****************************************************************************U*/
void PGASetMaxSimilarityValue(PGAContext *ctx, int max_similarity)
{
    PGADebugEntered("PGASetMaxSimilarityValue");
    PGAFailIfSetUp("PGASetMaxSimilarityValue");

    if ((max_similarity <= 0) || (max_similarity > 100))
        PGAError(ctx, "PGASetMaxSimilarityValue: max_similarity invalid",
                 PGA_FATAL, PGA_INT, (void *) &max_similarity);
    
    ctx->ga.MaxSimilarity = max_similarity;

    PGADebugExited("PGASetMaxSimilarityValue");
}

void PGASetAbortVar(int *abortPtr)
{
    pgapack_abortPtr = abortPtr;
}


/*U****************************************************************************
   PGASetTgtFitnessVal 

   Category: Initialization

   Inputs:
      ctx         - context variable
      tgt_fitness_val - tgt fitness required for stopping. either target average or best fitness depending 
      				on stoppage criteria.
      				Requires setting fitness tolerance using PGASetFitnessTol in order to specify
      				what % difeerence between current and target fitness is acceptable. 

   Outputs:
      None

   Example:
      PGAContext *ctx;
      :
      PGASetTgtFitnessVal(ctx,0);

****************************************************************************U*/
void PGASetTgtFitnessVal(PGAContext *ctx, double tgt_fitness_val){
	PGADebugEntered("PGASetTgtFitnessVal");
    PGAFailIfSetUp("PGASetTgtFitnessVal");
	ctx->ga.TgtFitnessVal = tgt_fitness_val;
	PGADebugExited("PGASetTgtFitnessVal");
}

/*U****************************************************************************
   PGASetFitnessTol 

   Category: Initialization

   Inputs:
      ctx - context variable
      fitness_tol - tolerance between current pop fitness (avg or best, depending on stoppage criteria)
      		target fitness set using PGASetTgtFitnessVal. once this tolerance is achieved, the GA can be stopped.

   Outputs:
      None

   Example:
      PGAContext *ctx;
      :
      PGASetFitnessTol(ctx,0);

****************************************************************************U*/
void PGASetFitnessTol(PGAContext *ctx,double fitness_tol){
	PGADebugEntered("PGASetFitnessTol");
    PGAFailIfSetUp("PGASetFitnessTol");
	ctx->ga.FitnessTol = fitness_tol;
	PGADebugExited("PGASetFitnessTol");
}


/*U****************************************************************************
   PGASetTgtFitnessVariance 

   Category: Initialization

   Inputs:
      ctx - context variable
      tgt_fitness_var - tgt fitness variance required for stopping. 
      				Requires setting fitness variance tolerance using PGASetVarTol in order to specify
      				what % difeerence between current and target fitness variance is acceptable.
   Outputs:
      None

   Example:
      PGAContext *ctx;
      :
      PGASetTgtFitnessVariance(ctx,0);

****************************************************************************U*/
void PGASetTgtFitnessVariance(PGAContext *ctx,double tgt_fitness_var){
	PGADebugEntered("PGASetTgtFitnessVariance");
    PGAFailIfSetUp("PGASetTgtFitnessVariance");
	ctx->ga.TgtFitnessVar = tgt_fitness_var;
	PGADebugExited("PGASetTgtFitnessVariance");
}

/*U****************************************************************************
   PGASetVarTol 

   Category: Initialization

   Inputs:
      ctx - context variable
      var_tol - tolerance between current pop fitness variance and target fitness set using PGASetTgtFitnessVariance. 
      			Once this tolerance is achieved, the GA can be stopped.
   Outputs:
      None

   Example:
      PGAContext *ctx;
      :
      PGASetVarTol(ctx,0);

****************************************************************************U*/
void PGASetVarTol(PGAContext *ctx,double var_tol){
	PGADebugEntered("PGASetVarTol");
    PGAFailIfSetUp("PGASetVarTol");
	ctx->ga.VarTol = var_tol;
	PGADebugExited("PGASetVarTol");
}

/*U****************************************************************************
   PGASetTgtElapsedTime 

   Category: Initialization

   Inputs:
      ctx - context variable
      tgt_elapsed_time - stop the GA if stoppage criteria is PGA_STOP_TIMEELAPSED
      					 and time between start of execution and end of current evaluation 
      					 is equal to tgt_elapsed_time.
   Outputs:
      None

   Example:
      PGAContext *ctx;
      :
      PGASetTgtElapsedTime(ctx,0);

****************************************************************************U*/
void PGASetTgtElapsedTime(PGAContext *ctx,double tgt_elapsed_time){
	PGADebugEntered("PGASetTgtElapsedTime");
    PGAFailIfSetUp("PGASetTgtElapsedTime");
	ctx->ga.TgtElapsedTime = tgt_elapsed_time;
	PGADebugExited("PGASetTgtElapsedTime");
}