#pragma once

#include <libguile.h>

namespace SchemeWrapper {

  using Sexp = SCM;

  void initialize();

  void setVariable(std::string var, Sexp val);
  Sexp getVariable(std::string var);

  bool isNull(Sexp sexp);
  bool isPair(Sexp sexp);
  bool isFalse(Sexp sexp);

  unsigned int sexp2uint(Sexp sexp);
  double sexp2double(Sexp sexp);
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
