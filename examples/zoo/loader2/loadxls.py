import Rappture
import sys
import os
import pandas as pd
import tempfile

# The name of the xls file that holds the data to plot
xlspath = ""

# Flag to tell us whether we should cleanup the file
# whose name is stored in xlspath.
cleanup = False

# Open the XML file containing the run parameters
rx = Rappture.PyXml(sys.argv[1])

# The user chose a data file to process by picking an
# option from the loader. Each option from the loader
# will store a different value into the hidden string
# "input.string(datafile)". Pull the value out of the
# hidden string and 
datafile = rx['input.string(datafile).current'].value

# Check if the user uploaded their own data file or chose
# one of our examples from the loader. Examples from the
# loader will be a file name starting with the prefix
# "file://". Uploaded files will be a non-ascii blob
# of binary.
#
# If the first seven bytes of the value we
# pulled out of the driver file start with "file://",
# then we can assume characters 7:end are the name of
# of the xls file we can read from disk.
#
# Else, assume the value pulled out of the driver file
# is a non-ascii binary blob of xls data and write it
# to disk (in a temporary file). Store the filename in
# our xlspath variable.

if datafile[0:7] == "file://":
    # first seven bytes match, store the file name
    xlspath = datafile[7:]
else :
    # datafile hold the data from the uploaded xml file
    # write the data from datafile to disk
    tmp_fd,tmp_fn = tempfile.mkstemp()
    os.write(tmp_fd,datafile)
    os.close(tmp_fd)
    xlspath = tmp_fn
    cleanup = True

# read the xls file into a pandas dataframe
xlsdata = pd.ExcelFile(xlspath)
df = xlsdata.parse('Sheet1')

# create a output curve object to plot the data in the xls file
curve = rx['output.curve(1)']
curve['about.label'] = "xls file"
curve['about.description'] = "xls file data plotted as an xy curve"
curve['xaxis.label'] = "X axis"
curve['xaxis.description'] = "X axis from the provided data file"
curve['yaxis.label'] = "Y axis"
curve['yaxis.description'] = "Y axis from the provided data file"
# save the 'x' and 'y' columns from the xls data in a Rappture Curve
curve['component.xy'] = (df['x'], df['y'])

# cleanup the temp file from user uploaded data
if cleanup is True:
    os.remove(xlspath)

# save the updated XML describing the run...
rx.close()
