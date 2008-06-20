/* -----------------------------------------------------------------------------
 * See the LICENSE file for information on copyright, usage and redistribution
 * of SWIG, and the README file for authors - http://www.swig.org/release.html.
 *
 * c.cxx
 *
 * C language module for SWIG.
 * ----------------------------------------------------------------------------- */

char cvsroot_c_cxx[] = "$Id$";

#include "swigmod.h"

class C:public Language {
  static const char *usage;

  File *f_runtime;
  File *f_header;
  File *f_wrappers;
  File *f_init;
  File *f_shadow_c;
  File *f_shadow_h;

  String *f_shadow_code_init;
  String *f_shadow_code_body;
  String *f_shadow_header;

  String *empty_string;

  bool shadow_flag;

public:

  /* -----------------------------------------------------------------------------
   * C()
   * ----------------------------------------------------------------------------- */

  C() : 
    empty_string(NewString("")),
    shadow_flag(true) {
  }

  /* ------------------------------------------------------------
   * main()
   * ------------------------------------------------------------ */

  virtual void main(int argc, char *argv[]) {

    SWIG_library_directory("c");

    // Add a symbol to the parser for conditional compilation
    Preprocessor_define("SWIGC 1", 0);

    // Add typemap definitions
    SWIG_typemap_lang("c");
    SWIG_config_file("c.swg");

    // Look for certain command line options
    for (int i = 1; i < argc; i++) {
      if (argv[i]) {
        if (strcmp(argv[i], "-help") == 0) {
          Printf(stdout, "%s\n", usage);
        } else if ((strcmp(argv[i], "-shadow") == 0) || (strcmp(argv[i], "-proxy") == 0)) {
          shadow_flag = true;
        } else if (strcmp(argv[i], "-noproxy") == 0) {
          shadow_flag = false;
        }
      }
    }
  }

  void emitSwigExport(File *f) {
    Printf(f, "#ifndef SWIGEXPORT\n");
    Printf(f, "# if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)\n");
    Printf(f, "#   if defined(STATIC_LINKED)\n");
    Printf(f, "#     define SWIGEXPORT\n");
    Printf(f, "#   else\n");
    Printf(f, "#     define SWIGEXPORT __declspec(dllexport)\n");
    Printf(f, "#   endif\n");
    Printf(f, "# else\n");
    Printf(f, "#   if defined(__GNUC__) && defined(GCC_HASCLASSVISIBILITY)\n");
    Printf(f, "#     define SWIGEXPORT __attribute__ ((visibility(\"default\")))\n");
    Printf(f, "#   else\n");
    Printf(f, "#     define SWIGEXPORT\n");
    Printf(f, "#   endif\n");
    Printf(f, "# endif\n");
    Printf(f, "#endif\n\n");
  }

  void emitSwigImport(File *f) {
    Printf(f, "#ifndef SWIGIMPORT\n");
    Printf(f, "# ifndef __GNUC__\n");
    Printf(f, "#   define __DLL_IMPORT __declspec(dllimport)\n");
    Printf(f, "# else\n");
    Printf(f, "#   define __DLL_IMPORT __attribute__((dllimport)) extern\n");
    Printf(f, "# endif\n");
    Printf(f, "# if !defined (__WIN32__)\n");
    Printf(f, "#   define SWIGIMPORT extern\n");
    Printf(f, "# else\n");
    Printf(f, "#   define SWIGIMPORT __DLL_IMPORT\n");
    Printf(f, "# endif\n");
    Printf(f, "#endif\n\n");
  }

  /* ---------------------------------------------------------------------
   * top()
   * --------------------------------------------------------------------- */

  virtual int top(Node *n) {
    String *module = Getattr(n, "name");
    String *outfile = Getattr(n, "outfile");

    /* initialize I/O */
    f_runtime = NewFile(outfile, "w");
    if (!f_runtime) {
      FileErrorDisplay(outfile);
      SWIG_exit(EXIT_FAILURE);
    }
    f_init = NewString("");
    f_header = NewString("");
    f_wrappers = NewString("");

    Swig_banner(f_runtime);

    // FIXME
    Printf(f_header, "#include \"malloc.h\"\n\n");

    /* generate shadow files if enabled */
    if (shadow_flag) {
      f_shadow_code_init = NewString("");
      f_shadow_code_body = NewString("");
      f_shadow_header = NewString("");

      /* create shadow file with appropriate name */
      String *shadow_code_filename = NewStringf("%s%s_proxy.c", SWIG_output_directory(), Char(module));
      if ((f_shadow_c = NewFile(shadow_code_filename, "w")) == 0) {
        FileErrorDisplay(shadow_code_filename);
        SWIG_exit(EXIT_FAILURE);
      }

      String *shadow_header_filename = NewStringf("%s%s_proxy.h", SWIG_output_directory(), Char(module));
      if ((f_shadow_h = NewFile(shadow_header_filename, "w")) == 0) {
        FileErrorDisplay(shadow_header_filename);
        SWIG_exit(EXIT_FAILURE);
      }

      Swig_register_filebyname("shadow_code_init", f_shadow_code_init);
      Swig_register_filebyname("shadow_code_body", f_shadow_code_body);
      Swig_register_filebyname("shadow_header", f_shadow_header);

      Swig_banner(f_shadow_code_init);
      Swig_banner(f_shadow_header);
      emitSwigImport(f_shadow_header);
      Printf(f_shadow_code_init, "#include \"%s\"\n\n", shadow_header_filename);
    }

    Swig_register_filebyname("header", f_header);
    Swig_register_filebyname("wrappers", f_wrappers);
    Swig_register_filebyname("runtime", f_runtime);
    Swig_register_filebyname("init", f_init);

    Printf(f_wrappers, "#ifdef __cplusplus\n");
    Printf(f_wrappers, "extern \"C\" {\n");
    Printf(f_wrappers, "#endif\n\n");
    
    /* emit code for children */
    Language::top(n);

    Printf(f_wrappers, "#ifdef __cplusplus\n");
    Printf(f_wrappers, "}\n");
    Printf(f_wrappers, "#endif\n");

    /* finalize generating shadow file */
    if (shadow_flag) {
      Printv(f_shadow_c, f_shadow_code_init, "\n", NIL);
      Printv(f_shadow_c, f_shadow_code_body, "\n", NIL);
      Printv(f_shadow_h, f_shadow_header, "\n", NIL);
      Close(f_shadow_c);
      Close(f_shadow_h);
      Delete(f_shadow_code_init);
      Delete(f_shadow_header);
    }

    /* write all to file */
    Dump(f_header, f_runtime);
    Dump(f_wrappers, f_runtime);
    Wrapper_pretty_print(f_init, f_runtime);

    /* cleanup */
    Delete(f_header);
    Delete(f_wrappers);
    Delete(f_init);
    Close(f_runtime);
    Delete(f_runtime);

    return SWIG_OK;
  }

  /* -----------------------------------------------------------------------
   * globalvariableHandler()
   * ------------------------------------------------------------------------ */  

  virtual int globalvariableHandler(Node *n) {
    SwigType *type = Getattr(n, "type");
    String *type_str = SwigType_str(type, 0);
    Printv(f_wrappers, "SWIGEXPORT ", type_str, " ", Getattr(n, "name"), ";\n", NIL);
    if (shadow_flag) {
      Printv(f_shadow_header, "SWIGIMPORT ", type_str, " ", Getattr(n, "name"), ";\n", NIL);
    }
    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------
   * functionWrapper()
   * ---------------------------------------------------------------------- */
  
  virtual int functionWrapper(Node *n) {
    String *name = Getattr(n, "sym:name");
    SwigType *return_type = Getattr(n, "type");
    String *return_type_str = SwigType_str(return_type, 0);
    String *arg_names = NewString("");
    ParmList *parms = Getattr(n, "parms");

    /* create new function wrapper object */
    Wrapper *wrapper = NewWrapper();

    /* create new wrapper name */
    String *wname = Swig_name_wrapper(name);
    Setattr(n, "wrap:name", wname);

    /* create wrapper function prototype */
    Printv(wrapper->def, "SWIGEXPORT ", return_type_str, " ", wname, "(", NIL);

    /* attach the standard typemaps */
    emit_attach_parmmaps(parms, wrapper);

    /* prepare parameter list */
    Parm *p, *np;
    for (p = parms; p; ) {
      np = nextSibling(p);
      SwigType *type = Getattr(p, "type");
      Printv(wrapper->def, SwigType_str(type, 0), " ", Getattr(p, "lname"), np ? ", " : "", NIL);
      Printv(arg_names, Getattr(p, "name"), np ? ", " : "", NIL);
      p = np;
    }
    Printv(wrapper->def, ") {", NIL);

    /* declare wrapper function local variables */
    emit_return_variable(n, return_type, wrapper);

    /* emit action code */
    String *action = emit_action(n);
    Append(wrapper->code, action);
    if (return_type && Strcmp(return_type, "void") != 0)
      Append(wrapper->code, "return result;\n");

    Append(wrapper->code, "}\n");
    Wrapper_print(wrapper, f_wrappers);

    /* take care of shadow function */
    if (shadow_flag) {
      String *proto = ParmList_str(parms);

      Printv(f_shadow_code_init, "extern ", return_type_str, " _wrap_", name, "(", proto, ");\n", NIL);
      Printv(f_shadow_code_body, return_type_str, " ", name, "(", proto, ") {\n", NIL);

      /* handle 'prepend' feature */
      String *prepend_str = Getattr(n, "feature:prepend");
      if (prepend_str) {
        char *t = Char(prepend_str);
        if (*t == '{') {
          Delitem(prepend_str, 0);
          Delitem(prepend_str, DOH_END);
        }
        Printv(f_shadow_code_body, prepend_str, "\n", NIL);
      }

      /* call to the wrapper function */
      Printv(f_shadow_code_body, "  return ", wname, "(", arg_names, ");\n", NIL);

      /* handle 'append' feature */
      String *append_str = Getattr(n, "feature:append");
      if (append_str) {
        char *t = Char(append_str);
        if (*t == '{') {
          Delitem(append_str, 0);
          Delitem(append_str, DOH_END);
          }
        Printv(f_shadow_code_body, append_str, "\n", NIL);
      }

      Printv(f_shadow_code_body, "}\n", NIL);

      /* add function declaration to the proxy header file */
      Printv(f_shadow_header, return_type_str, " ", name, "(", proto, ");\n");
    }

    Delete(arg_names);
    Delete(wname);
    DelWrapper(wrapper);

    return SWIG_OK;
  }

  /* ---------------------------------------------------------------------
   * classDeclaration()
   * --------------------------------------------------------------------- */

  virtual int classHandler(Node* n) {
    String* name = Getattr(n, "name");
    String* sobj_name = NewString("");
    Printv(sobj_name, "struct ", name, "Obj", NIL);
    Printv(f_header, sobj_name, "{\n  void* obj;\n};\n\n", NIL);

    if (shadow_flag) {
      Printv(f_shadow_header, sobj_name, ";\n\n", NIL);
    }

    Delete(sobj_name);
    return Language::classHandler(n);
  }

  /* ---------------------------------------------------------------------
   * memberfunctionHandler()
   * --------------------------------------------------------------------- */

  virtual int memberfunctionHandler(Node* n) {
    return SWIG_OK;
  }

  /* ---------------------------------------------------------------------
   * variableHandler()
   * --------------------------------------------------------------------- */

  virtual int variableHandler(Node* n) {
    if (Cmp(Getattr(n, "ismember"), "1") != 0)
      return globalvariableHandler(n);
    return SWIG_OK;
  }

  /* ---------------------------------------------------------------------
   * constructorHandler()
   * --------------------------------------------------------------------- */

  virtual int constructorHandler(Node* n) {
    String* sobj_name = NewString("");
    String* ctype = NewString("");
    String* code = NewString("");
    String* new_name = NewString("");
    String* arg_lnames = NewString("");
    ParmList* parms;
    Parm* p, *np;

    parms = Getattr(n, "parms");
    int i = 1;
    for (p = parms; p; ) {
      np = nextSibling(p);
      String* name = NewString("");
      Printf(name, "arg%d", i++);
      Setattr(p, "lname", name);
      Delete(name);
      Printv(arg_lnames, name, np ? ", " : "", NIL);
      p = np;
    }

    Printv(sobj_name, "struct ", Getattr(n, "name"), "Obj", NIL);
    ctype = Copy(sobj_name);
    SwigType_add_pointer(ctype);
    Printv(code, "result = (", sobj_name, "*) malloc(sizeof(", sobj_name, "));\n", NIL);
    Printv(code, "result->obj = (void*) new ", Getattr(n, "name"), "(", arg_lnames, ");\n", NIL);
    Setattr(n, "wrap:action", code);
    Setattr(n, "type", ctype);
    Printv(new_name, "new_", Getattr(n, "sym:name"), NIL);
    Setattr(n, "sym:name", new_name);

    functionWrapper(n);

    Delete(arg_lnames);
    Delete(new_name);
    Delete(code);
    Delete(ctype);
    Delete(sobj_name);
    return SWIG_OK;
  }

  /* ---------------------------------------------------------------------
   * destructorHandler()
   * --------------------------------------------------------------------- */

  virtual int destructorHandler(Node* n) {
    String* sobj_name = NewString("");
    String* ctype = NewString("");
    String* code = NewString("");
    String* new_name = NewString("");
    Parm* p;

    Printv(sobj_name, "struct ", Getattr(n, "sym:name"), "Obj", NIL);
    ctype = Copy(sobj_name);
    SwigType_add_pointer(ctype);

    p = NewParm(ctype, "self");
    Setattr(p, "lname", "arg1");
    Setattr(n, "parms", p);

    Setattr(n, "type", "void");
    Printv(code, "delete ((", sobj_name, "*) arg1->obj);\nfree(arg1);\n", NIL);
    Setattr(n, "wrap:action", code);
    Printv(new_name, "delete_", Getattr(n, "sym:name"), NIL);
    Setattr(n, "sym:name", new_name);
    
    functionWrapper(n);

    Delete(p);
    Delete(new_name);
    Delete(code);
    Delete(ctype);
    Delete(sobj_name);
    return SWIG_OK;
  }

};				/* class C */

/* -----------------------------------------------------------------------------
 * swig_c()    - Instantiate module
 * ----------------------------------------------------------------------------- */

static Language *new_swig_c() {
  return new C();
}

extern "C" Language *swig_c(void) {
  return new_swig_c();
}

/* -----------------------------------------------------------------------------
 * Static member variables
 * ----------------------------------------------------------------------------- */

const char *C::usage = (char *) "\
C Options (available with -c)\n\
\n";
