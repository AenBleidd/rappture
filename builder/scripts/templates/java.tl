# ----------------------------------------------------------------------
#  LANGUAGE:  Java API
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
language "Java" {
  extension ".java"
  command {java RpApp @driver}

  main {// ---------------------------------------------------------------------
//  MAIN PROGRAM - generated by the Rappture Builder
// ---------------------------------------------------------------------

class RpApp {

  public static void main(String args[]) {
    String str;

    // open the XML file containing the run parameters
    rappture.Library io = new rappture.Library(args[0]);

    /////////////////////////////////////////////////////////
    // Get input values from Rappture
    /////////////////////////////////////////////////////////
    @@INPUTS@@

    /////////////////////////////////////////////////////////
    //  Add your code here for the main body of your program
    /////////////////////////////////////////////////////////

    // spit out progress messages as you go along...
    rappture.Utils.progress(0, "Starting...");
    rappture.Utils.progress(5, "Loading data...");
    rappture.Utils.progress(50, "Half-way there");
    rappture.Utils.progress(100, "Done");

    /////////////////////////////////////////////////////////
    // Save output values back to Rappture
    /////////////////////////////////////////////////////////
    @@OUTPUTS@@

    io.result();
  }
}}

  input boolean {
    code "\n// get input value for $path"
    code "str = io.get(\"$path.current\");"
    code "boolean $id = str.equals(\"yes\");"
  }
  input image {
    code "\n// get input value for $path"
    code "// returns base64-encoded image data"
    code "$id = io.get(\"$path.current\");"
  }
  input integer {
    code "\n// get input value for $path"
    code "int $id = io.getInt(\"$path.current\");"
  }
  input number {
    set units [attr get units]
    if {$units ne ""} {
      code "\n// get input value for $path and convert to $units"
      code "str = io.get(\"$path.current\");"
      code "double $id = Double.valueOf(rappture.Units.convertString(str, \"$units\", false));"
    } else {
      code "\n// get input value for $path"
      code "double $id = io.getDouble(\"$path.current\");"
    }
  }
  input * {
    code "\n// get input value for $path"
    code "String $id = io.get(\"$path.current\");"
  }

  output boolean {
    code "\n// save output value for $path"
    code "io.put(\"$path.component.xy\", ($id) ? \"yes\" : \"no\");"
  }
  output curve {
    code "\n// save output value for $path"
    code "// this shows just one (x,y) point -- modify as needed"
    code "str = String.format(\"%g %g\\n\", x, y);"
    code "io.put(\"$path.component.xy\", line, true);"
  }
  output histogram {
    code "\n// save output value for $path"
    code "// this shows just one point -- modify as needed"
    code "// (x,h,w) = x-coord, height of bar, and width of bar (optional)"
    code "str = String.format(\"%g %g %g\\n\", x, h, w);"
    code "io.put(\"$path.component.xhw\", line, true);"
  }
  output image {
    code "\n// save output value for $path"
    code "// data should be base64-encoded image data"
    code "io.put(\"$path.current\",$id);"
  }
  output integer {
    code "\n// save output value for $path"
    code "io.put(\"$path.current\", Integer.toString($id));"
  }
  output number {
    code "\n// save output value for $path"
    code "io.put(\"$path.current\", Double.toString($id));"
  }
  output * {
    code "\n// save output value for $path"
    code "io.put(\"$path.current\",$id);"
  }

  makefile {# simple makefile for a Rappture-based program

JAVAC		= javac

RAPPTURE_DIR	= @@RAPPTUREDIR@@
CLASSPATH	= -classpath $(RAPPTURE_DIR)/lib/java


all: @@FILEROOT@@.class

@@FILEROOT@@.class: @@FILENAME@@
	$(JAVAC) $(CLASSPATH) $<

install: 

clean:
	$(RM) @@FILEROOT@@.class run*.xml
}
}
