/* ----------------------------------------------------------------------
 *  EXAMPLE: Fermi-Dirac function in C.
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <stdio.h>
#include <math.h>

int main(int argc, char *argv[]) {
    double T, Ef, E, dE, kT, Emin, Emax, f;

    printf("Enter the Fermi energy in eV:\n");
    scanf("%lg", &Ef);
    printf("Enter the Temperature in K:\n");
    scanf("%lg", &T);

    kT = 8.61734e-5 * T;
    Emin = Ef - 10*kT;
    Emax = Ef + 10*kT;

    E = Emin;
    dE = 0.005*(Emax-Emin);

    while (E < Emax) {
        f = 1.0/(1.0 + exp((E - Ef)/kT));
        printf("%f %f\n",f, E);
        E = E + dE;
    }
    return 0;
}
