# ----------------------------------------------------------------------
#  COMPONENT: interface - used in the main program of a simulator
#
#  Rappture.interface() is called at the start of a simulator.  You pass
#  in arguments and one or more interface packages.  If the -gui flag
#  is included, then the interfaces are loaded and interrogated, and
#  interface() writes out a tool.xml file, then invokes the Rappture GUI
#  to collect input.  The Rappture GUI writes out a driver.xml file,
#  and then invokes this tool again with the -driver flag to process
#  the input.
#  
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
import getopt, sys, os
import atexit
import Rappture

def interface(argv,*args):
    """
    Clients call this near the top of their simulator.  It looks
    at command line arguments and decides whether the Rappture GUI
    should be invoked, or perhaps the GUI has already been invoked
    and the simulator should process a driver file.

    The argv is a list of command-line arguments.  All remaining
    arguments represent packages that should be imported and
    interrogated for Rappture components, to feed the GUI.
    """

    try:
        opts,rest = getopt.getopt(argv[1:],'d:g',["driver","gui"])
    except getopt.GetoptError:
        print 'usage:'
        print '  %s --gui (launch the graphical interface)' % argv[0]
        print '  %s --driver file (use the input stored in file)' % argv[0]
        sys.exit(2)

    for opt,val in opts:
        if opt in ('-g','--gui'):
            #
            # Launch the GUI.  Start by loading the various modules
            # and interrogating them for symbols.
            #
            lib = Rappture.library('<?xml version="1.0"?><run><tool>' +
                    '<about>Press Simulate to view results.</about>' +
                    '<command>python ' +argv[0]+ ' -d @driver</command></tool></run>')

            for module in args:
                for symbol in dir(module):
                    s = module.__dict__[symbol]
                    if isinstance(s, Rappture.number):
                        s.put(lib)

            toolFileName = "tool.xml"
            f = open(toolFileName,"w")
            f.write( lib.xml() )
            f.close()

            print 'launch the gui...'
            #
            # this doesnt work, but needs to. when we 
            # os.execvp('driver', driverArgs ) with following definition of 
            # driverArgs (and we change the name of toolFileName), program
            # does not send the driver program the arguments as expected.
            #
            #driverArgs = ('-tool', toolFileName)
            driverArgs = ()
            os.execvp('driver', driverArgs )
            sys.exit(0)

        elif opt in ('-d','--driver'):
            #
            # Continue, using input from the driver file.
            # Register a hook to write out results upon exit.
            #
            Rappture.driver = Rappture.library(val)

            atexit.register(finish)
            return


def finish():
    """
    Called automatically whenever the program exits.  Writes out
    any results in the global driver to the "run.xml" file
    representing the entire run.
    """
#    f = open("run.xml","w")
#    f.write( Rappture.driver.xml() )
#    f.close()

    Rappture.result(Rappture.driver)
