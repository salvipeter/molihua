#include <fstream>

#include <libguile.h>

#include <QtWidgets>

#include "ph-gb.hh"

PHGB::PHGB(std::string filename) : Object(filename) {
  reload();
}

PHGB::~PHGB() {
}

void PHGB::draw(const Visualization &vis) const {
  Object::draw(vis);
  if (vis.show_control_points) {
    // TODO: draw cage
  }
}

void PHGB::drawWithNames(const Visualization &vis) const {
}

Vector PHGB::postSelection(int selected) {
  return {};
}

void PHGB::movement(int selected, const Vector &pos) {
}

void PHGB::updateBaseMesh() {
  mesh.clear();
  mesh.add_vertex({0, 0, 0});
  // TODO: call libcdgbs
  Object::updateBaseMesh(false, false);
}

SCM safeLoad(void *data) {
  scm_c_primitive_load(reinterpret_cast<std::string *>(data)->c_str());
  return SCM_UNSPECIFIED;
}

SCM errorHandler(void *, SCM key, SCM args) {
  auto display_fun = scm_c_public_ref("guile", "display");
  key = scm_object_to_string(key, display_fun);
  args = scm_object_to_string(args, display_fun);
  char *key_str = scm_to_stringn(key, nullptr, "UTF-8", SCM_FAILED_CONVERSION_QUESTION_MARK);
  char *args_str = scm_to_stringn(args, nullptr, "UTF-8", SCM_FAILED_CONVERSION_QUESTION_MARK);
  auto port = scm_open_output_string();
  auto stack = scm_make_stack(SCM_BOOL_T, SCM_EOL);
  scm_display_backtrace(stack, port, SCM_BOOL_F, SCM_BOOL_F);
  auto bt = scm_get_output_string(port);
  char *bt_str = scm_to_stringn(bt, nullptr, "UTF-8", SCM_FAILED_CONVERSION_QUESTION_MARK);
  QString text = QString("Exception: ") + key_str + "\n" + args_str + "\nBacktrace:\n" + bt_str;
  QMessageBox::critical(nullptr, "Script Error", text);
  free(bt_str);
  free(args_str);
  free(key_str);
  return SCM_UNSPECIFIED;
}

SCM dummyHandler(void *, SCM, SCM) {
  return SCM_UNSPECIFIED;
}

bool PHGB::reload() {
  scm_c_catch(SCM_BOOL_T, safeLoad, reinterpret_cast<void *>(const_cast<std::string *>(&filename)),
              dummyHandler, nullptr, errorHandler, nullptr);
  // TODO: extract cage & ribbons
  updateBaseMesh();
  return true;
}
