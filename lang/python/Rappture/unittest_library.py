# ----------------------------------------------------------------------
#  COMPONENT: library unit test - provides tests for Rappture Library
#
#  Unit tests for the python bindings of Rappture's Library module
# ======================================================================
#  AUTHOR:  Derrick S. Kearney, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

import unittest
import Rappture

class CopyTests(unittest.TestCase):

    def setUp(self):
        self.lib  = Rappture.library("test.xml")
        self.lib2 = Rappture.library("test.xml")
        # self.lib3 = Rappture.library()

    def tearDown(self):
        return

    # def testArguments_UninitializedLib(self):
    #     print self.lib3.xml()
    #     self.assertRaises(RuntimeError,self.lib.copy,"input","output",self.lib3)

    def testArguments_TooMaryArgs(self):
        self.assertRaises(TypeError,self.lib.copy,1,2,3,4)

    def testArguments_NotEnoughArgs(self):
        self.assertRaises(TypeError,self.lib.copy,1)

    def testArguments_ArgsWrongType2(self):
        self.assertRaises(TypeError,self.lib.copy,1,2)

    def testArguments_ArgsWrongType3(self):
        self.assertRaises(RuntimeError,self.lib.copy,"input","output",3)

    def testArguments_ToPathRootCopyElement(self):
        self.lib.copy("","input")
        self.assertEqual(self.lib.xml(),'''<?xml version="1.0"?>
<run>
    <string id="test">
        <current>&lt;sdfsfs&gt;sdfs</current>
    </string>
    <time>Thu Mar 15 02:03:39 2007 EDT</time>
    <image>
        <current>@@RP-ENC:b64
R0lGODlhGgASAKEBAAAAAP///////////yH+FUNyZWF0ZWQgd2l0aCBUaGUgR0lNUAAh+QQBCgAB
ACwAAAAAGgASAAACLoyPqcvtD8CRj8VZrYw8h/tRn2eA4Eiaosa1qttC1EmW81qrtbYvdG8DCodE
RQEAOw==</current>
    </image>
</run>
''')

    def testArguments_ToPathRootCopyContents(self):
        self.lib.copy("","input.time")
        self.assertEqual(self.lib.xml(),'''<?xml version="1.0"?>
<run>Thu Mar 15 02:03:39 2007 EDT</run>
''')

    def testArguments_ToPathElementCopyContents(self):
        self.lib.copy("output","input.time")
        self.assertEqual(self.lib.xml(),'''<?xml version="1.0"?>
<run>
    <tool>
        <repository>
            <rappture>
                <date>$Date: 2007-03-08 15:27:02 -0500 (Thu, 08 Mar 2007) $</date>
                <revision>$Rev: 613 $</revision>
                <url>$URL: https://kearneyd@repo.nanohub.org/svn/rappture/trunk/src/core/RpLibrary.cc $</url>
            </rappture>
        </repository>
    </tool>
    <input>
        <string id="test">
            <current>&lt;sdfsfs&gt;sdfs</current>
        </string>
        <time>Thu Mar 15 02:03:39 2007 EDT</time>
        <image>
            <current>@@RP-ENC:b64
R0lGODlhGgASAKEBAAAAAP///////////yH+FUNyZWF0ZWQgd2l0aCBUaGUgR0lNUAAh+QQBCgAB
ACwAAAAAGgASAAACLoyPqcvtD8CRj8VZrYw8h/tRn2eA4Eiaosa1qttC1EmW81qrtbYvdG8DCodE
RQEAOw==</current>
        </image>
    </input>
    <output>Thu Mar 15 02:03:39 2007 EDT</output>
</run>
''')

    def testArguments_ToPathElementCopyElementsFromLib2(self):
        self.lib.copy("lib2input","input",self.lib2)
        self.assertEqual(self.lib.xml(),'''<?xml version="1.0"?>
<run>
    <tool>
        <repository>
            <rappture>
                <date>$Date: 2007-03-08 15:27:02 -0500 (Thu, 08 Mar 2007) $</date>
                <revision>$Rev: 613 $</revision>
                <url>$URL: https://kearneyd@repo.nanohub.org/svn/rappture/trunk/src/core/RpLibrary.cc $</url>
            </rappture>
        </repository>
    </tool>
    <input>
        <string id="test">
            <current>&lt;sdfsfs&gt;sdfs</current>
        </string>
        <time>Thu Mar 15 02:03:39 2007 EDT</time>
        <image>
            <current>@@RP-ENC:b64
R0lGODlhGgASAKEBAAAAAP///////////yH+FUNyZWF0ZWQgd2l0aCBUaGUgR0lNUAAh+QQBCgAB
ACwAAAAAGgASAAACLoyPqcvtD8CRj8VZrYw8h/tRn2eA4Eiaosa1qttC1EmW81qrtbYvdG8DCodE
RQEAOw==</current>
        </image>
    </input>
    <lib2input>
        <string id="test">
            <current>&lt;sdfsfs&gt;sdfs</current>
        </string>
        <time>Thu Mar 15 02:03:39 2007 EDT</time>
        <image>
            <current>@@RP-ENC:b64
R0lGODlhGgASAKEBAAAAAP///////////yH+FUNyZWF0ZWQgd2l0aCBUaGUgR0lNUAAh+QQBCgAB
ACwAAAAAGgASAAACLoyPqcvtD8CRj8VZrYw8h/tRn2eA4Eiaosa1qttC1EmW81qrtbYvdG8DCodE
RQEAOw==</current>
        </image>
    </lib2input>
</run>
''')

#    def testToPathRootCopyElementsFromElementObj(self):
#        e = self.lib.element("input.string",as="object")
#        self.lib.copy("","",e)
#        self.assertEqual(self.lib.xml(),'''<?xml version="1.0"?>
#<run>
#    <current>&lt;sdfsfs&gt;sdfs</current>
#</run>
#''')

#    def testToPathRootIdCopyElementsFromElementObj(self):
#        e = self.lib.element("input.string",as="object")
#        self.lib.copy("(qqq)","",e)
#        self.assertEqual(self.lib.xml(),'''<?xml version="1.0"?>
#<run>
#    <current>&lt;sdfsfs&gt;sdfs</current>
#</run>
#''')

    def testToPathId(self):
        e = self.lib.element("input.string", type="object")
        self.lib.copy("estring(qqq)","",e)
        self.assertEqual(self.lib.xml(),'''<?xml version="1.0"?>
<run>
    <tool>
        <repository>
            <rappture>
                <date>$Date: 2007-03-08 15:27:02 -0500 (Thu, 08 Mar 2007) $</date>
                <revision>$Rev: 613 $</revision>
                <url>$URL: https://kearneyd@repo.nanohub.org/svn/rappture/trunk/src/core/RpLibrary.cc $</url>
            </rappture>
        </repository>
    </tool>
    <input>
        <string id="test">
            <current>&lt;sdfsfs&gt;sdfs</current>
        </string>
        <time>Thu Mar 15 02:03:39 2007 EDT</time>
        <image>
            <current>@@RP-ENC:b64
R0lGODlhGgASAKEBAAAAAP///////////yH+FUNyZWF0ZWQgd2l0aCBUaGUgR0lNUAAh+QQBCgAB
ACwAAAAAGgASAAACLoyPqcvtD8CRj8VZrYw8h/tRn2eA4Eiaosa1qttC1EmW81qrtbYvdG8DCodE
RQEAOw==</current>
        </image>
    </input>
    <estring id="qqq">
        <current>&lt;sdfsfs&gt;sdfs</current>
    </estring>
</run>
''')

    def testToPathNoIdFromPathId(self):
        e = self.lib.element("input.string",type="object")
        self.lib.copy("estring","",e)
        self.assertEqual(self.lib.xml(),'''<?xml version="1.0"?>
<run>
    <tool>
        <repository>
            <rappture>
                <date>$Date: 2007-03-08 15:27:02 -0500 (Thu, 08 Mar 2007) $</date>
                <revision>$Rev: 613 $</revision>
                <url>$URL: https://kearneyd@repo.nanohub.org/svn/rappture/trunk/src/core/RpLibrary.cc $</url>
            </rappture>
        </repository>
    </tool>
    <input>
        <string id="test">
            <current>&lt;sdfsfs&gt;sdfs</current>
        </string>
        <time>Thu Mar 15 02:03:39 2007 EDT</time>
        <image>
            <current>@@RP-ENC:b64
R0lGODlhGgASAKEBAAAAAP///////////yH+FUNyZWF0ZWQgd2l0aCBUaGUgR0lNUAAh+QQBCgAB
ACwAAAAAGgASAAACLoyPqcvtD8CRj8VZrYw8h/tRn2eA4Eiaosa1qttC1EmW81qrtbYvdG8DCodE
RQEAOw==</current>
        </image>
    </input>
    <estring>
        <current>&lt;sdfsfs&gt;sdfs</current>
    </estring>
</run>
''')

class ElementTests(unittest.TestCase):

    def setUp(self):
        self.lib = Rappture.library("test.xml")

    def tearDown(self):
        return

    def testArguments_TooMaryArgs(self):
        self.assertRaises(TypeError,self.lib.element,1,2,3,4)

    def testArguments_ArgsWrongType2(self):
        self.assertRaises(TypeError,self.lib.element,1,2)

    def testArguments_NoArgs(self):
        ele = self.lib.element()
        self.assertEqual(type(ele),type(self.lib))

    def testArguments_PathArg(self):
        ele = self.lib.element("tool.repository")
        self.assertEqual(type(ele),type(self.lib))

    def testArguments_TwoArgs(self):
        ele = self.lib.element("input.string","component")
        self.assertEqual(ele,"string(test)")

    def testArguments_TwoKeywordArgs(self):
        ele = self.lib.element(path="input.string",type="path")
        self.assertEqual(ele,"input.string(test)")

    def testArguments_AsKeywordArgs(self):
        ele = self.lib.element("input.string",type="type")
        self.assertEqual(ele,"string")

    def testArguments_AsId(self):
        ele = self.lib.element(path="input.string",type="id")
        self.assertEqual(ele,"test")

    def testArguments_UnrecognizedAs(self):
        self.assertRaises(ValueError,self.lib.element,"input.string","blahh")

class GetTests(unittest.TestCase):

    def setUp(self):
        self.lib = Rappture.library("test.xml")

    def tearDown(self):
        return

    def testArguments_TooMaryArgs(self):
        self.assertRaises(TypeError,self.lib.get,1,2,3,4)

    def testArguments_ArgsWrongType2(self):
        self.assertRaises(TypeError,self.lib.get,1,2)

    def testArgumentBlank(self):
        self.assertEqual(self.lib.get(),"")

    def testArgumentPath(self):
        self.assertEqual(self.lib.get("input.time"),"Thu Mar 15 02:03:39 2007 EDT")

    def testArgumentDecode(self):
        # grabs the decoded binary data
        self.assertEqual(self.lib.get("input.image.current"),"\x47\x49\x46\x38\x39\x61\x1a\x00\x12\x00\xa1\x01\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\xff\x21\xfe\x15\x43\x72\x65\x61\x74\x65\x64\x20\x77\x69\x74\x68\x20\x54\x68\x65\x20\x47\x49\x4d\x50\x00\x21\xf9\x04\x01\x0a\x00\x01\x00\x2c\x00\x00\x00\x00\x1a\x00\x12\x00\x00\x02\x2e\x8c\x8f\xa9\xcb\xed\x0f\xc0\x91\x8f\xc5\x59\xad\x8c\x3c\x87\xfb\x51\x9f\x67\x80\xe0\x48\x9a\xa2\xc6\xb5\xaa\xdb\x42\xd4\x49\x96\xf3\x5a\xab\xb5\xb6\x2f\x74\x6f\x03\x0a\x87\x44\x45\x01\x00\x3b")

    def testArgumentDecodeYes(self):
        # grabs the decoded binary data
        self.assertEqual(self.lib.get("input.image.current", decode="yes"),"\x47\x49\x46\x38\x39\x61\x1a\x00\x12\x00\xa1\x01\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\xff\x21\xfe\x15\x43\x72\x65\x61\x74\x65\x64\x20\x77\x69\x74\x68\x20\x54\x68\x65\x20\x47\x49\x4d\x50\x00\x21\xf9\x04\x01\x0a\x00\x01\x00\x2c\x00\x00\x00\x00\x1a\x00\x12\x00\x00\x02\x2e\x8c\x8f\xa9\xcb\xed\x0f\xc0\x91\x8f\xc5\x59\xad\x8c\x3c\x87\xfb\x51\x9f\x67\x80\xe0\x48\x9a\xa2\xc6\xb5\xaa\xdb\x42\xd4\x49\x96\xf3\x5a\xab\xb5\xb6\x2f\x74\x6f\x03\x0a\x87\x44\x45\x01\x00\x3b")

    def testArgumentDecodeTrue(self):
        # grabs the decoded binary data
        self.assertEqual(self.lib.get("input.image.current", decode="true"),"\x47\x49\x46\x38\x39\x61\x1a\x00\x12\x00\xa1\x01\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\xff\x21\xfe\x15\x43\x72\x65\x61\x74\x65\x64\x20\x77\x69\x74\x68\x20\x54\x68\x65\x20\x47\x49\x4d\x50\x00\x21\xf9\x04\x01\x0a\x00\x01\x00\x2c\x00\x00\x00\x00\x1a\x00\x12\x00\x00\x02\x2e\x8c\x8f\xa9\xcb\xed\x0f\xc0\x91\x8f\xc5\x59\xad\x8c\x3c\x87\xfb\x51\x9f\x67\x80\xe0\x48\x9a\xa2\xc6\xb5\xaa\xdb\x42\xd4\x49\x96\xf3\x5a\xab\xb5\xb6\x2f\x74\x6f\x03\x0a\x87\x44\x45\x01\x00\x3b")

    def testArgumentDecodeOn(self):
        # grabs the decoded binary data
        self.assertEqual(self.lib.get("input.image.current", decode="on"),"\x47\x49\x46\x38\x39\x61\x1a\x00\x12\x00\xa1\x01\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\xff\x21\xfe\x15\x43\x72\x65\x61\x74\x65\x64\x20\x77\x69\x74\x68\x20\x54\x68\x65\x20\x47\x49\x4d\x50\x00\x21\xf9\x04\x01\x0a\x00\x01\x00\x2c\x00\x00\x00\x00\x1a\x00\x12\x00\x00\x02\x2e\x8c\x8f\xa9\xcb\xed\x0f\xc0\x91\x8f\xc5\x59\xad\x8c\x3c\x87\xfb\x51\x9f\x67\x80\xe0\x48\x9a\xa2\xc6\xb5\xaa\xdb\x42\xd4\x49\x96\xf3\x5a\xab\xb5\xb6\x2f\x74\x6f\x03\x0a\x87\x44\x45\x01\x00\x3b")

    def testArgumentDecode1Str(self):
        # grabs the decoded binary data
        self.assertEqual(self.lib.get("input.image.current", decode="1"),"\x47\x49\x46\x38\x39\x61\x1a\x00\x12\x00\xa1\x01\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\xff\x21\xfe\x15\x43\x72\x65\x61\x74\x65\x64\x20\x77\x69\x74\x68\x20\x54\x68\x65\x20\x47\x49\x4d\x50\x00\x21\xf9\x04\x01\x0a\x00\x01\x00\x2c\x00\x00\x00\x00\x1a\x00\x12\x00\x00\x02\x2e\x8c\x8f\xa9\xcb\xed\x0f\xc0\x91\x8f\xc5\x59\xad\x8c\x3c\x87\xfb\x51\x9f\x67\x80\xe0\x48\x9a\xa2\xc6\xb5\xaa\xdb\x42\xd4\x49\x96\xf3\x5a\xab\xb5\xb6\x2f\x74\x6f\x03\x0a\x87\x44\x45\x01\x00\x3b")

    def testArgumentDecode1Int(self):
        # grabs the decoded binary data
        self.assertEqual(self.lib.get("input.image.current", decode=1),"\x47\x49\x46\x38\x39\x61\x1a\x00\x12\x00\xa1\x01\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\xff\x21\xfe\x15\x43\x72\x65\x61\x74\x65\x64\x20\x77\x69\x74\x68\x20\x54\x68\x65\x20\x47\x49\x4d\x50\x00\x21\xf9\x04\x01\x0a\x00\x01\x00\x2c\x00\x00\x00\x00\x1a\x00\x12\x00\x00\x02\x2e\x8c\x8f\xa9\xcb\xed\x0f\xc0\x91\x8f\xc5\x59\xad\x8c\x3c\x87\xfb\x51\x9f\x67\x80\xe0\x48\x9a\xa2\xc6\xb5\xaa\xdb\x42\xd4\x49\x96\xf3\x5a\xab\xb5\xb6\x2f\x74\x6f\x03\x0a\x87\x44\x45\x01\x00\x3b")

    def testArgumentNoDecodeNo(self):
        # grabs the undecoded data
        self.assertEqual(self.lib.get("input.image.current", decode="no"),"""@@RP-ENC:b64
R0lGODlhGgASAKEBAAAAAP///////////yH+FUNyZWF0ZWQgd2l0aCBUaGUgR0lNUAAh+QQBCgAB
ACwAAAAAGgASAAACLoyPqcvtD8CRj8VZrYw8h/tRn2eA4Eiaosa1qttC1EmW81qrtbYvdG8DCodE
RQEAOw==""")

    def testArgumentNoDecodeFalse(self):
        # grabs the undecoded data
        self.assertEqual(self.lib.get("input.image.current", decode="FalSe"),"""@@RP-ENC:b64
R0lGODlhGgASAKEBAAAAAP///////////yH+FUNyZWF0ZWQgd2l0aCBUaGUgR0lNUAAh+QQBCgAB
ACwAAAAAGgASAAACLoyPqcvtD8CRj8VZrYw8h/tRn2eA4Eiaosa1qttC1EmW81qrtbYvdG8DCodE
RQEAOw==""")

    def testArgumentNoDecodeOff(self):
        # grabs the undecoded data
        self.assertEqual(self.lib.get("input.image.current", decode="OFF"),"""@@RP-ENC:b64
R0lGODlhGgASAKEBAAAAAP///////////yH+FUNyZWF0ZWQgd2l0aCBUaGUgR0lNUAAh+QQBCgAB
ACwAAAAAGgASAAACLoyPqcvtD8CRj8VZrYw8h/tRn2eA4Eiaosa1qttC1EmW81qrtbYvdG8DCodE
RQEAOw==""")

    def testArgumentNoDecode0Str(self):
        # grabs the undecoded data
        self.assertEqual(self.lib.get("input.image.current", decode="0"),"""@@RP-ENC:b64
R0lGODlhGgASAKEBAAAAAP///////////yH+FUNyZWF0ZWQgd2l0aCBUaGUgR0lNUAAh+QQBCgAB
ACwAAAAAGgASAAACLoyPqcvtD8CRj8VZrYw8h/tRn2eA4Eiaosa1qttC1EmW81qrtbYvdG8DCodE
RQEAOw==""")

    def testArgumentNoDecode0Int(self):
        # grabs the undecoded data
        self.assertEqual(self.lib.get("input.image.current", decode=0),"""@@RP-ENC:b64
R0lGODlhGgASAKEBAAAAAP///////////yH+FUNyZWF0ZWQgd2l0aCBUaGUgR0lNUAAh+QQBCgAB
ACwAAAAAGgASAAACLoyPqcvtD8CRj8VZrYw8h/tRn2eA4Eiaosa1qttC1EmW81qrtbYvdG8DCodE
RQEAOw==""")

    def testArgumentDecodeError(self):
        self.assertRaises(ValueError,self.lib.get,"input.image.current","blahh")

#    def testGetEncodedData(self):
#
#        self.assertEqual()

class PutTests(unittest.TestCase):

    def setUp(self):
        self.lib = Rappture.library("test.xml")

    def tearDown(self):
        return

    def testArguments_TooMaryArgs(self):
        self.assertRaises(TypeError,self.lib.put,1,2,3,4,5,6,7)

    def testArguments_NotEnoughArgs(self):
        self.assertRaises(TypeError,self.lib.put,1)

    def testArguments_ArgsWrongType2(self):
        self.assertRaises(TypeError,self.lib.put,1,2)

    def testArguments_ValueIsNone(self):
        self.lib.put("input.none",None)
        self.assertEqual(self.lib.get("input.none"),"None")

    def testArgumentAppendValError(self):
        self.assertRaises(ValueError,self.lib.put,"output.blah","val","","blahhhh")

    def testArgumentCompressValError(self):
        self.assertRaises(ValueError,self.lib.put,"output.blah","vl","","no","","blh")

    def testArgumentBlank(self):
        self.assertRaises(TypeError,self.lib.put,None)

    def testArgumentPathValue(self):
        self.lib.put("output.string","43eV")
        self.assertEqual(self.lib.get("output.string"),"43eV")

    def testArgumentPathValueIdAppendTypeCompress(self):
        self.lib.put("output.string","43eV","id",0,"string",0)
        self.assertEqual(self.lib.get("output.string"),"43eV")

    def testArgumentCheckId(self):
        # this test checks that the id argument is no longer in use
        self.lib.put("output.string","43eV","id",0,"string",0)
        self.assertEqual(self.lib.get("output.string(id)"),"")

    def testArgumentAllNamedArgs(self):
        self.lib.put(path="output.string",value="43eV",id="id",append=0,type="string",compress=0)
        self.assertEqual(self.lib.get("output.string"),"43eV")

    def testArgumentCheckAppend(self):
        self.lib.put("input.string.current","43eV",append=1)
        self.assertEqual(self.lib.get("input.string.current"),"&lt;sdfsfs&gt;sdfs43eV")

    def testArgumentCheckAppendYes(self):
        self.lib.put("input.string.current","43eV",append="yes")
        self.assertEqual(self.lib.get("input.string.current"),"&lt;sdfsfs&gt;sdfs43eV")

    def testArgumentCheckTypeStringCompressYes(self):
        self.lib.put("input.string.current","43eV",type="string",compress="yes")
        self.assertEqual(self.lib.get("input.string.current",decode=False),\
"""@@RP-ENC:zb64
H4sIAAAAAAAAAzMxTg0DAACAouIEAAAA
""")

    def testArgumentCheckTypeFile(self):
        self.lib.put("output.file","test.xml",type="file")
        self.assertEqual(self.lib.get("output.file"),"""<?xml version="1.0"?>
<run>
    <tool>
        <repository>
            <rappture>
                <date>$Date: 2007-03-08 15:27:02 -0500 (Thu, 08 Mar 2007) $</date>
                <revision>$Rev: 613 $</revision>
                <url>$URL: https://kearneyd@repo.nanohub.org/svn/rappture/trunk/src/core/RpLibrary.cc $</url>
            </rappture>
        </repository>
    </tool>
    <input>
        <string id="test">
            <current>&lt;sdfsfs&gt;sdfs</current>
        </string>
        <time>Thu Mar 15 02:03:39 2007 EDT</time>
        <image>
            <current>@@RP-ENC:b64
R0lGODlhGgASAKEBAAAAAP///////////yH+FUNyZWF0ZWQgd2l0aCBUaGUgR0lNUAAh+QQBCgAB
ACwAAAAAGgASAAACLoyPqcvtD8CRj8VZrYw8h/tRn2eA4Eiaosa1qttC1EmW81qrtbYvdG8DCodE
RQEAOw==</current>
        </image>
    </input>
</run>
""")

    def testArgumentCheckTypeFileCompressTrue(self):
        self.lib.put("output.file","test.xml",type="file",compress=True)
        self.assertEqual(self.lib.get("output.file"),"""<?xml version="1.0"?>
<run>
    <tool>
        <repository>
            <rappture>
                <date>$Date: 2007-03-08 15:27:02 -0500 (Thu, 08 Mar 2007) $</date>
                <revision>$Rev: 613 $</revision>
                <url>$URL: https://kearneyd@repo.nanohub.org/svn/rappture/trunk/src/core/RpLibrary.cc $</url>
            </rappture>
        </repository>
    </tool>
    <input>
        <string id="test">
            <current><sdfsfs>sdfs</current>
        </string>
        <time>Thu Mar 15 02:03:39 2007 EDT</time>
        <image>
            <current>@@RP-ENC:b64
R0lGODlhGgASAKEBAAAAAP///////////yH+FUNyZWF0ZWQgd2l0aCBUaGUgR0lNUAAh+QQBCgAB
ACwAAAAAGgASAAACLoyPqcvtD8CRj8VZrYw8h/tRn2eA4Eiaosa1qttC1EmW81qrtbYvdG8DCodE
RQEAOw==</current>
        </image>
    </input>
</run>
""")

    def testArgumentCheckTypeFileCompressTrueNoDecode(self):
        self.lib.put("output.file","test.xml",type="file",compress=True)
        self.assertEqual(self.lib.get("output.file",decode=False),"""@@RP-ENC:zb64
H4sIAAAAAAAAA3VTXW/aMBR951dYCFWbOrATSptlIcUkKZXGKGTNUHkziZtkDXGwnbD8++VD
HTC2+2D5nnvP/TiWjftfuwQUlIuYpeOuMkDde7Nj8Dw1O6AyQzKWtNfG5TRjIpaMl0ewDZAs
kzmn53ATCoikZs+uTh2oCN310bCPNKCMdPVORyrooxFC4MNzlH8CFf6N8CbtI+gZsOFeluS0
iOuJzZ5LCx3cKsM6+Q96Sch5YvY8d66DSMpM6BC+UcJTWgaTeqVBSlIW5dsB4yEURQrf14Gy
kuINCu5Dn1Wum83jLSe8HPh+3bKuey4EvFSinuxcNgMedTXiNMvlSbaQPE5DEAfjrqRCdv9q
4Oec01SaV4n8IoJX8SquwvZmwPfYSeu22gki4x01K7EboZURQKqOhvrwcyM6cOznarg65ciI
dySk/5liMnGXfWdh6dvbm46LktmTnUSzEH/HX50prm0Jj1Y+Xj94i3KzfkCb9SoM1AQRa+qR
mRdW3IWHcXS9Wk2tEE872Do0/KYYxtaclcu9X0hbs9yf2o8NfzloEZRuqlJ848SECaLspbQU
Z7fWlD2X25cimGm2xQKn464c/HQYj/8p0cl+ldO+RvVm9R/4DWcyEdUgAwAA
""")

    def testArgumentCheckTypeError(self):
        self.assertRaises(ValueError,self.lib.put,"output.file","vl","","","type","")

    def testPutObject(self):
        e = self.lib.element("input.string",type="object")
        self.lib.put("output.estring(qqq)",e)
        self.assertEqual(self.lib.get("output.estring(qqq).current"),'<sdfsfs>sdfs')

class XmlTests(unittest.TestCase):

    def setUp(self):
        self.lib = Rappture.library("test.xml")

    def tearDown(self):
        return

    def testArguments_NoArgs(self):
        xml = self.lib.xml()
        self.assertEqual(xml,'''<?xml version="1.0"?>
<run>
    <tool>
        <repository>
            <rappture>
                <date>$Date: 2007-03-08 15:27:02 -0500 (Thu, 08 Mar 2007) $</date>
                <revision>$Rev: 613 $</revision>
                <url>$URL: https://kearneyd@repo.nanohub.org/svn/rappture/trunk/src/core/RpLibrary.cc $</url>
            </rappture>
        </repository>
    </tool>
    <input>
        <string id="test">
            <current>&lt;sdfsfs&gt;sdfs</current>
        </string>
        <time>Thu Mar 15 02:03:39 2007 EDT</time>
        <image>
            <current>@@RP-ENC:b64
R0lGODlhGgASAKEBAAAAAP///////////yH+FUNyZWF0ZWQgd2l0aCBUaGUgR0lNUAAh+QQBCgAB
ACwAAAAAGgASAAACLoyPqcvtD8CRj8VZrYw8h/tRn2eA4Eiaosa1qttC1EmW81qrtbYvdG8DCodE
RQEAOw==</current>
        </image>
    </input>
</run>
''')

    def testArguments_Args(self):
        self.assertRaises(TypeError,self.lib.xml,"arg1")

class ResultTests(unittest.TestCase):

    class redirectedOutput:
        # this class doesn't work yet
        # the stdout from the c++ result function is not
        # sent through python's sys.stdout
        def write(self,text):
            import re
            pattern = re.compile('run\d+\.xml')
            match = pattern.search(text)
            if not match == None:
                checkFile(match.group())
        def checkFile(filename):
            lib2 = Rappture.library(filename)
            self.assertEqual(self.lib.xml(),lib2.xml())

    def setUp(self):
        self.lib = Rappture.library("test.xml")

    def tearDown(self):
        return

# tests commented out because i dont like to keep deleting the files they create
#    def testArguments_NoArgs(self):
#        import sys
#        sys.stdout = self.redirectedOutput()
#        self.lib.result()
#
#    def testArguments_ValidStatusArg(self):
#        import sys
#        sys.stdout = self.redirectedOutput()
#        self.lib.result(status=0)

    def testArguments_InvalidStatusArg(self):
        self.assertRaises(TypeError,self.lib.result,"0")

if __name__ == '__main__':
    unittest.main()
