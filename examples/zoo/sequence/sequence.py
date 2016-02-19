# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <dequence> elements
# ======================================================================
#  AUTHOR:  Martin Hunt, Purdue University
#  Copyright (c) 2015  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

import Rappture
import sys
from PIL import Image
from io import BytesIO
from Rappture import RPENC_B64, RPENC_ZB64
from Rappture.encoding import decode, encode

# uncomment these for debugging
# sys.stdout = open('seq.out', 'w')
# sys.stderr = open('seq.err', 'w')


# rotate image data
def rotate_image(data, angle):

    # bug workaround in some PIL versions
    def fileno():
        raise AttributeError

    # open image from data and rotate
    image = Image.open(BytesIO(data))
    rot = image.rotate(angle, expand=True)
    # save image to a file in memory
    memfile = BytesIO()
    memfile.fileno = fileno  # required in some broken PILs
    rot.save(memfile, image.format)
    return memfile.getvalue()


# open the XML file containing the run parameters
rx = Rappture.PyXml(sys.argv[1])

data = rx['input.image.current'].value
# image data in B64 encoded in the xml
data = decode(data, RPENC_B64)

nframes = int(rx['input.integer(nframes).current'].value)

image = Image.open(BytesIO(data))

outs = rx['output.sequence(outs)']
outs['about.label'] = 'Animated Sequence'
outs['index.label'] = 'Frame'

for i in range(nframes):
    element = outs['element(%s)' % i]
    element['index'] = i
    element['about.label'] = 'Frame %s' % i
    rdata = rotate_image(data, i*360.0/nframes)

    # Image data must be B64 or ZB64 encoded.
    # The next two lines are equivalent. Can use RPENC_B64 or RPENC_ZB64
    element['image.current'] = encode(rdata, RPENC_ZB64)
    # element.put('image.current', rdata, compress=True)

# save the updated XML describing the run...
rx.close()
