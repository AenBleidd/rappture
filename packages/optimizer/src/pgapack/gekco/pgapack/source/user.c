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
*     FILE: user.c: This file contains a function to set user functions.
*
*     Authors: David M. Levine, Philip L. Hallstrom, David M. Noelle,
*              Brian P. Walenz
*****************************************************************************/

#include "pgapack.h"

/*U****************************************************************************
   PGASetUserFunction - specifies the name of a user-written function
   call to provide a specific GA capability (e.g., crossover,
   mutation, etc.).  This function MUST be used when using a non-native
   datatype and must be called once for each of:
       PGA_USERFUNCTION_CREATESTRING     -- String creation
       PGA_USERFUNCTION_MUTATION         -- Mutation
       PGA_USERFUNCTION_CROSSOVER        -- Crossover
       PGA_USERFUNCTION_PRINTSTRING      -- String Output
       PGA_USERFUNCTION_COPYSTRING       -- Duplication
       PGA_USERFUNCTION_DUPLICATE        -- Duplicate Checking
       PGA_USERFUNCTION_INITSTRING       -- Initialization
       PGA_USERFUNCTION_BUILDDATATYPE    -- MPI Datatype creation
       PGA_USERFUNCTION_STOPCOND         -- Stopping conditions
       PGA_USERFUNCTION_ENDOFGEN         -- Auxiliary functions at the end
                                            of each generation
   It MAY be called when using a native datatype to replace the built-in
   functions PGAPack has for that datatype (For example, if the Integer data
   type is used for a traveling salesperson problem, the user may want to
   provide their own custom crossover operator).  See the user guide and the
   examples in the examples directory for more details.

   Category: Generation

   Inputs:
      ctx      - context variable
      constant - symbolic constant of the user function to set
      f        - name of the function to use

   Outputs:
      None

   Example:
      void MyStringInit(PGAContext *, void *);
      PGAContext *ctx;
      :
      PGASetUserFunction(ctx, PGA_USERFUNCTION_INITSTRING, MyStringInit);

****************************************************************************U*/
void PGASetUserFunction(PGAContext *ctx, int constant, void *f)
{
    PGADebugEntered("PGASetUserFunction");

    if (f == NULL)
	PGAError ( ctx, "PGASetUserFunction: Invalid function",
		  PGA_FATAL, PGA_VOID, NULL);

    switch (constant) {
      case PGA_USERFUNCTION_CREATESTRING:
	if (ctx->sys.UserFortran) 
	    PGAError(ctx, "PGASetUserFunction: Cannot call "
		     "PGA_USERFUNCTION_CREATESTRING from Fortran.",
		     PGA_FATAL, PGA_VOID, NULL);
	else
	    ctx->cops.CreateString = (void(*)(PGAContext *, int, int, int))f;
	break;
      case PGA_USERFUNCTION_MUTATION:
	if (ctx->sys.UserFortran) 
	    ctx->fops.Mutation = (int(*)(void *, void *, void *, void *))f;
	else
	    ctx->cops.Mutation = (int(*)(PGAContext *, int, int, double))f;
	break;
      case PGA_USERFUNCTION_CROSSOVER:
	if (ctx->sys.UserFortran) 
	    ctx->fops.Crossover = (void(*)(void *, void *, void *, void *, void *, void *, void *))f;
	else
	    ctx->cops.Crossover =  (void(*)(PGAContext *, int, int, int, int, int, int))f;
	break;
      case PGA_USERFUNCTION_PRINTSTRING:
	if (ctx->sys.UserFortran) 
	    ctx->fops.PrintString = (void(*)(void *, void *, void *, void *))f;
	else
	    ctx->cops.PrintString =  (void(*)(PGAContext *, FILE *, int, int))f;
	break;
      case PGA_USERFUNCTION_COPYSTRING:
	if (ctx->sys.UserFortran) 
	    PGAError(ctx, "PGASetUserFunction: Cannot call "
		     "PGA_USERFUNCTION_COPYSTRING from Fortran.",
		     PGA_FATAL, PGA_VOID, NULL);
	else
	    ctx->cops.CopyString = (void(*)(PGAContext *, int, int, int, int))f;
	break;
      case PGA_USERFUNCTION_DUPLICATE:
	if (ctx->sys.UserFortran) 
	    ctx->fops.Duplicate = (int(*)(void *, void *, void *, void *, void *))f;
	else
	    ctx->cops.Duplicate = (int(*)(PGAContext *, int, int, int, int))f;
	break;
      case PGA_USERFUNCTION_INITSTRING:
	if (ctx->sys.UserFortran) 
	    ctx->fops.InitString = (void(*)(void *, void *, void *))f;
	else
	    ctx->cops.InitString = (void(*)(PGAContext *, int, int))f;
	break;
      case PGA_USERFUNCTION_BUILDDATATYPE:
	if (ctx->sys.UserFortran) 
	    PGAError(ctx, "PGASetUserFunction: Cannot call "
		     "PGA_USERFUNCTION_BUILDDATATYPE from Fortran.",
		     PGA_FATAL, PGA_VOID, NULL);
	else
	    ctx->cops.BuildDatatype = (MPI_Datatype(*)(PGAContext *, int, int))f;
	break;
      case PGA_USERFUNCTION_STOPCOND:
	if (ctx->sys.UserFortran) 
	    ctx->fops.StopCond = (int(*)(void *))f;
	else
	    ctx->cops.StopCond = (int(*)(PGAContext *))f;
	break;
      case PGA_USERFUNCTION_ENDOFGEN:
	if (ctx->sys.UserFortran) 
	    ctx->fops.EndOfGen = (void(*)(void *))f;
	else
	    ctx->cops.EndOfGen = (void(*)(PGAContext *))f;
	break;
      case PGA_USERFUNCTION_RANKING:
        if (ctx->sys.UserFortran)
            ctx->fops.Rank = (void(*)(void *, void *))f;
        else
            ctx->cops.Rank = (void(*)(PGAContext *, int))f;
        break;
      case PGA_USERFUNCTION_FITNESS:
        if (ctx->sys.UserFortran)
            ctx->fops.Fitness = (void(*)(void *, void*))f;
        else
            ctx->cops.Fitness = (void(*)(PGAContext *, int))f;
        break;
      case PGA_USERFUNCTION_SELECT:
        if (ctx->sys.UserFortran)
            ctx->fops.Select = (void(*)(void *, void*))f;
        else
            ctx->cops.Select = (void(*)(PGAContext *, int))f;
        break;
/*********************************************/
/* new added by Lu Yang */
      case PGA_USERFUNCTION_SORTPOP:
	if(ctx->sys.UserFortran)
	    ctx->fops.SortPop = (void(*)(void *,void *))f;
	else
	    ctx->cops.SortPop = (void(*)(PGAContext *,int))f;
	break;
/*********************************************/
      case PGA_USERFUNCTION_SETTEMP:
	if (ctx->sys.UserFortran) 
	    ctx->fops.SetTemp = (void(*)(void *))f;
	else
	    ctx->cops.SetTemp = (void(*)(PGAContext *))f;
	break;
      case PGA_USERFUNCTION_ENDOFREPFCN:
	if(ctx->sys.UserFortran)
	    ctx->fops.EndOfRepFcn = (void(*)(void *,void *,void *))f;
	else
	    ctx->cops.EndOfRepFcn = (void(*)(PGAContext *,int ,int))f;
	break;
	    
 
     case PGA_USERFUNCTION_INITUSERCTX:
         if (ctx->sys.UserFortran) 
           ctx->fops.InitUserCtx = (void(*)(void *))f;
         else 
           ctx->cops.InitUserCtx = (void(*)(PGAContext *))f;
         break;
 
      case PGA_USERFUNCTION_KILLUSERCTX:
       if (ctx->sys.UserFortran) 
           ctx->fops.KillUserCtx = (void(*)(void *))f;
       else
           ctx->cops.KillUserCtx = (void(*)(PGAContext *))f;
       break;
 



      case PGA_USERFUNCTION_DESTROYUD:
	if(ctx->sys.UserFortran)
	    ctx->fops.DestroyUD  = (void(*)(void *,void *,void *))f;
	else
	    ctx->cops.DestroyUD  = (void(*)(PGAContext *,int ,int))f;
	break;

      default:
	PGAError(ctx, "PGASetUserFunction: Invalid constant:",
		 PGA_FATAL, PGA_INT, (void *) &constant);
	break;
    }

    PGADebugExited("PGASetUserFunction");
}

/**************************************************************************/
/*       EHW project Modification ( ADDITION OF FUNCTION )                */
/*       This function returns the size of the user data vector in all    */
/*       individual's chromosome vector                                   */
/**************************************************************************/
int PGAGetSizeUD(PGAContext *ctx)
{
  return ctx->ga.sUD ;
}

/**************************************************************************/
/*       EHW project Modification ( ADDITION OF FUNCTION )                */
/*       This function sets the size of the user data vector in all       */
/*       individual's chromosome vector                                   */
/**************************************************************************/
void PGASetSizeUD(PGAContext *ctx,int s)
{
  ctx->ga.sUD = s;
}

/**************************************************************************/
/*       EHW project Modification ( ADDITION OF FUNCTION )                */
/*       This function gets the user data vector in all                   */
/*       individual's chromosome vector                                   */
/**************************************************************************/
void *PGAGetUD(PGAContext *ctx,int s,int pop)
{
  return PGAGetIndividual(ctx,s,pop)->UserData;
}
