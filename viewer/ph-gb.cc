#include <libguile.h>

#include <QtWidgets>

#include <libcdgbs/SurfGBS.hpp>

#include "ph-gb.hh"

PHGB::PHGB(std::string filename) : Object(filename) {
  reload();
}

PHGB::~PHGB() {
}

void PHGB::draw(const Visualization &vis) const {
  // Cage
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

  SCM onePatch = scm_variable_ref(scm_c_lookup("only-one-patch"));
  size_t show_only = 0;
  if (onePatch != SCM_BOOL_F)
    show_only = scm_to_uint(onePatch);

  // Rest as in Object::draw()

  glPolygonMode(GL_FRONT_AND_BACK,
                !vis.show_solid && vis.show_wireframe ? GL_LINE : GL_FILL);
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(1, 1);

  if (vis.show_solid || vis.show_wireframe) {
    if (vis.type == VisType::PLAIN)
      glColor3d(1.0, 1.0, 1.0);
    else if (vis.type == VisType::ISOPHOTES) {
      glBindTexture(GL_TEXTURE_2D, vis.current_isophote_texture);
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
      glEnable(GL_TEXTURE_2D);
      glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
      glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
      glEnable(GL_TEXTURE_GEN_S);
      glEnable(GL_TEXTURE_GEN_T);
    } else if (vis.type == VisType::SLICING) {
      glBindTexture(GL_TEXTURE_1D, vis.slicing_texture);
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
      glEnable(GL_TEXTURE_1D);
    }
    for (auto f : mesh.faces()) {
      if (show_only > 0 && mesh.data(f).group != show_only)
        continue;
      glBegin(GL_POLYGON);
      for (auto v : f.vertices()) {
        if (vis.type == VisType::MEAN)
          glColor3dv(vis.colorMap(vis.mean_min, vis.mean_max, mesh.data(v).mean).data());
        else if (vis.type == VisType::SLICING)
          glTexCoord1d(mesh.point(v) | vis.slicing_dir * vis.slicing_scaling);
        glNormal3dv(mesh.normal(v).data());
        glVertex3dv(mesh.point(v).data());
      }
      glEnd();
    }
    if (vis.type == VisType::ISOPHOTES) {
      glDisable(GL_TEXTURE_GEN_S);
      glDisable(GL_TEXTURE_GEN_T);
      glDisable(GL_TEXTURE_2D);
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    } else if (vis.type == VisType::SLICING)
      glDisable(GL_TEXTURE_1D);
  }

  if (vis.show_solid && vis.show_wireframe) {
    glPolygonMode(GL_FRONT, GL_LINE);
    glColor3d(0.0, 0.0, 0.0);
    glDisable(GL_LIGHTING);
    for (auto f : mesh.faces()) {
      glBegin(GL_POLYGON);
      for (auto v : f.vertices())
        glVertex3dv(mesh.point(v).data());
      glEnd();
    }
    glEnable(GL_LIGHTING);
  }
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void PHGB::drawWithNames(const Visualization &vis) const {
}

Vector PHGB::postSelection(int selected) {
  return {};
}

void PHGB::movement(int selected, const Vector &pos) {
}

namespace {

  // Based on the one in OpenMesh's STL reader
  class CmpVec {
  public:
    explicit CmpVec(double eps) : eps(eps) { }
    bool operator()(const Vector &v0, const Vector &v1) const {
      if (std::abs(v0[0] - v1[0]) <= eps) {
        if (std::abs(v0[1] - v1[1]) <= eps)
          return v0[2] < v1[2] - eps;
        else
          return v0[1] < v1[1] - eps;
      } else
        return v0[0] < v1[0] - eps;
    }
  private:
    double eps;
  };

  template <typename T>
  void mergeMeshes(BaseMesh &m1, const T &m2, size_t id) {
    CmpVec comp(1e-10);
    std::map<Vector, BaseMesh::VertexHandle, CmpVec> vmap(comp);
    std::map<typename T::VertexHandle, BaseMesh::VertexHandle> hmap;
    for (auto v : m1.vertices())
      vmap[m1.point(v)] = v;
    for (auto v : m2.vertices()) {
      auto it = vmap.find(Vector(m2.point(v)));
      if (it != vmap.end())
        hmap[v] = it->second;
      else
        hmap[v] = m1.add_vertex(Vector(m2.point(v)));
    }
    for (auto f : m2.faces()) {
      std::vector<BaseMesh::VertexHandle> face;
      for (auto v : f.vertices())
        face.push_back(hmap.at(v));
      auto fh = m1.add_face(face);
      if (m1.is_valid_handle(fh))
        m1.data(fh).group = id;
    }
  }

}

void PHGB::updateBaseMesh() {
  size_t resolution = scm_to_uint(scm_variable_ref(scm_c_lookup("resolution")));
  mesh.clear();
  double large = std::numeric_limits<double>::max();
  Vector box_min(large, large, large), box_max(-large, -large, -large);
  for (auto v : cage.vertices()) {
    const auto &p = cage.point(v);
    box_min.minimize(p);
    box_max.maximize(p);
  }
  mesh.add_vertex(box_min);
  mesh.add_vertex(box_max);
  double edge_size = (box_max - box_min).norm() / resolution;
  size_t id = 0;
  for (const auto &patch : patches) {
    libcdgbs::Mesh patch_mesh;
    libcdgbs::SurfGBS surf;
    std::vector<std::vector<libcdgbs::SurfGBS::Ribbon>> ribbons(1);
    for (const auto &r : patch) {
      Geometry::PointVector cpts;
      for (size_t j = 0; j < r[0].size(); ++j)
        for (size_t i = 0; i < 2; ++i)
          cpts.push_back(Geometry::Point3D(r[i][j].data()));
      ribbons[0].emplace_back(3, 1, cpts);
    }
    surf.load_ribbons_and_evaluate(ribbons, edge_size, patch_mesh);
    mergeMeshes(mesh, patch_mesh, ++id);
  }
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
