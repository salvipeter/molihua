#include "scheme-wrapper.hh"

#include <iostream>

namespace SchemeWrapper {

  void initialize() {
    scm_init_guile();
    scm_set_current_warning_port(scm_open_output_string()); // quiet
  }

  void setVariable(std::string var, Sexp val) {
    scm_c_define(var.c_str(), val);
  }

  Sexp getVariable(std::string var) {
    return scm_variable_ref(scm_c_lookup(var.c_str()));
  }

  bool isNull(Sexp sexp) {
    return scm_null_p(sexp) == SCM_BOOL_T;
  }

  bool isPair(Sexp sexp) {
    return scm_is_pair(sexp);
  }

  bool isFalse(Sexp sexp) {
    return sexp == SCM_BOOL_F;
  }

  unsigned int sexp2uint(Sexp sexp) {
    return scm_to_uint(sexp);
  }

  double sexp2double(Sexp sexp) {
    return scm_to_double(sexp);
  }

  Sexp bool2sexp(bool b) {
    return b ? SCM_BOOL_T : SCM_BOOL_F;
  }

  Sexp uint2sexp(unsigned int n) {
    return scm_from_uint(n);
  }

  Sexp double2sexp(double x) {
    return scm_from_double(x);
  }

  Sexp string2symbol(std::string s) {
    return scm_from_utf8_symbol(s.c_str());
  }

  Sexp car(Sexp cons) {
    return scm_car(cons);
  }

  Sexp cdr(Sexp cons) {
    return scm_cdr(cons);
  }

  unsigned int listLength(Sexp lst) {
    return scm_to_uint(scm_length(lst));
  }

  Sexp listElement(Sexp lst, unsigned int i) {
    return scm_list_ref(lst, scm_from_uint(i));
  }

  unsigned int vectorLength(Sexp vec) {
    return scm_to_uint(scm_vector_length(vec));
  }

  Sexp vectorElement(Sexp vec, unsigned int i) {
    return scm_vector_ref(vec, scm_from_uint(i));
  }

  Sexp evaluateString(std::string s) {
    return scm_c_eval_string(s.c_str());
  }

}
