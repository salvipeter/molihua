#include <QtWidgets>

#include <libcdgbs/SurfGBS.hpp>

#include "options.hh"
#include "ph-gb.hh"
#include "scheme-wrapper.hh"
#include "triangulator.hh"

namespace {
  const double line_width = 3.0;
}

PHGB::PHGB(std::string filename) : Object(filename) {
  reload();
}

PHGB::~PHGB() {
}

static void bernstein(size_t n, double u, std::vector<double> &coeff) {
  coeff.clear(); coeff.reserve(n + 1);
  coeff.push_back(1.0);
  double u1 = 1.0 - u;
  for (size_t j = 1; j <= n; ++j) {
    double saved = 0.0;
    for (size_t k = 0; k < j; ++k) {
      double tmp = coeff[k];
      coeff[k] = saved + tmp * u1;
      saved = tmp * u;
    }
    coeff.push_back(saved);
  }
}

void PHGB::draw(const Visualization &vis) const {
  Object::draw(vis);

  auto onePatch = SchemeWrapper::getVariable("only-one-patch");
  size_t show_only = 0;
  if (!SchemeWrapper::isFalse(onePatch))
    show_only = SchemeWrapper::sexp2uint(onePatch);

  if (vis.cage == Visualization::CageType::NET) {
    glDisable(GL_LIGHTING);
    glLineWidth(line_width);
    glColor3d(0.3, 0.3, 1.0);
    for (auto f : cage.faces()) {
      glBegin(GL_LINE_LOOP);
      for (auto v : f.vertices())
        glVertex3dv(cage.point(v).data());
      glEnd();
    }
    glLineWidth(1.0);
    glEnable(GL_LIGHTING);
  } else if (vis.cage == Visualization::CageType::SURFACE) {
    glColor3d(0.3, 0.7, 0.4);
    for (auto f : cage_triangulated.triangles()) {
      auto a = cage_triangulated[f[0]], b = cage_triangulated[f[1]], c = cage_triangulated[f[2]];
      auto normal = ((b - a) ^ (c - a)).normalize();
      glBegin(GL_POLYGON);
      for (auto v : f) {
        glNormal3dv(normal.data());
        glVertex3dv(cage_triangulated[v].data());
      }
      glEnd();
    }
  }

  if (vis.show_offsets) {
    glDisable(GL_LIGHTING);
    glLineWidth(line_width);
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
      for (size_t j = 0; skip && j < face_indices.size(); ++j) {
        if (face_indices[j] != show_only - 1)
          continue;
        for (auto vh : cage.fv_range(cage.face_handle(j)))
          if (vh.idx() == (int)i) {
            skip = false;
            break;
          }
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
    glLineWidth(line_width);
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

  if (vis.ribbons != Visualization::RibbonType::NONE) {
    glDisable(GL_LIGHTING);
    glLineWidth(line_width);
    for (size_t i = 0; i < patches.size(); ++i) {
      if (show_only > 0 && show_only - 1 != i)
        continue;
      const auto &patch = patches[i];
      for (const auto &loop : patch)
        for (size_t ri = 0; ri < loop.size(); ++ri) {
          const auto &ribbon = loop[ri];
          glColor3dv(Visualization::HSV2RGB({360.0 * ri / loop.size(), 1, 1}).data());
          if (vis.ribbons == Visualization::RibbonType::NET) {
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
          } else {
            // Ribbon surface visualization
            std::vector<double> coeff_s, coeff_h;
            size_t ds = ribbon[0].size() - 1, dh = 1;
            for (size_t hi = 1; hi <= vis.ribbon_hres; ++hi) {
              double h = vis.ribbon_hmax * hi / vis.ribbon_hres;
              bernstein(dh, h, coeff_h);
              glBegin(GL_LINE_STRIP);
              for (size_t si = 0; si <= vis.ribbon_sres; ++si) {
                double s = 1.0 * si / vis.ribbon_sres;
                bernstein(ds, s, coeff_s);
                Vector p(0.0, 0.0, 0.0);
                for (size_t k = 0; k <= dh; ++k)
                  for (size_t l = 0; l <= ds; ++l)
                    p += ribbon[k][l] * coeff_h[k] * coeff_s[l];
                glVertex3dv(p.data());
              }
              glEnd();
            }
            for (size_t si = 0; si <= vis.ribbon_sres; ++si) {
              double s = 1.0 * si / vis.ribbon_sres;
              bernstein(ds, s, coeff_s);
              glBegin(GL_LINE_STRIP);
              for (size_t hi = 0; hi <= vis.ribbon_hres; ++hi) {
                double h = vis.ribbon_hmax * hi / vis.ribbon_hres;
                bernstein(dh, h, coeff_h);
                Vector p(0.0, 0.0, 0.0);
                for (size_t k = 0; k <= dh; ++k)
                  for (size_t l = 0; l <= ds; ++l)
                    p += ribbon[k][l] * coeff_h[k] * coeff_s[l];
                glVertex3dv(p.data());
              }
              glEnd();
            }
          }
        }
    }
    glLineWidth(1.0);
    glEnable(GL_LIGHTING);
  }

  if (vis.show_misc_lines) {
    glDisable(GL_LIGHTING);
    glLineWidth(line_width);
    glColor3d(0.0, 0.0, 0.0);

    glBegin(GL_LINES);
    for (const auto &[p, q] : misc_lines) {
      glVertex3dv(p.data());
      glVertex3dv(q.data());
    }
    glEnd();

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
  size_t resolution = SchemeWrapper::sexp2uint(SchemeWrapper::getVariable("resolution"));
  mesh.clear();
  domains.clear();
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
    auto surface_type =
      Options::instance()->biharmonic() ? libcdgbs::SurfGBS::BIHARMONIC : libcdgbs::SurfGBS::GBS;
    libcdgbs::SurfGBS::InputParams params(edge_size,
                                          static_cast<bool>(Options::instance()->reparam()),
                                          *Options::instance()->reparam(),
                                          Options::instance()->hsplit(),
                                          Options::instance()->C1(),
                                          Options::instance()->scaling(),
                                          Options::instance()->hWidth(),
                                          true);
    surf.load_ribbons_and_evaluate(ribbons, patch_mesh, params, surface_type);
    domains.push_back(surf.meshDomain);
    mergeMeshes(mesh, patch_mesh, ++id);
  }
  Object::updateBaseMesh(false, false);
  QApplication::restoreOverrideCursor();
}

bool PHGB::reload() {
  auto cmd = std::string("(guard (ex ((eq? ex #t) #t)" // s7 passes also the result as ex
                         "           (else"
                         "             (display \"Error: \") (display ex) (display \" / \")"
                         "             (display (error-object-message ex))"
                         "             (newline)"
                         "             (display \"Irritants: \")"
                         "             (display (error-object-irritants ex))"
                         "             (newline)"
                         "             #f))"
                         "  (load \"molihua.scm\")"
                         "  (load-model \"") + filename + "\")"
                         "  #t)";
  auto load_ok = SchemeWrapper::evaluateString(cmd);

  if (SchemeWrapper::isFalse(load_ok))
    return false;

  size_t n_vertices, n_faces;

  // Extract cage
  {
    cage.clear();
    face_indices.clear();
    auto vertices = SchemeWrapper::getVariable("vertices");
    auto faces = SchemeWrapper::getVariable("faces");
    n_vertices = SchemeWrapper::vectorLength(vertices);
    n_faces = SchemeWrapper::vectorLength(faces);
    std::vector<CageMesh::VertexHandle> handles;
    cage_triangulated.resizePoints(n_vertices);
    for (size_t i = 0; i < n_vertices; ++i) {
      Vector p;
      auto lst = SchemeWrapper::vectorElement(vertices, i);
      for (size_t j = 0; j < 3; ++j)
        p[j] = SchemeWrapper::sexp2double(SchemeWrapper::listElement(lst, j));
      handles.push_back(cage.add_vertex(p));
      cage_triangulated[i] = Geometry::Point3D(p[0], p[1], p[2]);
    }
    for (size_t i = 0; i < n_faces; ++i) {
      auto loops = SchemeWrapper::vectorElement(faces, i);
      size_t m = SchemeWrapper::listLength(loops);
      std::vector<std::vector<size_t>> to_triangulate(m);
      for (size_t l = 0; l < m; ++l) {
        auto lst = SchemeWrapper::listElement(loops, l);
        size_t n = SchemeWrapper::listLength(lst);
        std::vector<CageMesh::VertexHandle> face;
        for (size_t j = 0; j < n; ++j) {
          auto index = SchemeWrapper::sexp2uint(SchemeWrapper::listElement(lst, j));
          face.push_back(handles[index]);
          to_triangulate[l].push_back(index);
        }
        cage.add_face(face);
        face_indices.push_back(i);
      }
      addTriangulatedFace(cage_triangulated, to_triangulate);
    }
  }

  // Extract ribbons
  {
    auto ribbons = SchemeWrapper::getVariable("ribbons");
    patches.resize(n_faces);
    for (size_t i = 0; i < n_faces; ++i) {
      auto loops = SchemeWrapper::vectorElement(ribbons, i);
      size_t m = SchemeWrapper::listLength(loops);
      patches[i].resize(m);
      for (size_t l = 0; l < m; ++l) {
        auto loop = SchemeWrapper::listElement(loops, l);
        size_t n = SchemeWrapper::listLength(loop);
        patches[i][l].resize(n);
        for (size_t j = 0; j < n; ++j) {
          auto ribbon = SchemeWrapper::listElement(loop, j);
          std::array<SchemeWrapper::Sexp, 2> curves = {
            SchemeWrapper::car(ribbon),
            SchemeWrapper::cdr(ribbon)
          };
          for (size_t k = 0; k < 2; ++k) {
            patches[i][l][j][k].clear();
            while (!SchemeWrapper::isNull(curves[k])) {
              auto point = SchemeWrapper::car(curves[k]);
              Vector p;
              for (size_t c = 0; c < 3; ++c)
                p[c] = SchemeWrapper::sexp2double(SchemeWrapper::listElement(point, c));
              patches[i][l][j][k].push_back(p);
              curves[k] = SchemeWrapper::cdr(curves[k]);
            }
          }
        }
      }
    }
  }

  // Extract offsets & chamfers
  {
    auto vertices = SchemeWrapper::getVariable("offset-vertices");
    auto faces = SchemeWrapper::getVariable("offset-faces");
    size_t n_off_vertices = SchemeWrapper::vectorLength(vertices);
    offset_vertices.resize(n_off_vertices);
    offset_faces.resize(n_faces);
    chamfers.resize(n_vertices);
    for (size_t i = 0; i < n_off_vertices; ++i) {
      auto &p = offset_vertices[i];
      auto lst = SchemeWrapper::vectorElement(vertices, i);
      for (size_t j = 0; j < 3; ++j)
        p[j] = SchemeWrapper::sexp2double(SchemeWrapper::listElement(lst, j));
    }
    for (size_t i = 0; i < n_faces; ++i) {
      auto loops = SchemeWrapper::vectorElement(faces, i);
      size_t m = SchemeWrapper::listLength(loops);
      auto &face = offset_faces[i];
      face.resize(m);
      for (size_t l = 0; l < m; ++l) {
        auto lst = SchemeWrapper::listElement(loops, l);
        size_t n = SchemeWrapper::listLength(lst);
        face[l].clear();
        for (size_t j = 0; j < n; ++j) {
          auto index = SchemeWrapper::listElement(lst, j);
          if (SchemeWrapper::isPair(index)) {
            face[l].push_back(SchemeWrapper::sexp2uint(SchemeWrapper::car(index)));
            face[l].push_back(SchemeWrapper::sexp2uint(SchemeWrapper::cdr(index)));
          } else
            face[l].push_back(SchemeWrapper::sexp2uint(index));
        }
      }
    }
    for (size_t i = 0; i < n_vertices; ++i) {
      auto chamfer = SchemeWrapper::evaluateString("(chamfer " + std::to_string(i) + ")");
      chamfers[i].clear();
      while (!SchemeWrapper::isNull(chamfer)) {
        auto index = SchemeWrapper::car(chamfer);
        if (SchemeWrapper::isPair(index)) {
          chamfers[i].push_back(SchemeWrapper::sexp2uint(SchemeWrapper::cdr(index)));
          chamfers[i].push_back(SchemeWrapper::sexp2uint(SchemeWrapper::car(index)));
        } else
          chamfers[i].push_back(SchemeWrapper::sexp2uint(index));
        chamfer = SchemeWrapper::cdr(chamfer);
      }
    }
  }

  // Extract misc lines
  {
    misc_lines.clear();
    auto misc = SchemeWrapper::getVariable("misc-lines");
    while (!SchemeWrapper::isNull(misc)) {
      auto segment = SchemeWrapper::car(misc);
      auto sp = SchemeWrapper::car(segment), sq = SchemeWrapper::cdr(segment);
      Vector p, q;
      for (size_t c = 0; c < 3; ++c) {
        p[c] = SchemeWrapper::sexp2double(SchemeWrapper::listElement(sp, c));
        q[c] = SchemeWrapper::sexp2double(SchemeWrapper::listElement(sq, c));
      }
      misc_lines.push_back({p, q});
      misc = SchemeWrapper::cdr(misc);
    }
  }

  updateBaseMesh();
  return true;
}

std::optional<libcdgbs::Mesh> PHGB::getDomain(int patch) const {
  if ((size_t)patch > domains.size())
    return {};
  return { domains[patch-1] };
}
