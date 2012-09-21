/*
 * ----------------------------------------------------------------------
 *  Ruby Rappture Extension Source
 *
 * ======================================================================
 *  AUTHOR:  Benjamin Haley, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

/* This file uses Ruby's C API to create a new Ruby class named Rappture 
 *  as a wrapper around the Rappture library C++ API. 
 *
 * Define RAISE_EXCEPTIONS (see Makefile) to raise Ruby Exceptions on errors.
 * Without this, those methods which return a Ruby object on success return
 *  Qnil on errors.  For those methods which return Qnil on either success or 
 *  failure, there is no way to check for errors without an Exception.
 */

#include <string>        /* std::string */
#include <stdlib.h>      /* atof() */
#include "RpLibrary.h"   /* RpLibrary, Rappture::Buffer */
#include "RpUtils.h"     /* Rappture::Utils::progress() */
#include "RpUnits.h"     /* RpUnits::convert() */
#include "ruby.h"        /* VALUE, rb_*(), Data_*_Struct(), NUM2INT(), 
                            INT2NUM(), NUM2DBL(), STR2CSTR(), ANYARGS */

/******************************************************************************
 * File scope variables
 ******************************************************************************/


/* The new Rappture class */
VALUE classRappture;


extern "C" void Init_Rappture(void);

/******************************************************************************
 * RbRp_GetString()
 *
 * Implement the get() public method of the Rappture class.
 * Return a Ruby String from the location path in the Rappture object self.
 *
 * The original plan was to convert the returned std::string to a Ruby String, 
 *  then try calling the to_f and to_i methods on the Ruby String, to see if 
 *  we should return a Float (double) or a Fixnum (integer).  The problem with 
 *  this approach is that these methods (to_f and to_i) return 0.0 and 0, 
 *  respectively, on error,  **without raising an Exception**, so there is no 
 *  way to  distinguish between an error and legitimate zero values.  The user 
 *  will have to call the appropriate Ruby conversion functions on the returned
 *  String, for now. 
 ******************************************************************************/

static VALUE
RbRp_GetString(VALUE self, VALUE path)
{
   RpLibrary *lib;
   std::string str;

   /* Extract the pointer to the Rappture object, lib, from the Ruby object, 
      self */
   Data_Get_Struct(self, RpLibrary, lib);

   /* Read the data from path in lib as a C++ std::string. */
   str = lib->getString(STR2CSTR(path));
      
   /* Return a Ruby VALUE */
   return rb_str_new2(str.c_str());

}  /* end RbRp_GetString */


/******************************************************************************
 * RbRp_GetData()
 *
 * Implement the getdata() public method of the Rappture class.
 * Return a Ruby String from the location path in the Rappture object self.
 ******************************************************************************/

static VALUE
RbRp_GetData(VALUE self, VALUE path)
{
   RpLibrary *lib;
   Rappture::Buffer buf;

   /* Extract the pointer to the Rappture object, lib, from the Ruby object, 
      self */
   Data_Get_Struct(self, RpLibrary, lib);

   /* Read the data from path in lib as a C++ std::string. */
   buf = lib->getData(STR2CSTR(path));
      
   /* Return a Ruby VALUE */
   return rb_str_new2(buf.bytes());

}  /* end RbRp_GetData */


/******************************************************************************
 * RbRp_PutObject()
 *
 * Implement the put() public method of the Rappture class.
 * Put a Ruby String, Float (double), or Fixnum (integer) object into the
 *  location path in the Rappture object self.
 * Return Qnil.
 * On error, if RAISE_EXCEPTIONS is defined, raise a Ruby RuntimeError 
 *  Exception.
 ******************************************************************************/

static VALUE
RbRp_PutObject(VALUE self, VALUE path, VALUE value, VALUE append)
{
#ifdef RAISE_EXCEPTIONS
   /* Get the Ruby ID of the "to_s" method. */
   ID id_to_s = rb_intern("to_s");

   /* Call the "to_s" method on value to get a Ruby String representation of
      the value, in case we need to compose an error message below.  The final 
      argument 0 indicates no further arguments.*/
   VALUE rbStrName = rb_funcall(value, id_to_s, 0);
#endif

   RpLibrary *lib;
   int intVal;

   /* Extract the pointer to the Rappture object, lib, from the Ruby 
      object, self */
   Data_Get_Struct(self, RpLibrary, lib);

   /* Check the type of the Ruby object value.  If the type if one we are
      prepared to handle, call the appropriate put() function.  If the type
      is unexpected, raise a RuntimeError Exception, if RAISE_EXCEPTIONS is
      defined.*/ 
   switch (TYPE(value))
   {
      case T_STRING:
         lib->put(STR2CSTR(path), STR2CSTR(value), "", NUM2INT(append));
         break;
      case T_FIXNUM:
         intVal = NUM2INT(value);
         lib->putData(STR2CSTR(path), (const char *)&intVal, sizeof(int),
                      NUM2INT(append));
         break;
      case T_FLOAT:
         lib->put(STR2CSTR(path), NUM2DBL(value), "", NUM2INT(append));
         break;
      default:
#ifdef RAISE_EXCEPTIONS
         rb_raise(rb_eRuntimeError, 
                  "Unable to put object %s to Rappture: unknown type", 
                  STR2CSTR(rbStrName));
#endif
         break;
   }  /* end switch */

   /* Return a Ruby VALUE */
   return Qnil;

}  /* end RbRp_PutObject */


/******************************************************************************
 * RbRp_PutData()
 *
 * Implement the putdata() public method of the Rappture class.
 * Put a Ruby String into the location path in the Rappture object self.
 * Return Qnil.
 * On error, if RAISE_EXCEPTIONS is defined, raise a Ruby RuntimeError 
 *  Exception.
 ******************************************************************************/

static VALUE
RbRp_PutData(VALUE self, VALUE path, VALUE value, VALUE append)
{
#ifdef RAISE_EXCEPTIONS
   /* Get the Ruby ID of the "to_s" method. */
   ID id_to_s = rb_intern("to_s");

   /* Call the "to_s" method on value to get a Ruby String representation of
      the value, in case we need to compose an error message below.  The final 
      argument 0 indicates no further arguments.*/
   VALUE rbStrName = rb_funcall(value, id_to_s, 0);
#endif

   RpLibrary *lib;

   /* Extract the pointer to the Rappture object, lib, from the Ruby 
      object, self */
   Data_Get_Struct(self, RpLibrary, lib);

   if (T_STRING == TYPE(value))
   {
      long int nbytes;
      char *bytes = rb_str2cstr(value, &nbytes);

      lib->putData(STR2CSTR(path), bytes, nbytes, NUM2INT(append));
   }
#ifdef RAISE_EXCEPTIONS
   else
   {
      rb_raise(rb_eRuntimeError, 
               "Unable to put data \"%s\" to Rappture: unknown type", 
               STR2CSTR(rbStrName));
   }
#endif

   /* Return a Ruby VALUE */
   return Qnil;

}  /* end RbRp_PutData */


/******************************************************************************
 * RbRp_PutFile()
 *
 * Implement the putfile() public method of the Rappture class.
 * Put a file into the location path in the Rappture object self.
 * Return Qnil.
 * On error, if RAISE_EXCEPTIONS is defined, raise a Ruby RuntimeError 
 *  Exception.
 ******************************************************************************/

static VALUE
RbRp_PutFile(VALUE self, VALUE path, VALUE filename, VALUE append, 
             VALUE compress)
{
#ifdef RAISE_EXCEPTIONS
   /* Get the Ruby ID of the "to_s" method. */
   ID id_to_s = rb_intern("to_s");

   /* Call the "to_s" method on value to get a Ruby String representation of
      the value, in case we need to compose an error message below.  The final 
      argument 0 indicates no further arguments.*/
   VALUE rbStrName = rb_funcall(filename, id_to_s, 0);
#endif

   RpLibrary *lib;

   /* Extract the pointer to the Rappture object, lib, from the Ruby 
      object, self */
   Data_Get_Struct(self, RpLibrary, lib);

   if (T_STRING == TYPE(filename))
   {
      VALUE ft = rb_const_get(rb_cObject, rb_intern("FileTest")); 
      ID id_filetest = rb_intern("file?");

      if (Qtrue == rb_funcall(ft, id_filetest, 1, filename))  /* valid filename */
      {
         lib->putFile(STR2CSTR(path), STR2CSTR(filename), NUM2INT(compress), 
                      NUM2INT(append));
      }
#ifdef RAISE_EXCEPTIONS
      else
      {
         rb_raise(rb_eRuntimeError, "%s is not a valid file", 
                  STR2CSTR(rbStrName));
      }
#endif
   }
#ifdef RAISE_EXCEPTIONS
   else
      rb_raise(rb_eRuntimeError, "Bad file name: %s", STR2CSTR(rbStrName));
#endif

   /* Return a Ruby VALUE */
   return Qnil;

}  /* end RbRp_PutFile */


/******************************************************************************
 * RbRp_Result()
 *
 * Implement the result() public method of the Rappture class.
 * Write the XML of the Rappture object self to disk.
 * Return Qnil.
 ******************************************************************************/

static VALUE
RbRp_Result(VALUE self, VALUE status)
{
   RpLibrary *lib;

   /* Extract the pointer to the Rappture object, lib, from the Ruby 
      object, self */
   Data_Get_Struct(self, RpLibrary, lib);

   /* Write the Rappture XML to disk. */
   lib->result(NUM2INT(status));

   /* Return a Ruby VALUE */
   return Qnil;

}  /* end RbRp_Result */


/******************************************************************************
 * RbRp_Xml()
 *
 * Impelement the xml() public method of the Rappture class.
 * Return a Ruby String with the XML of the Rappture object self.
 * Return Qnil on error, or, if RAISE_EXCEPTIONS is defined, raise a 
 *  RuntimeError Exception.
 ******************************************************************************/
   
static VALUE
RbRp_Xml(VALUE self)
{
   RpLibrary *lib;
   std::string str;

   /* Extract the pointer to the Rappture object, lib, from the Ruby 
      object, self */
   Data_Get_Struct(self, RpLibrary, lib);

   /* Get the Rappture object's XML */
   str = lib->xml();

   /* Return a Ruby VALUE */
   if (str.empty())
   {
#ifdef RAISE_EXCEPTIONS
      rb_raise(rb_eRuntimeError, "Unable to retrieve XML");
#else
      return Qnil;
#endif
   }
   else
      return rb_str_new2(str.c_str());

}  /* end RbRp_Xml */


/******************************************************************************
 * RbRp_Progress()
 *
 * Implement the progress() public method for the Rappture class.
 * Write a progress message to stdout, using the Ruby Fixnum percent and the
 *  Ruby String message.
 * Return Qnil.
 ******************************************************************************/

static VALUE
RbRp_Progress(VALUE self, VALUE percent, VALUE message)
{
   /* Note: self is a dummy here */

   /* Write the message */
   (void)Rappture::Utils::progress(NUM2INT(percent), STR2CSTR(message));

   /* Return a Ruby VALUE */
   return Qnil;

}  /* end RbRp_Progress */


/******************************************************************************
 * RbRp_Convert()
 *
 * Implement the convert() public method for the Rappture class.
 * Convert the Ruby String fromVal, containing a numeric value and optional
 *  units, to the units specified by the Ruby String toUnitsName.
 * If the Ruby Fixnum showUnits is 1, return a Ruby String with units appended,
 *  else return a Ruby Float (double).
 * On error, return Qnil, or, if RAISE_EXCEPTIONS is defined, raise a 
 *  RuntimeError Exception.
 ******************************************************************************/

static VALUE
RbRp_Convert(VALUE self, VALUE fromVal, VALUE toUnitsName, VALUE showUnits) 
{
   VALUE retVal = Qnil;
   RpLibrary *lib;
   std::string strRetVal;
   int result;

   /* Extract the pointer to the Rappture object, lib, from the Ruby 
      object, self */
   Data_Get_Struct(self, RpLibrary, lib);

   /* Convert */
   strRetVal = RpUnits::convert(STR2CSTR(fromVal), STR2CSTR(toUnitsName), 
                                NUM2INT(showUnits), &result);
   /* Return value */
   if (0 == result)
   {
      const char *retCStr = strRetVal.c_str();

      if (NUM2INT(showUnits))  /* Ruby String */
         retVal = rb_str_new2(retCStr);
      else                     /* Ruby Float */
         retVal = rb_float_new(atof(retCStr));
   }
#ifdef RAISE_EXCEPTIONS
   else
      rb_raise(rb_eRuntimeError, "Unable to convert \"%s\" to \"%s\"", 
               STR2CSTR(fromVal), STR2CSTR(toUnitsName));
#endif
   return retVal;

}  /* end RbRp_Convert */


/******************************************************************************
 * RbRp_Delete()
 *
 * Implement the destructor for the Rappture class.
 * This function is registered with Ruby's garbage collector, which requires
 *  the void * argument.
 ******************************************************************************/
   
static void
RbRp_Delete(void *ptr)
{
   delete ((RpLibrary *)ptr);

}  /* end RbRp_Delete */


/******************************************************************************
 * RbRp_Init()
 *
 * Implement the initialize method, called from the new() method, for the 
 *  Rappture class.
 * Set the instance variable @driver (the name of the driver XML file).
 * Return self.
 ******************************************************************************/

static VALUE
RbRp_Init(VALUE self, VALUE driver)
{
   /* Set instance variable */
   rb_iv_set(self, "@driver", driver);

   /* Return a Ruby VALUE */
   return self;

}  /* end RbRp_Init */


/******************************************************************************
 * RbRp_New()
 *
 * Implement the new() method for the Rappture class.
 * Create a Rappture I/O object from the XML file driver.
 * Create a new Ruby object.
 * Associate the Rappture object and the Ruby object with the class value 
 *  classval, and garbage collection functions (mark and sweep).
 * Pass driver to the initialize method.
 * Return the new Ruby object.
 * Note this method is not static.
 ******************************************************************************/

VALUE
RbRp_New(VALUE classval, VALUE driver)
{
   /* Create the Rappture object from the XML driver file. */
   RpLibrary *lib = new RpLibrary(STR2CSTR(driver));
      
   /* Data_Wrap_Struct() creates a new Ruby object which associates the
      Rappture object, lib, with the class type classval, and registers 0
      as the marking function and RbRp_Delete() as the freeing function for
      Ruby's mark and sweep garbage collector. */
   VALUE rbLib = Data_Wrap_Struct(classval, 0, RbRp_Delete, lib);

   /* Call the initialize method; rb_obj_call_init() requires an argument
      array. */
   VALUE argv[1] = {driver};
   rb_obj_call_init(rbLib, 1, argv);

   /* Return a Ruby VALUE */
   return rbLib;

}  /* end RbRp_New */


/******************************************************************************
 * Init_Rappture()
 *
 * This is the first function called when Ruby loads the Rappture extension.
 * Create the Rappture class, add constants, and register methods.
 ******************************************************************************/

void
Init_Rappture(void)
{
   /* Create the Rappture class as a sublcass of Ruby's Object class. */
   classRappture = rb_define_class("Rappture", rb_cObject);

   /* Add constants to the Rappture class, accessible via, e.g., 
      "Rappture::APPEND"*/
   rb_define_const(classRappture, "APPEND", INT2NUM(RPLIB_APPEND));
   rb_define_const(classRappture, "OVERWRITE", INT2NUM(RPLIB_OVERWRITE));
   rb_define_const(classRappture, "UNITS_ON", INT2NUM(RPUNITS_UNITS_ON));
   rb_define_const(classRappture, "UNITS_OFF", INT2NUM(RPUNITS_UNITS_OFF));
   rb_define_const(classRappture, "COMPRESS", INT2NUM(RPLIB_COMPRESS));
   rb_define_const(classRappture, "NO_COMPRESS", INT2NUM(RPLIB_NO_COMPRESS));

   /* Function pointer cast necessary for C++ (but not C) */
#  define RB_FUNC(func)  (VALUE (*)(ANYARGS))func

   /* Register methods; last argument gives the expected number of arguments, 
      not counting self (i.e. number of arguments from Ruby code) */
   rb_define_singleton_method(classRappture, "new", RB_FUNC(RbRp_New),    1);
   rb_define_method(classRappture, "initialize", RB_FUNC(RbRp_Init),      1);
   rb_define_method(classRappture, "get",        RB_FUNC(RbRp_GetString), 1);
   rb_define_method(classRappture, "getdata",    RB_FUNC(RbRp_GetData),   1);
   rb_define_method(classRappture, "put",        RB_FUNC(RbRp_PutObject), 3);
   rb_define_method(classRappture, "putdata",    RB_FUNC(RbRp_PutData),   3);
   rb_define_method(classRappture, "putfile",    RB_FUNC(RbRp_PutFile),   4);
   rb_define_method(classRappture, "result",     RB_FUNC(RbRp_Result),    1);
   rb_define_method(classRappture, "xml",        RB_FUNC(RbRp_Xml),       0);
   rb_define_method(classRappture, "convert",    RB_FUNC(RbRp_Convert),   3);
   rb_define_method(classRappture, "progress",   RB_FUNC(RbRp_Progress),  2);

}  /* end Init_Rappture */

/* TODO rpElement*(), rpChildren*() */

