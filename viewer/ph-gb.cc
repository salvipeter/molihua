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
    glDisable(GL_LIGHTING);
    glLineWidth(3.0);
    glColor3d(0.3, 0.3, 1.0);
    for (auto f : cage.faces()) {
      glBegin(GL_LINE_LOOP);
      for (auto v : f.vertices())
        glVertex3dv(cage.point(v).data());
      glEnd();
    }
    glLineWidth(1.0);
    glEnable(GL_LIGHTING);
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
  for (auto v : cage.vertices())
    mesh.add_vertex(cage.point(v));
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

  // Extract cage
  SCM vertices = scm_variable_ref(scm_c_lookup("vertices"));
  SCM faces = scm_variable_ref(scm_c_lookup("faces"));
  size_t n_vertices = scm_to_uint(scm_vector_length(vertices));
  size_t n_faces = scm_to_uint(scm_vector_length(faces));
  std::vector<CageMesh::VertexHandle> handles;
  for (size_t i = 0; i < n_vertices; ++i) {
    Vector p;
    SCM lst = scm_vector_ref(vertices, scm_from_uint(i));
    for (size_t j = 0; j < 3; ++j)
      p[j] = scm_to_double(scm_list_ref(lst, scm_from_uint(j)));
    handles.push_back(cage.add_vertex(p));
  }
  for (size_t i = 0; i < n_faces; ++i) {
    SCM lst = scm_vector_ref(faces, scm_from_uint(i));
    size_t n = scm_to_uint(scm_length(lst));
    std::vector<CageMesh::VertexHandle> face;
    for (size_t j = 0; j < n; ++j)
      face.push_back(handles[scm_to_uint(scm_list_ref(lst, scm_from_uint(j)))]);
    cage.add_face(face);
  }
  
  // Extract ribbons
  SCM ribbons = scm_variable_ref(scm_c_lookup("ribbons"));
  patches.resize(n_faces);
  for (size_t i = 0; i < n_faces; ++i) {
    SCM lst = scm_vector_ref(ribbons, scm_from_uint(i));
    size_t n = scm_to_uint(scm_length(lst));
    patches[i].resize(n);
    for (size_t j = 0; j < n; ++j) {
      SCM ribbon = scm_list_ref(lst, scm_from_uint(j));
      std::array<SCM, 2> curves = { scm_car(ribbon), scm_cdr(ribbon) };
      for (size_t k = 0; k < 2; ++k) {
        patches[i][j][k].resize(4);
        for (size_t l = 0; l <= 3; ++l) {
          SCM point = scm_list_ref(curves[k], scm_from_uint(l));
          for (size_t c = 0; c < 3; ++c)
            patches[i][j][k][l][c] = scm_to_double(scm_list_ref(point, scm_from_uint(c)));
        }
      }
    }
  }

  updateBaseMesh();
  return true;
}
