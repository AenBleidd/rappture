# ----------------------------------------------------------------------
#  COMPONENT: library - provides access to the XML library
#
#  These routines make it easy to load the XML description for a
#  series of tool parameters and browse through the results.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2005  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
from xml.dom import minidom
import re, string

class library:
    """
    A Rappture library object represents an entire XML document or
    part of an XML tree.  It simplifies the interface to the XML
    package, giving the caller an easier mechanism for finding,
    querying, and setting values in the tree.
    """

    re_xml = re.compile('<\?[Xx][Mm][Ll]')
    re_dots = re.compile('(\([^\)]*)\.([^\)]*\))')
    re_pathElem = re.compile('^(([a-zA-Z_]+)([0-9]*))?(\(([^\)]+)\))?$')
    re_pathCreateElem = re.compile('^([^\(]+)\(([^\)]+)\)$')

    # ------------------------------------------------------------------
    def __init__(self, elem):
        if isinstance(elem, minidom.Node):
            # if the input is a node, then wrap up this node
            self.doc = elem.ownerDocument
            self.node = elem
        elif self.re_xml.match(elem):
            # if the input is a string, then parse the string
            self.doc = minidom.parseString(elem)
            self.node = self.doc.documentElement
        else:
            # if the input is a file, then parse the file
            self.doc = minidom.parse(elem)
            self.node = self.doc.documentElement

    # ------------------------------------------------------------------
    def element(self, path="", flavor="object"):
        """
        Clients use this to query a particular element within the
        entire data structure.  The path is a string of the form
        "structure.box(source).corner".  This example represents
        the tag <corner> within a tag <box id="source"> within a
        a tag <structure>, which must be found at the top level
        within this document.

        By default, this method returns an object representing the
        DOM node referenced by the path.  This is changed by setting
        the "flavor" argument to "name" (for name of the tail element),
        to "type" (for the type of the tail element), to "component"
        (for the component name "type(name)"), or to "object"
        for the default (an object representing the tail element).
        """

        node = self._find(path)
        if not node:
            return None

        if flavor == 'object':
            return library(node)
        elif flavor == 'component':
            return self._node2comp(node)
        elif flavor == 'name':
            return self._node2name(node)
        elif flavor == 'type':
            return node.tagName

        raise ValueError, "bad flavor '%s': should be object, name, type" % flavor

    # ------------------------------------------------------------------
    def children(self, path="", flavor="object", type=None):
        """
        Clients use this to query the children of a particular element
        within the entire data structure.  This is just like the
        element() method, but it returns the children of the element
        instead of the element itself.  If the optional type argument
        is specified, then the return list is restricted to children
        of the specified type.

        By default, this method returns a list of objects representing
        the children.  This is changed by setting the "flavor" argument
        to "name" (for tail names of all children), to "type" (for the
        types of all children), to "component" (for the path component
        names of all children), or to "object" for the default (a list
        child objects).
        """

        node = self._find(path)
        if not node:
            return None

        nlist = [n for n in node.childNodes if not n.nodeName.startswith('#')]

        if type:
            nlist = [n for n in nlist if n.nodeName == type]

        if flavor == 'object':
            return [library(n) for n in nlist]
        elif flavor == 'component':
            return [self._node2comp(n) for n in nlist]
        elif flavor == 'name':
            return [self._node2name(n) for n in nlist]
        elif flavor == 'type':
            return [n.tagName for n in nlist]

    # ------------------------------------------------------------------
    def get(self, path=""):
        """
        Clients use this to query the value of a node.  If the path
        is not specified, it returns the value associated with the
        root node.  Otherwise, it returns the value for the element
        specified by the path.
        """

        if path == "":
            node = self.node
        else:
            node = self._find(path)

        if not node:
            return None

        clist = node.childNodes
        if len(clist) == 1 and isinstance(clist[0],minidom.Text):
            return clist[0].nodeValue.strip()

        return None

    # ------------------------------------------------------------------
    def xml(self):
        """
        Clients use this to query the XML representation for this
        object.
        """
        return self.doc.toxml()

    # ------------------------------------------------------------------
    def _node2name(self, node):
        """
        Used internally to create a name for the specified node.
        If the node doesn't have a specific name ("id" attribute)
        then a name of the form "type123" is constructed.
        """

        try:
            # node has a name? then return it
            name = node.getAttribute("id")
        except AttributeError:
            name = ''

        if name == '':
            # try to build a name like "type123"
            type = node.tagName
            siblings = node.parentNode.getElementsByTagName(type)
            index = siblings.index(node)
            if index == 0:
                name = type
            else:
                name = "%s%d" % (type,index)

        return name

    # ------------------------------------------------------------------
    def _node2comp(self, node):
        """
        Used internally to create a path component name for the
        specified node.  A path component name has the form "type(name)"
        or just "type##" if the node doesn't have a name.  This name
        can be used in a path to uniquely address the component.
        """

        try:
            # node has a name? then return it
            name = node.getAttribute("id")
            if name != '':
                name = "%s(%s)" % (node.tagName, name)
        except AttributeError:
            name = ''

        if name == '':
            # try to build a name like "type123"
            type = node.tagName
            siblings = node.parentNode.getElementsByTagName(type)
            index = siblings.index(node)
            if index == 0:
                name = type
            else:
                name = "%s%d" % (type,index)

        return name

    # ------------------------------------------------------------------
    def _find(self, path, create=0):
        """
        Used internally to find a particular element within the
        root node according to the path, which is a string of
        the form "type##(name).type##(name). ...", where each
        "type" is a tag <type>; if the optional ## is specified,
        it indicates an index for the <type> tag within its parent;
        if the optional (name) part is included, it indicates a
        tag of the form <type id="name">.

        By default, it looks for an element along the path and
        returns None if not found.  If the create flag is set,
        it creates various elements along the path as it goes.
        This is useful for "put" operations.

        Returns an object representing the element indicated by
        the path, or None if the path is not found.
        """

        if path == "":
            return self.node

        path = self._path2list(path)

        #
        # Follow the given path and look for all of the parts.
        #
        node = lastnode = self.node
        for part in path:
            #
            # Pick apart "type123(name)" for this part.
            #
            match = self.re_pathElem.search(part)
            if not match:
                raise ValueError, "bad path component '%s': should have the form 'type123(name)'" % part

            (dummy, type, index, dummy, name) = match.groups()

            if not name:
                #
                # If the name is like "type2", then look for elements with
                # the type name and return the one with the given index.
                # If the name is like "type", then assume the index is 0.
                #
                if not index or index == "":
                    index = 0
                else:
                    index = int(index)

                nlist = node.getElementsByTagName(type)
                if index < len(nlist):
                    node = nlist[index]
                else:
                    node = None
            else:
                #
                # If the name is like "type(name)", then look for elements
                # that match the type and see if one has the requested name.
                # if the name is like "(name)", then look for any elements
                # with the requested name.
                #
                if type:
                    nlist = node.getElementsByTagName(type)
                else:
                    nlist = node.childNodes

                node = None
                for n in nlist:
                    try:
                        tag = n.getAttribute("id")
                    except AttributeError:
                        tag = ""

                    if tag == name:
                        node = n
                        break

            if not node:
                if not create:
                    return None

                #
                # If the "create" flag is set, then create a node
                # with the specified "type(name)" and continue on.
                #
                match = self.re_pathCreateElem.search(part)
                if match:
                    (type, name) = match.groups()
                else:
                    type = part
                    name = ""

                node = self.doc.createElement(type)
                lastnode.appendChild(node)

                if name:
                    node.setAttribute("name",name)

            lastnode = node

        # last node is the desired element
        return node

    # ------------------------------------------------------------------
    def _path2list(self, path):
        """
        Used internally to convert a path name of the form
        "foo.bar(x.y).baz" to a list of the form ['foo',
        'bar(x.y)','baz'].  Note that it's careful to preserve
        dots within ()'s but splits on those outside ()'s.
        """

        #
        # Normally, we just split on .'s within the path.  But there
        # might be some .'s embedded within ()'s in the path.  Change
        # any embedded .'s to an out-of-band character, then split on
        # the .'s, and change the embedded .'s back.
        #
        while self.re_dots.search(path):
            path = self.re_dots.sub('\\1\007\\2',path)

        path = re.split('\.', path)
        return [re.sub('\007','.',elem) for elem in path]

if __name__ == "__main__":
    lib = library("""<?xml version="1.0"?>
<device>
<label>Heterojunction</label>

<recipe>
  <slab>
    <thickness>0.1um</thickness>
    <material>GaAs</material>
  </slab>
  <slab>
    <thickness>0.1um</thickness>
    <material>Al(0.3)Ga(0.7)As</material>
  </slab>
</recipe>
<field id="donors">
  <label>Doping</label>
  <units>/cm3</units>
  <scale>log</scale>
  <color>green</color>
  <component id="Nd+">
    <constant>1.0e19/cm3</constant>
    <domain>slab0</domain>
  </component>
</field>
</device>
""")
    print lib.get('label')
    print lib.get('recipe.slab1.thickness')
    print lib.get('field(donors).component(Nd+).domain')
    print lib.get('field(donors).component.domain')
