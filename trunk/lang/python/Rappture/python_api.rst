=======================
New Rappture Python API
=======================

This is a new Python API, which extends the old API, improving
readability and providing a more "Pythonic" feel.  It is an extension
to the old API, which is located `here <https://nanohub.org/infrastructure/rappture/wiki/rappture_python_api>`_

Opening an XML File
-------------------

Tools written in Python will be typically called with the name of the output XML file
as the first command line argument. You will need to open the file with the
PyXml() function.

.. code-block:: python

    import Rappture
    io = Rappture.PyXml(sys.argv[1])

.. note::

    If you get "AttributeError: 'module' object has no attribute 'PyXml'"
    that means you are using an older version of Rappture without the new
    API.

Closing the XML File
--------------------

When your tool is finished, before it exits, the PyXml instance must
call close().

.. code-block:: python

    io.close()


Getting Started
---------------

Here is a simple xml input file we will use for the examples.  Save
it into 'driver.xml'

.. code-block:: xml

    <run>
        <input>
            <number id="Ef">
                <units>eV</units>
                <min>-10eV</min>
                <max>10eV</max>
                <default>0eV</default>
                <current>3eV</current>
            </number>
        </input>
    </run>

Now save this code into a file 'my_tool.py'

.. code-block:: python

    import Rappture
    import sys

    rx = Rappture.PyXml(sys.argv[1])
    print rx['input.number(Ef).current'].value
    rx.close()

Running it, we get::

    > python my_tool.py driver.xml
    3eV
    =RAPPTURE-RUN=>run1431956733728770.xml

The last line came from Rappture.  It is the output xml filename. This file
is the driver file, plus any changes your tool made (in this case, none) and
some additional information about the run, including username, hostname, and time.

Reading Values
--------------

The previous example showed the simplest case of reading from the driver file.
The driver file is in XML format.  Each node in an XML file can contain either
information or other nodes.  Nodes are specified by their path relative to the
root or other nodes.  If you try this:

.. code-block:: python

    print io['input.number(Ef).current']

You will get a node reference, which looks something like::

    <Rappture.library object at 0x7fa9c3c3e168>['input.number(Ef).current']

If you want the value at that path, you must ask for it by using the "value" method.

.. code-block:: python

    print io['input.number(Ef).current'].value

Outputs::

    3eV

Nodes are very useful because the allow us to use shorter, more readable
path names.  For example if you want to read all the information about
variable 'Ef' from the driver file:

.. code-block:: python

    ef = io['input.number(Ef)']
    print ef['units'].value
    print ef['min'].value
    print ef['max'].value
    print ef['default'].value
    print ef['current'].value

Outputs::

    eV
    -10eV
    10eV
    0eV
    3eV

You can also ask for the pathname of the node.

.. code-block:: python

    ef = io['input.number(Ef)']
    print ef['current'].name

::

    input.number(Ef).current


Converting Values
-----------------

All values read from the driver file are strings.  Most will have units.
To use them in your simulation, you will need integers or floating point numbers.
And you must be sure they are in the required units.
We do this by calling Rappture.Units.convert().

.. code-block:: python

    ef = io['input.number(Ef)']
    ef_str = ef['current'].value

    ev = Rappture.Units.convert(ef_str, to="eV", units="off")
    print "ev=", ev

    mev = Rappture.Units.convert(ef_str, to="meV", units="off")
    print "mev=", mev

::

    ev= 3.0
    mev= 3000.0


Writing Values
--------------

As with reading, we use nodes and pathnames.  Values are automatically
converted into apropriate strings.  You can check this by reading the value
or inspecting the generated xml.  The generated xml is available in the
output xml file, or you can call the xml() method.

.. code-block:: python

    h1 = io['output.histogram(example1)']
    h1['about.label'] = 'Name Value Histogram'

    labels = ['Tigers', 'Bears', 'Lions', 'Monkeys', 'Hawks',
              'Elephants', 'Foxes', 'Geckos', 'Zebras', 'Giraffes']
    values = [5, 7, 3, 15, 8, 6, 18, 4, 9, 2]

    h1['component.xy'] = (labels, values)

    # read back the values to see that the (labels, values)
    # tuple has been converted into interleaved strings as required
    # by https://nanohub.org/infrastructure/rappture/wiki/rp_xml_ele_histogram
    #
    print h1['component.xy'].value

    # If we want to see all the generated xml...
    print h1.xml()

::

    "Tigers" 5
    "Bears" 7
    "Lions" 3
    "Monkeys" 15
    "Hawks" 8
    "Elephants" 6
    "Foxes" 18
    "Geckos" 4
    "Zebras" 9
    "Giraffes" 2


And the xml part looks like this:

.. code-block:: xml

   <histogram id="example1">
        <about>
            <label>Name Value Histogram</label>
        </about>
        <component>
            <xy>&quot;Tigers&quot; 5
    &quot;Bears&quot; 7
    &quot;Lions&quot; 3
    &quot;Monkeys&quot; 15
    &quot;Hawks&quot; 8
    &quot;Elephants&quot; 6
    &quot;Foxes&quot; 18
    &quot;Geckos&quot; 4
    &quot;Zebras&quot; 9
    &quot;Giraffes&quot; 2</xy>
        </component>
    </histogram>


In general, ints and floats are converted to strings, and lists, tuples, and arrays are converted into interleaved
strings.  If x and y are lists or arrays, then (x, y) or [x, y] would be inserted into the xml as strings
representing x[0], y[0], x[1], y[1], ...

If the argument is a multidimensional array, it is flattened (serialized)
in the Fortran order, where the first ('x') index changes fastest.  This is
what Rappture normally requires for fields and meshes.

See the rappture examples for more information.

.. note::

    Many of the old examples showed writing values by iterating through lists and appending values
    to the node.  That will still work, although it is extremely slow.  The more efficient,
    and easier way is to write out entire lists or arrays at once.

Debugging
---------

When debugging a tool that is started by the Rappture GUI(instead of running from the command
line as we have in these examples), the Rappture GUI will not display output from prints.  You can
redirect output to files for easier debugging by putting this at the top of your code.

.. code-block:: python

    import sys
    sys.stderr = open('tool.err', 'w')
    sys.stdout = open('tool.out', 'w')



REFERENCE
=========

Opening
-------

- node = Rappture.PyXml(driver_filename)
    *driver_filename*
        The XML driver file.
    *Returns*
        A node reference set to the root (top) of the xml.

Closing
-------

- node.close()
    Writes out the XML.

Reading
-------

- node[path]
    *node*
        A node reference
    *path*
        A string indicating an XML path.
    *Returns*
        A new node reference.

- node[path].value
    *node*
        A node reference
    *path*
        A string indicating an XML path.
    *Returns*
        A string representing the value at the node.

- node[path].name
    *node*
        A node reference
    *path*
        A string indicating an XML path.
    *Returns*
        The xml path as a string.

- node.get(path)
    [Deprecated. For compatibility with the old API.]

    *node*
        A node reference
    *path*
        A string indicating an XML path.
    *Returns*
        A string representing the value at the node.

Writing
-------

- node[path] = value
    Encodes *value* as a string and writes it to the
    xml path *path* relative to *node*.

    *node*
        A node reference
    *path*
        A string indicating an XML path.
    *value*
        Data to write.

- node.put(path, value, append, type, compress)
    Encodes *value* as a string and writes it to the
    xml path *path* relative to *node*. This syntax
    allows some rare additional options.  The previous
    syntax is prefered for readability.

    *node*
        A node reference
    *path*
        A string indicating an XML path.
    *value*
        Data to write.
    *append*
        Append this to the current value at the node, if there
        is one.  Not recommended. It is far more efficient to
        build strings and lists in Python and write them in one
        operation.
    *type*
        'string' (default) or 'file'.  If set to 'file', then *value*
        is a filename and the data from it is copied to the xml node.
    *compress*
        True or False (default). If True, data is gzipped
        and base64 encoded.

Examples:

.. code-block:: python

    rx.put('current', 'results.txt', type='file')
    rx.put('current', 'results.txt', type='file', compress=True)


Progress Bar
------------

- Rappture.Utils.progress(percent, message)
    Updates the Rappture GUI progress bar.

    *percent*
        Int or float from 0 to 100 indicating completion percentage.
    *message*
        String. Message to display.


Encoding/Decoding
-----------------

Some special tools may require manipulation of external binary data.


.. code-block:: python

    from Rappture import PyXml, RPENC_Z, RPENC_B64, RPENC_ZB64
    from Rappture.encoding import encode, decode, isbinary

    # string with embedded non-ascii char
    instr = "foobar\n012\0"

    # compress and uncompress, for testing
    x = encode(instr, RPENC_Z)
    outstr = decode(x, 0)

    # check
    assert instr == outstr
    assert isbinary(instr) == True
