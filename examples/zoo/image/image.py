# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <image> elements
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
# sys.stdout = open('image.out', 'w')
# sys.stderr = open('image.err', 'w')


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

angle = rx['input.(angle).current'].value
angle = float(Rappture.Units.convert(angle, to='deg', units='off'))

rx['output.image(outi).about.label'] = "Rotated Image"

data = rotate_image(data, angle)

# Image data must be B64 or ZB64 encoded.
# The next two lines are equivalent. Can use RPENC_B64 or RPENC_ZB64
rx['output.image(outi).current'] = encode(data, RPENC_ZB64)
#rx.put('output.image(outi).current', data, compress=True)

# add a little html note
htmltext = "html://<p style=\"text-align: center;\"><a href=\"angles.html\">Learn more about angles...</a></p>"
rx['output.image(outi).note.contents'] = htmltext

# save the updated XML describing the run...
rx.close()
