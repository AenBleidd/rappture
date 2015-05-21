# ----------------------------------------------------------------------
#  COMPONENT: library - provides access to the XML library
#
#  These routines make it easy to load the XML description for a
#  series of tool parameters and browse through the results.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
from xml.dom import minidom
import re, string, time

class library:
    """
    A Rappture library object represents an entire XML document or
    part of an XML tree.  It simplifies the interface to the XML
    package, giving the caller an easier mechanism for finding,
    querying, and setting values in the tree.
    """

    re_xml = re.compile('<\?[Xx][Mm][Ll]')
    re_dots = re.compile('(\([^\)]*)\.([^\)]*\))')
    re_pathElem = re.compile('^(([a-zA-Z_]+#?)([0-9]*))?(\(([^\)]+)\))?$')
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
    def element(self, path="", type="object"):
        """
        Clients use this to query a particular element within the
        entire data structure.  The path is a string of the form
        "structure.box(source).corner".  This example represents
        the tag <corner> within a tag <box id="source"> within a
        a tag <structure>, which must be found at the top level
        within this document.

        By default, this method returns an object representing the
        DOM node referenced by the path.  This is changed by setting
        the "type" argument to "id" (for name of the tail element),
        to "type" (for the type of the tail element), to "component"
        (for the component name "type(id)"), or to "object"
        for the default (an object representing the tail element).
        """

        node = self._find(path)
        if not node:
            return None

        if type == 'object':
            return library(node)
        elif type == 'component':
            return self._node2comp(node)
        elif type == 'id':
            return self._node2name(node)
        elif type == 'type':
            return node.tagName

        raise ValueError, "bad type value '%s': should be component, id, object, type" % type

    # ------------------------------------------------------------------
    def children(self, path="", type="object", rtype=None):
        """
        Clients use this to query the children of a particular element
        within the entire data structure.  This is just like the
        element() method, but it returns the children of the element
        instead of the element itself.  If the optional rtype argument
        is specified, then the return list is restricted to children
        of the specified type.

        By default, this method returns a list of objects representing
        the children.  This is changed by setting the "type" argument
        to "id" (for tail names of all children), to "type" (for the
        types of all children), to "component" (for the path component
        names of all children), or to "object" for the default (a list
        child objects).
        """

        node = self._find(path)
        if not node:
            return None

        nlist = [n for n in node.childNodes if not n.nodeName.startswith('#')]

        if rtype:
            nlist = [n for n in nlist if n.nodeName == rtype]

        if type == 'object':
            return [library(n) for n in nlist]
        elif type == 'component':
            return [self._node2comp(n) for n in nlist]
        elif type == 'id':
            return [self._node2name(n) for n in nlist]
        elif type == 'type':
            return [n.tagName for n in nlist]

        raise ValueError, "bad type value '%s': should be component, id, object, type" % type

    # ------------------------------------------------------------------
    def get(self, path=""):
        """
        Clients use this to query the value of a node.  If the path
        is not specified, it returns the value associated with the
        root node.  Otherwise, it returns the value for the element
        specified by the path.
        """

        node = self._find(path)
        if not node:
            return None

        clist = node.childNodes
        rval = None
        for node in [n for n in clist if isinstance(n,minidom.Text)]:
            if rval == None:
                rval = node.nodeValue
            else:
                rval += node.nodeValue
        if rval:
            rval = rval.strip()
        return rval

    # ------------------------------------------------------------------
    def put(self, path="", value="", id=None, append=0):
        """
        Clients use this to set the value of a node.  If the path
        is not specified, it sets the value for the root node.
        Otherwise, it sets the value for the element specified
        by the path.  If the value is a string, then it is treated
        as the text within the tag at the tail of the path.  If it
        is a DOM node or a library, then it is inserted into the
        tree at the specified path.

        If the optional id is specified, then it sets the identifier
        for the tag at the tail of the path.  If the optional append
        flag is specified, then the value is appended to the current
        value.  Otherwise, the value replaces the current value.
        """

        node = self._find(path,create=1)

        if append:
            nlist = [n for n in node.childNodes
                       if not n.nodeType == n.TEXT_NODE]
        else:
            nlist = node.childNodes

        for n in nlist:
            node.removeChild(n)

        if value:
            # if there's a value, then add it to the node
            if isinstance(value, library):
                node.appendChild(value.node.cloneNode(1))
            elif isinstance(value, minidom.Node):
                node.appendChild(value)
            elif value and value != '':
                n = self.doc.createTextNode(value)
                node.appendChild(n)

        # if there's an id, then set it on the node
        if id:
            node.setAttribute("id",id)

    # ------------------------------------------------------------------
    def remove(self, path=""):
        """
        Clients use this to remove the specified node.  Removes the
        node from the tree and returns it as an element, so you can
        put it somewhere else if you like.
        """

        node = self._find(path)
        if not node:
            return None

        rval = library('<?xml version="1.0"?>' + node.toxml())
        node.parentNode.removeChild(node)

        return rval

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
        specified node.  A path component name has the form "type(id)"
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
        the form "typeNN(id).typeNN(id). ...", where each
        "type" is a tag <type>; if the optional NN is specified,
        it indicates an index for the <type> tag within its parent;
        if the optional (id) part is included, it indicates a
        tag of the form <type id="id">.

        By default, it looks for an element along the path and
        returns None if not found.  If the create flag is set,
        it creates various elements along the path as it goes.
        This is useful for "put" operations.

        If you include "#" instead of a specific number, a node
        will be created automatically with a new number.  For example,
        the path "foo.bar#" called the first time will create "foo.bar",
        the second time "foo.bar1", the third time "foo.bar2" and
        so forth.

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
            # Pick apart "type123(id)" for this part.
            #
            match = self.re_pathElem.search(part)
            if not match:
                raise ValueError, "bad path component '%s': should have the form 'type123(id)'" % part

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
                # If the name is like "type(id)", then look for elements
                # that match the type and see if one has the requested name.
                # if the name is like "(id)", then look for any elements
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
                # with the specified "type(id)" and continue on.
                # If the type is "type#", then create a node with
                # an automatic number.
                #
                match = self.re_pathCreateElem.search(part)
                if match:
                    (type, name) = match.groups()
                else:
                    type = part
                    name = ""

                if type.endswith('#'):
                    type = type.rstrip('#')
                    node = self.doc.createElement(type)

                    # find the last node of same type and append there
                    pos = None
                    for n in lastnode.childNodes:
                        if n.nodeName == type:
                            pos = n

                    if pos:
                        pos = pos.nextSibling

                    if pos:
                        lastnode.insertBefore(node,pos)
                    else:
                        lastnode.appendChild(node)
                else:
                    node = self.doc.createElement(type)
                    lastnode.appendChild(node)

                if name:
                    node.setAttribute("id",name)

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
