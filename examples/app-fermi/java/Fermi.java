
class Fermi{
  
  public static void main(String args[]){

    rappture.Library driver = new rappture.Library(args[0]);

    String Tstr = driver.get("input.number(temperature).current");
    double T = Double.valueOf(rappture.Units.convert(Tstr, "K", false));
    
    String Efstr = driver.get("input.number(Ef).current");
    double Ef = Double.valueOf(rappture.Units.convert(Efstr, "eV", false));

    double kT = 8.61734E-5 * T;
    double Emin = Ef - 10*kT;
    double Emax = Ef + 10*kT;

    double E = Emin;
    double dE = 0.005*(Emax-Emin);

    double f;
    String line;

    driver.put("output.curve(f12).about.label", "Fermi-Dirac Factor");
    driver.put("output.curve(f12).xaxis.label", "Fermi-Dirac Factor");
    driver.put("output.curve(f12).yaxis.label", "Energy");
    driver.put("output.curve(f12).yaxis.units", "eV");

    while (E < Emax){
      f = 1.0/(1.0 + Math.exp((E - Ef)/kT));
      line = String.format("%f %f\n", f, E);
      rappture.Utils.progress((int)((E-Emin)/(Emax-Emin)*100), "Iterating");
      driver.put("output.curve(f12).component.xy", line, true);
      E = E + dE;
    } 

    driver.result();
  }

}

