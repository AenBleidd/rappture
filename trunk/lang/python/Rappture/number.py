# ----------------------------------------------------------------------
#  COMPONENT: number
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
import Rappture

class number:
    def __init__(self,path,id,**kwargs):
        """
        Blah, blah...
        """

        self.basePath = path
        self.path = path + ".number(" + id + ")"
        self.id = id
        self.attribs = kwargs

    def put(self,lib):
        lib.put(self.path)
        for key,val in self.attribs.items():
            lib.put(self.path+'.'+key, str(val))

    def __coerce__(self,other):
        attrList = Rappture.driver.children(self.path, "id")
        attrName = 'default'
        for attr in attrList:
            if attr == 'current':
                attrName = attr
        s = Rappture.driver.get(self.path+'.'+attrName)
        # return (float(s),other)
        #
        # the following code doesnt work if the <units> tag does not exist
        # becasue it sends an empty string to convert and it fails there
        toUnit = Rappture.driver.get(self.path+'.units')
        if (toUnit != None):
            val = Rappture.Units.convert(s,to=toUnit,units="off")
        else :
            # you have to hope the user didn't put units onto the <default>
            # value (which is unlikely in most cases, btw)
            val = float(s)
        return (val,other)
