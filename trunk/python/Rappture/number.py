# ----------------------------------------------------------------------
#  COMPONENT: number
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2005  Purdue Research Foundation, West Lafayette, IN
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
        return (float(s),other)
