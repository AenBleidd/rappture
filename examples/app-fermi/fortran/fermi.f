c ----------------------------------------------------------------------
c  EXAMPLE: Fermi-Dirac function in Fortran.
c
c  This simple example shows how to use Rappture within a simulator
c  written in Fortran.
c ======================================================================
c  AUTHOR:  Michael McLennan, Purdue University
c  AUTHOR:  Derrick Kearney, Purdue University
c  Copyright (c) 2004-2005
c  Purdue Research Foundation, West Lafayette, IN
c ======================================================================
      program fermi
        IMPLICIT NONE

        integer rp_lib, rp_units_convert_dbl, rp_units_add_presets

        integer driver, ok
        double precision T, Ef, kT, Emin, Emax, dE, f, E
        CHARACTER*100 inFile, strVal
        character*40 xy

        call getarg(1,inFile)
        driver = rp_lib(inFile)

        ok = rp_units_add_presets("all")

        call rp_lib_get(driver,
     +        "input.number(temperature).current", strVal)
        ok = rp_units_convert_dbl(strVal,"K",T)

        call rp_lib_get(driver,
     +        "input.number(Ef).current", strVal)
        ok = rp_units_convert_dbl(strVal,"K",Ef)

        kT = 8.61734e-5 * T
        Emin = Ef - 10*kT
        Emax = Ef + 10*kT

        dE = 0.005*(Emax - Emin)

        do 10 E=Emin,Emax,dE
          f = 1.0/(1.0+exp((E-Ef)/kT))
          write(xy,'(E20.12,F13.9,A)') f, E, char(10)
          call rp_lib_put_str(driver,
     +        "output.curve(f12).component.xy", xy, 1)
 10     continue

        call rp_result(driver)
      end program fermi
