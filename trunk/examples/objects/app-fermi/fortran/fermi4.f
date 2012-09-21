c ----------------------------------------------------------------------
c  EXAMPLE: Fermi-Dirac function in Fortran.
c
c  This simple example shows how to use Rappture within a simulator
c  written in Fortran.
c
c ======================================================================
c  AUTHOR:  Michael McLennan, Purdue University
c  AUTHOR:  Derrick Kearney, Purdue University
c  Copyright (c) 2004-2012  HUBzero Foundation, LLC
c
c  See the file "license.terms" for information on usage and
c  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
c ======================================================================

      program fermi
        IMPLICIT NONE

        integer rp_lib, rp_units_convert_dbl

        integer lib, o, progress
        double precision T, Ef, kT, Emin, Emax, dE, f, E
        CHARACTER*100 inFile, strVal
        character*40 xy

        call getarg(1,inFile)

        lib = Rp_LibraryInit()
        call Rp_LibraryLoadFile(lib,inFile)
        if (Rp_LibraryError(lib) .ne. 0) then
            o = Rp_LibraryOutcome(lib)
            write(0,*) "Error while opening ",inFile
            stop
        endif

        call rp_lib_get(driver,
     +        "input.number(temperature).current", strVal)
        ok = rp_units_convert_dbl(strVal,"K",T)
        if (ok .ne. 0) then
            write(strVal,*) "Error while converting temperature"
            write(0,*) strVal
            call rp_lib_put_str (driver,"output.log",strVal,0)
            call rp_result(driver)
            stop
        endif

        call rp_lib_get(driver,
     +        "input.number(Ef).current", strVal)
        ok = rp_units_convert_dbl(strVal,"eV",Ef)
        if (ok .ne. 0) then
            write(strVal,*) "Error while converting Ef"
            write(0,*) strVal
            call rp_lib_put_str (driver,"output.log",strVal,0)
            call rp_result(driver)
            stop
        endif

        kT = 8.61734e-5 * T
        Emin = Ef - 10*kT
        Emax = Ef + 10*kT

        dE = 0.005*(Emax - Emin)

c       Label out graph with a title, x-axis label, 
c       y-axis label and y-axis units

        call rp_lib_put_str (driver,"output.curve(f12).about.label",
     +          "Fermi-Dirac Factor",0)
        call rp_lib_put_str (driver,"output.curve(f12).xaxis.label",
     +          "Fermi-Dirac Factor",0)
        call rp_lib_put_str (driver,"output.curve(f12).yaxis.label",
     +          "Energy",0)
        call rp_lib_put_str (driver,"output.curve(f12).yaxis.units",
     +          "eV",0)

        do 10 E=Emin,Emax,dE
          f = 1.0/(1.0+exp((E-Ef)/kT))
          progress = nint((E-Emin)/(Emax-Emin)*100)
          call rp_utils_progress (progress,"Iterating")
          write(xy,'(E20.12,F13.9,A)') f, E, char(10)
          call rp_lib_put_str (driver,
     +          "output.curve(f12).component.xy", xy, 1)
 10     continue

        call rp_result(driver)
      end program fermi
