from distutils.core import setup, Extension

module = Extension('Rappture.Units',
                   include_dirs = [ '../include/cee',
                                    '../include/core'],
                   sources = [  'core/RpUnitsStd.cc',
                                'core/RpUnits.cc',
                                'python/PyRpUnits.cc' ])

setup(name="Rappture.Units",
      version="0.1",
      description="module for converting Rappture Units",
      ext_modules=[module]
      )

