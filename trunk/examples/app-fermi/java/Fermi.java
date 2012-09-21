/*
 * EXAMPLE: Fermi-Dirac function in Java.
 *
 * This simple example shows how to use Rappture within a simulator
 * written in Java.
 * ======================================================================
 * AUTHOR:  Ben Rafferty, Purdue University
 * Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

class Fermi{
  
  public static void main(String args[]){

    // create a rappture library from the file path
    rappture.Library driver = new rappture.Library(args[0]);

    // read the input values and convert to correct units
    String Tstr = driver.get("input.number(temperature).current");
    double T = Double.valueOf(rappture.Units.convertString(Tstr, "K", false));
    String Efstr = driver.get("input.number(Ef).current");
    double Ef = Double.valueOf(rappture.Units.convertString(Efstr, "eV", false));

    // Set the energy range and step size
    double kT = 8.61734E-5 * T;
    double Emin = Ef - 10*kT;
    double Emax = Ef + 10*kT;
    double E = Emin;
    double dE = 0.005*(Emax-Emin);

    // Add relevant information to the output curve
    driver.put("output.curve(f12).about.label", "Fermi-Dirac Factor");
    driver.put("output.curve(f12).xaxis.label", "Fermi-Dirac Factor");
    driver.put("output.curve(f12).yaxis.label", "Energy");
    driver.put("output.curve(f12).yaxis.units", "eV");

    // Calculate the Fermi-Dirac function over the energy range
    // Add the results to the output curve
    double f;
    String line;
    while (E < Emax){
      f = 1.0/(1.0 + Math.exp((E - Ef)/kT));
      line = String.format("%f %f\n", f, E);
      rappture.Utils.progress((int)((E-Emin)/(Emax-Emin)*100), "Iterating");
      driver.put("output.curve(f12).component.xy", line, true);
      E = E + dE;
    } 

    // Finalize the results and inform rappture that the simulation has ended
    driver.result();
  }

}

