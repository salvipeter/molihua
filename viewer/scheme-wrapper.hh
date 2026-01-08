#pragma once

#include <string>

#ifdef USE_GUILE_SCHEME
  #include <libguile.h>
#elif defined(USE_S7_SCHEME)
  #include <s7.h>
#else // USE_CHIBI_SCHEME
  #ifdef slots
    #pragma push_macro("slots")
    #undef slots
    #define RESTORE_SLOTS
  #endif
  #include <chibi/eval.h>
  #ifdef RESTORE_SLOTS
    #pragma pop_macro("slots")
    #undef RESTORE_SLOTS
  #endif
#endif

namespace SchemeWrapper {

#ifdef USE_GUILE_SCHEME
  using Sexp = SCM;
#elif defined(USE_S7_SCHEME)
  using Sexp = s7_pointer;
#else // USE_CHIBI_SCHEME
  using Sexp = sexp;
#endif

  void initialize();

  void setVariable(std::string var, Sexp val);
  Sexp getVariable(std::string var);

  bool isNull(Sexp s);
  bool isPair(Sexp s);
  bool isFalse(Sexp s);

  unsigned int sexp2uint(Sexp s);
  double sexp2double(Sexp s);
  Sexp bool2sexp(bool b);
  Sexp uint2sexp(unsigned int n);
  Sexp double2sexp(double x);
  Sexp string2symbol(std::string s);

  Sexp car(Sexp cons);
  Sexp cdr(Sexp cons);
  unsigned int listLength(Sexp lst);
  Sexp listElement(Sexp lst, unsigned int i);
  unsigned int vectorLength(Sexp vec);
  Sexp vectorElement(Sexp vec, unsigned int i);

  Sexp evaluateString(std::string s);

}
