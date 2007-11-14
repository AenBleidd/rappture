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
*     FILE: cmdline.c: This file contains routines needed to parse the
*                      command line.
*
*     Authors: David M. Levine, Philip L. Hallstrom, David M. Noelle,
*              Brian P. Walenz
*****************************************************************************/

#include "pgapack.h"

extern char PGAProgram[100];
#define bad_arg(a)    ( ((a)==NULL) || ((*(a)) == '-') )

/*I****************************************************************************
   PGAReadCmdLin - Code that looks at the arguments, recognizes any that
   are for PGAPack, uses the arguments, and removes them from the command
   line args.

   Inputs:
      ctx  - context variable
      argc - address of the count of the number of command line argumen
      argv - array of command line arguments

   Outputs:
      None

   Example:
      void main(int argc, char **argv) {
          PGAContext *ctx;
          :
          PGAReadCmdLine(ctx, &argc, argv);
      }

****************************************************************************I*/
void PGAReadCmdLine( PGAContext *ctx, int *argc, char **argv )
{
     int c;
     char *s, **a;

     
     /* Put name of called program (according to the args) into PGAProgram */
     s = (char *)  strrchr(*argv, '/');
     if (s)
          strcpy(PGAProgram, s + 1);
     else
          strcpy(PGAProgram, *argv);

     /* Set all command line flags (except procgroup) to their defaults */

     /* Move to last argument, so that we can go backwards. */
     a = &argv[*argc - 1];

     /*
      * Loop backwards through arguments, catching the ones that start with
      * '-'.  Backwards is more efficient when you are stripping things out.
      */
     for (c = (*argc); c > 1; c--, a--)
     {
          if (**a != '-')
               continue;

          if ( !strcmp(*a, "-pgadbg") || !strcmp(*a, "-pgadebug") )
          {
               if bad_arg(a[1])
                    PGAUsage(ctx);
#if OPTIMIZE==0
               PGAParseDebugArg( ctx, a[1] );
#endif
               PGAStripArgs(a, argc, &c, 2);
               continue;
          }

          if ( !strcmp(*a, "-pgaversion") )
          {
               PGAStripArgs(a, argc, &c, 1);
               PGAPrintVersionNumber( ctx );
	       PGADestroy(ctx);
	       exit(-1);
          }

          if (!strcmp(*a, "-pgahelp") )
          {
               if (a[1] == NULL)
                    PGAUsage(ctx);
               else
                    if (!strcmp(a[1], "debug"))
                         PGAPrintDebugOptions(ctx);
                    else
                         fprintf(stderr, "Invalid option following"
                                 "-pgahelp.\n");
          }
     }
}

#if OPTIMIZE==0
/*I****************************************************************************
   PGAParseDebugArg - routine to parse debug command line options, and set
   the appropriate debug level (via PGASetDebugLevel).

   Inputs:
      ctx - context variable
      st  - debug command line options

   Outputs:
      None

   Example:
      Internal function.  Called only by PGAReadCmdLine.

****************************************************************************I*/
void PGAParseDebugArg(PGAContext *ctx, char *st)
{
     int           num2index = 0, num1index = 0, index, num1 = 0, num2 = 0, x;
     unsigned long length = strlen(st);
     char          range = 0, num1ch[4], num2ch[4];


     length--;
     for(index=0; index <= length; index++)
     {
          if (!isdigit(st[index]) && st[index] != ',' && st[index] != '-')
               PGAError(ctx, "PGASetDebugLevel: Invalid Debug Value:",
                        PGA_FATAL, PGA_CHAR, (void *) st);
          if (st[index] == '-')
          {
               range = 1;
               num1ch[num1index] = '\0';
               num1 = atoi(num1ch);
               if (num1 < 0 || num1 > PGA_DEBUG_MAXFLAGS)
                    PGAError(ctx,
                             "PGASetDebugLevel: Lower Limit Out of Range:",
                             PGA_FATAL, PGA_INT, (void *) &num1);
               num1index = 0;
          }
          else
          {
               if (isdigit(st[index]))
                    if (range)
                         num2ch[num2index++] = st[index];
                    else
                         num1ch[num1index++] = st[index];
               if (st[index] == ',' || index == length)
               {
                    if (range)
                    {
                         num2ch[num2index] = '\0';
                         num2 = atoi(num2ch);
                         if (num2 < 0 || num2 > PGA_DEBUG_MAXFLAGS)
                              PGAError(ctx,
                                       "PGASetDebugLevel: Upper Limit Out of"
                                       " Range:",
                                       PGA_FATAL, PGA_INT, (void *) &num2);
                         if (num1 <= num2)
                         {
                              for (x = num1; x <= num2; x++)
                              {
                                   if (x == 212)
                                        printf("%s %s\n", num1ch, num2ch);

                                   PGASetDebugLevel(ctx, x);
                              }

                         }
                         else
                              PGAError(ctx,
                                       "PGASetDebugLevel: Lower Limit Exceeds"
                                       "Upper:", PGA_FATAL, PGA_INT,
                                       (void *) &num1);
                         num2index = 0;
                         range = 0;
                    }
                    else
                    {
                         num1ch[num1index] = '\0';
                         num1 = atoi(num1ch);
                         if (num1 < 0 || num1 > PGA_DEBUG_MAXFLAGS)
                              PGAError(ctx, "PGASetDebugLevel: Debug Number"
                                       "Out of Range:", PGA_FATAL, PGA_INT,
                                       (void *) &num1);
                         if (num1 == 212)
                              printf("%s\n", num1ch);

                         PGASetDebugLevel(ctx, num1);
                         num1index = 0;
                    }
               }
          }
     }
}
#endif

/*I****************************************************************************
   PGAStripArgs - code to strip arguments out of command list

   Inputs:
      argc - address of the count of the number of command line arguments
      argv - array of command line arguments
      c    -
      num  -

   Outputs:
      None

   Example:
      Internal function.  Called only by PGAReadCmdLine.

****************************************************************************I*/
void PGAStripArgs(char **argv, int *argc, int *c, int num)
{
    char **a;
    int i;

    /* Strip out the argument. */
    for (a = argv, i = (*c); i <= *argc; i++, a++)
        *a = (*(a + num));
    (*argc) -= num;
}
