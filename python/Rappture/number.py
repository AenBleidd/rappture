# ----------------------------------------------------------------------
#  COMPONENT: number
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2005  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
import Rappture

class number:
    def __init__(self,path,**kwargs):
        """
        Blah, blah...
        """

        self.path = path
        self.attribs = kwargs

    def put(self,lib):
        lib.put(self.path)
        for key,val in self.attribs.items():
            lib.put(self.path+'.'+key, str(val))

    def __coerce__(self,other):
        s = Rappture.driver.get(self.path+'.current')
        return (float(s),other)
