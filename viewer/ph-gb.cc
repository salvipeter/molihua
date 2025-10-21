#include <libguile.h>

#include <QtWidgets>

#include <libcdgbs/SurfGBS.hpp>

#include "options.hh"
#include "ph-gb.hh"

PHGB::PHGB(std::string filename) : Object(filename) {
  reload();
}

PHGB::~PHGB() {
}

void PHGB::draw(const Visualization &vis) const {
  Object::draw(vis);

  SCM onePatch = scm_variable_ref(scm_c_lookup("only-one-patch"));
  size_t show_only = 0;
  if (onePatch != SCM_BOOL_F)
    show_only = scm_to_uint(onePatch);

  if (vis.show_cage) {
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

  if (vis.show_offsets) {
    glDisable(GL_LIGHTING);
    glLineWidth(3.0);
    glColor3d(1.0, 0.7, 0.0);
    for (size_t i = 0; i < offset_faces.size(); ++i) {
      if (show_only > 0 && show_only - 1 != i)
        continue;
      const auto &fs = offset_faces[i];
      for (const auto &f : fs) {
        glBegin(GL_LINE_LOOP);
        for (auto v : f)
          glVertex3dv(offset_vertices[v].data());
        glEnd();
      }
    }
    glLineWidth(1.0);
    glEnable(GL_LIGHTING);
  }

  if (vis.show_chamfers) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4d(1.0, 0.0, 0.7, 0.3);
    auto n_chamfers = chamfers.size();
    for (size_t i = 0; i < n_chamfers; ++i) {
      bool skip = show_only > 0;
      if (skip)
        for (auto vh : cage.fv_range(cage.face_handle(show_only - 1)))
          if (vh.idx() == (int)i) {
            skip = false;
            break;
          }
      if (skip)
        continue;
      const auto &f = chamfers[i];
      glBegin(GL_POLYGON);
      for (auto v : f)
        glVertex3dv(offset_vertices[v].data());
      glEnd();
    }
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
  }

  if (vis.boundaries != Visualization::BoundaryType::NONE) {
    glDisable(GL_LIGHTING);
    glLineWidth(3.0);
    glColor3d(0.0, 0.7, 0.7);
    for (size_t i = 0; i < patches.size(); ++i) {
      if (show_only > 0 && show_only - 1 != i)
        continue;
      const auto &patch = patches[i];
      for (const auto &loop : patch)
        for (const auto &ribbon : loop) {
          glBegin(GL_LINE_STRIP);
          if (vis.boundaries == Visualization::BoundaryType::CURVE) {
            size_t resolution = 100;
            for (size_t i = 0; i <= resolution; ++i) {
              double u = (double)i / resolution;
              auto tmp = ribbon[0];
              size_t n = tmp.size() - 1;
              for (size_t k = 1; k <= n; ++k)
                for (size_t i = 0; i <= n - k; ++i)
                  tmp[i] = tmp[i] * (1 - u) + tmp[i+1] * u;
              glVertex3dv(tmp[0].data());
            }
          } else {
            // Just the control points
            for (const auto &p : ribbon[0])
              glVertex3dv(p.data());
          }
          glEnd();
        }
    }
    glLineWidth(1.0);
    glEnable(GL_LIGHTING);
  }

  if (vis.show_control_points) {
    glDisable(GL_LIGHTING);
    glLineWidth(3.0);
    glColor3d(0.0, 1.0, 0.0);
    for (size_t i = 0; i < patches.size(); ++i) {
      if (show_only > 0 && show_only - 1 != i)
        continue;
      const auto &patch = patches[i];
      for (const auto &loop : patch)
        for (const auto &ribbon : loop) {
          for (const auto &row : ribbon) {
            glBegin(GL_LINE_STRIP);
            for (const auto &p : row)
              glVertex3dv(p.data());
            glEnd();
          }
          for (size_t j = 0; j < ribbon[0].size(); ++j) {
            glBegin(GL_LINE_STRIP);
            glVertex3dv(ribbon[0][j].data());
            glVertex3dv(ribbon[1][j].data());
            glEnd();
          }
        }
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
  QApplication::setOverrideCursor(Qt::WaitCursor);
  size_t resolution = scm_to_uint(scm_variable_ref(scm_c_lookup("resolution")));
  mesh.clear();
  double large = std::numeric_limits<double>::max();
  Vector box_min(large, large, large), box_max(-large, -large, -large);
  for (auto v : cage.vertices()) {
    const auto &p = cage.point(v);
    box_min.minimize(p);
    box_max.maximize(p);
  }
  double edge_size = (box_max - box_min).norm() / resolution;
  size_t id = 0;
  for (const auto &patch : patches) {
    size_t n_loops = patch.size();
    libcdgbs::Mesh patch_mesh;
    libcdgbs::SurfGBS surf;
    std::vector<std::vector<libcdgbs::SurfGBS::Ribbon>> ribbons(n_loops);
    for (size_t loop = 0; loop < n_loops; ++loop) {
      for (const auto &r : patch[loop]) {
        Geometry::PointVector cpts;
        auto deg = r[0].size() - 1;
        for (size_t j = 0; j <= deg; ++j)
          for (size_t i = 0; i < 2; ++i)
            cpts.push_back(Geometry::Point3D(r[i][j].data()));
        ribbons[loop].emplace_back(deg, 1, cpts);
      }
    }
    if (Options::instance()->reparam())
      surf.load_ribbons_and_evaluate(ribbons, edge_size, patch_mesh, true,
                                     *Options::instance()->reparam(),
                                     Options::instance()->hsplit(),
                                     Options::instance()->C1());
    else
      surf.load_ribbons_and_evaluate(ribbons, edge_size, patch_mesh, false);
    mergeMeshes(mesh, patch_mesh, ++id);
  }
  Object::updateBaseMesh(false, false);
  QApplication::restoreOverrideCursor();
}

SCM safeLoad(void *data) {
  auto filename = reinterpret_cast<std::string *>(data);
  scm_c_primitive_load("molihua.scm");
  scm_c_eval_string((std::string("(load-model \"") + *filename + "\")").c_str());
  return SCM_UNSPECIFIED;
}

static bool load_ok;

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
  load_ok = false;
  return SCM_UNSPECIFIED;
}

SCM dummyHandler(void *, SCM, SCM) {
  return SCM_UNSPECIFIED;
}

bool PHGB::reload() {
  load_ok = true;
  scm_c_catch(SCM_BOOL_T, safeLoad, reinterpret_cast<void *>(const_cast<std::string *>(&filename)),
              dummyHandler, nullptr, errorHandler, nullptr);
  if (!load_ok)
    return false;

  size_t n_vertices, n_faces;

  // Extract cage
  {
    SCM vertices = scm_variable_ref(scm_c_lookup("vertices"));
    SCM faces = scm_variable_ref(scm_c_lookup("faces"));
    n_vertices = scm_to_uint(scm_vector_length(vertices));
    n_faces = scm_to_uint(scm_vector_length(faces));
    std::vector<CageMesh::VertexHandle> handles;
    for (size_t i = 0; i < n_vertices; ++i) {
      Vector p;
      SCM lst = scm_vector_ref(vertices, scm_from_uint(i));
      for (size_t j = 0; j < 3; ++j)
        p[j] = scm_to_double(scm_list_ref(lst, scm_from_uint(j)));
      handles.push_back(cage.add_vertex(p));
    }
    for (size_t i = 0; i < n_faces; ++i) {
      SCM loops = scm_vector_ref(faces, scm_from_uint(i));
      size_t m = scm_to_uint(scm_length(loops));
      for (size_t l = 0; l < m; ++l) {
        SCM lst = scm_list_ref(loops, scm_from_uint(l));
        size_t n = scm_to_uint(scm_length(lst));
        std::vector<CageMesh::VertexHandle> face;
        for (size_t j = 0; j < n; ++j)
          face.push_back(handles[scm_to_uint(scm_list_ref(lst, scm_from_uint(j)))]);
        cage.add_face(face);
      }
    }
  }

  // Extract ribbons
  {
    SCM ribbons = scm_variable_ref(scm_c_lookup("ribbons"));
    patches.resize(n_faces);
    for (size_t i = 0; i < n_faces; ++i) {
      SCM loops = scm_vector_ref(ribbons, scm_from_uint(i));
      size_t m = scm_to_uint(scm_length(loops));
      patches[i].resize(m);
      for (size_t l = 0; l < m; ++l) {
        SCM loop = scm_list_ref(loops, scm_from_uint(l));
        size_t n = scm_to_uint(scm_length(loop));
        patches[i][l].resize(n);
        for (size_t j = 0; j < n; ++j) {
          SCM ribbon = scm_list_ref(loop, scm_from_uint(j));
          std::array<SCM, 2> curves = { scm_car(ribbon), scm_cdr(ribbon) };
          for (size_t k = 0; k < 2; ++k) {
            patches[i][l][j][k].clear();
            while (scm_null_p(curves[k]) == SCM_BOOL_F) {
              SCM point = scm_car(curves[k]);
              Vector p;
              for (size_t c = 0; c < 3; ++c)
                p[c] = scm_to_double(scm_list_ref(point, scm_from_uint(c)));
              patches[i][l][j][k].push_back(p);
              curves[k] = scm_cdr(curves[k]);
            }
          }
        }
      }
    }
  }

  // Extract offsets & chamfers
  {
    SCM vertices = scm_variable_ref(scm_c_lookup("offset-vertices"));
    SCM faces = scm_variable_ref(scm_c_lookup("offset-faces"));
    size_t n_off_vertices = scm_to_uint(scm_vector_length(vertices));
    offset_vertices.resize(n_off_vertices);
    offset_faces.resize(n_faces);
    chamfers.resize(n_vertices);
    for (size_t i = 0; i < n_off_vertices; ++i) {
      auto &p = offset_vertices[i];
      SCM lst = scm_vector_ref(vertices, scm_from_uint(i));
      for (size_t j = 0; j < 3; ++j)
        p[j] = scm_to_double(scm_list_ref(lst, scm_from_uint(j)));
    }
    for (size_t i = 0; i < n_faces; ++i) {
      SCM loops = scm_vector_ref(faces, scm_from_uint(i));
      size_t m = scm_to_uint(scm_length(loops));
      auto &face = offset_faces[i];
      face.resize(m);
      for (size_t l = 0; l < m; ++l) {
        SCM lst = scm_list_ref(loops, scm_from_uint(l));
        size_t n = scm_to_uint(scm_length(lst));
        face[l].clear();
        for (size_t j = 0; j < n; ++j) {
          SCM index = scm_list_ref(lst, scm_from_uint(j));
          if (scm_is_pair(index)) {
            face[l].push_back(scm_to_uint(scm_car(index)));
            face[l].push_back(scm_to_uint(scm_cdr(index)));
          } else
            face[l].push_back(scm_to_uint(index));
        }
      }
    }
    for (size_t i = 0; i < n_vertices; ++i) {
      SCM chamfer = scm_c_eval_string(("(chamfer " + std::to_string(i) + ")").c_str());
      chamfers[i].clear();
      while (scm_null_p(chamfer) == SCM_BOOL_F) {
        SCM index = scm_car(chamfer);
        if (scm_is_pair(index)) {
          chamfers[i].push_back(scm_to_uint(scm_cdr(index)));
          chamfers[i].push_back(scm_to_uint(scm_car(index)));
        } else
          chamfers[i].push_back(scm_to_uint(index));
        chamfer = scm_cdr(chamfer);
      }
    }
  }

  updateBaseMesh();
  return true;
}
