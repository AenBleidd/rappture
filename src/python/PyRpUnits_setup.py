from distutils.core import setup, Extension

module = Extension('RpUnits',
                   include_dirs = [ '/usr/local/include',
                                    '../../include/cee',
                                    '../../include/core'],
                   libraries = ['RpUnits'],
                   library_dirs = ['../lib'],
                   sources = ['src/PyRpUnits.cc'])

setup(name="RpUnits",
      version="0.1",
      description="module for converting Rappture Units",
      ext_modules=[module])

