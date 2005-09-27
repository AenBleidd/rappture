from distutils.core import setup, Extension

# need to write an if statement to check to see if the libRpUnits exists
# if lib exists, then use the lib to compile against.
# else compile everything from source.
#
#module = Extension('RpUnits',
#                   include_dirs = [ '/usr/local/include',
#                                    '../../include/cee',
#                                    '../../include/core'],
#                   libraries = ['RpUnits'],
#                   library_dirs = ['../lib'],
#                   sources = [  'PyRpUnits.cc' ])

module = Extension('Rappture.Units',
                   include_dirs = [ '../../include/cee',
                                    '../../include/core'],
                   sources = [  '../core/RpUnitsStd.cc',
                                '../core/RpUnits.cc',
                                'PyRpUnits.cc' ])

setup(name="Rappture.Units",
      version="0.1",
      description="module for converting Rappture Units",
      ext_modules=[module]
      )

