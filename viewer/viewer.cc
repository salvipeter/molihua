#include <QMessageBox>
#include <QtGui/QKeyEvent>

#include "ph-gb.hh"
#include "viewer.hh"

Viewer::Viewer(QWidget *parent) : QGLViewer(parent), white_back(false) {
  setSelectRegionWidth(10);
  setSelectRegionHeight(10);
  axes.shown = false;
}

Viewer::~Viewer() {
  glDeleteTextures(1, &vis.isophote_texture);
  glDeleteTextures(1, &vis.environment_texture);
  glDeleteTextures(1, &vis.slicing_texture);
}

double Viewer::getMeanCutoffRatio() const {
  return vis.mean_cutoff_ratio;
}

void Viewer::setMeanCutoffRatio(double ratio) {
  vis.mean_cutoff_ratio = ratio;
  updateCurvatureMinMax();
}

double Viewer::getMeanMin() const {
  return vis.mean_min;
}

void Viewer::setMeanMin(double min) {
  vis.mean_min = min;
}

double Viewer::getMeanMax() const {
  return vis.mean_max;
}

void Viewer::setMeanMax(double max) {
  vis.mean_max = max;
}

double Viewer::getGaussCutoffRatio() const {
  return vis.gauss_cutoff_ratio;
}

void Viewer::setGaussCutoffRatio(double ratio) {
  vis.gauss_cutoff_ratio = ratio;
  updateCurvatureMinMax();
}

double Viewer::getGaussMin() const {
  return vis.gauss_min;
}

void Viewer::setGaussMin(double min) {
  vis.gauss_min = min;
}

double Viewer::getGaussMax() const {
  return vis.gauss_max;
}

void Viewer::setGaussMax(double max) {
  vis.gauss_max = max;
}

const double *Viewer::getSlicingDir() const {
  return vis.slicing_dir.data();
}

void Viewer::setSlicingDir(double x, double y, double z) {
  vis.slicing_dir = Vector(x, y, z).normalized();
}

double Viewer::getSlicingScaling() const {
  return vis.slicing_scaling;
}

void Viewer::setSlicingScaling(double scaling) {
  vis.slicing_scaling = scaling;
}

double Viewer::getRibbonHMax() const {
  return vis.ribbon_hmax;
}

void Viewer::setRibbonHMax(double hmax) {
  vis.ribbon_hmax = hmax;
}

size_t Viewer::getRibbonHRes() const {
  return vis.ribbon_hres;
}

void Viewer::setRibbonHRes(size_t hres) {
  vis.ribbon_hres = hres;
}

size_t Viewer::getRibbonSRes() const {
  return vis.ribbon_sres;
}

void Viewer::setRibbonSRes(size_t sres) {
  vis.ribbon_sres = sres;
}

void Viewer::deleteObjects() {
  objects.clear();
}

bool Viewer::open(std::string filename) {
  std::shared_ptr<Object> surface = std::make_shared<PHGB>(filename);
  if (!surface->valid())
    return false;
  objects.push_back(surface);
  updateCurvatureMinMax();
  setupCamera();
  return true;
}

bool Viewer::reload() {
  for (auto o : objects)
    if (!o->reload())
      return false;
  update();
  return true;
}

void Viewer::init() {
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
  glEnable(GL_LIGHT0);

  // Light parameters (directional white-ish, slightly cool)
  GLfloat light_pos[]     = { 1.0f, 1.0f, 2.0f, 0.0f }; // w=0 => directional
  GLfloat light_diffuse[] = { 0.95f, 0.98f, 1.00f, 1.0f };
  GLfloat light_specular[]= { 1.0f,  1.0f,  1.0f,  1.0f };
  GLfloat light_ambient[] = { 0.12f, 0.12f, 0.12f, 1.0f };

  glLightfv(GL_LIGHT0, GL_POSITION,  light_pos);
  glLightfv(GL_LIGHT0, GL_DIFFUSE,   light_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR,  light_specular);
  glLightfv(GL_LIGHT0, GL_AMBIENT,   light_ambient);

  // Material (muted blue-gray diffuse + modest specular)
  GLfloat mat_diffuse[]  = { 0.62f, 0.68f, 0.74f, 1.0f }; // your surface color
  GLfloat mat_specular[] = { 0.25f, 0.25f, 0.25f, 1.0f }; // specular strength
  GLfloat mat_shininess  = 48.0f;                         // tightness of highlight

  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   mat_diffuse);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  mat_specular);
  glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);

  // Optional: mild scene ambient (keeps silhouettes visible w/o flattening)
  GLfloat globalAmbient[] = { 0.08f, 0.08f, 0.08f, 1.0f };
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

  QImage img(":/isophotes.png");
  glGenTextures(1, &vis.isophote_texture);
  glBindTexture(GL_TEXTURE_2D, vis.isophote_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.width(), img.height(), 0, GL_BGRA,
               GL_UNSIGNED_BYTE, img.convertToFormat(QImage::Format_ARGB32).bits());

  QImage img2(":/environment.png");
  glGenTextures(1, &vis.environment_texture);
  glBindTexture(GL_TEXTURE_2D, vis.environment_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img2.width(), img2.height(), 0, GL_BGRA,
               GL_UNSIGNED_BYTE, img2.convertToFormat(QImage::Format_ARGB32).bits());

  glGenTextures(1, &vis.slicing_texture);
  glBindTexture(GL_TEXTURE_1D, vis.slicing_texture);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  static const unsigned char slicing_img[] = { 0b11111111, 0b00011100 };
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 2, 0, GL_RGB, GL_UNSIGNED_BYTE_3_3_2, &slicing_img);
}

void Viewer::draw() {
  if (white_back)
    glClearColor(1.0, 1.0, 1.0, 1.0);
  else
    glClearColor(0.33, 0.33, 0.43, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  for (auto o : objects)
    o->draw(vis);

  if (axes.shown) {
    using qglviewer::Vec;
    const auto &p = Vec(axes.position);
    glColor3d(1.0, 0.0, 0.0);
    drawArrow(p, p + Vec(axes.size, 0.0, 0.0), axes.size / 50.0);
    glColor3d(0.0, 1.0, 0.0);
    drawArrow(p, p + Vec(0.0, axes.size, 0.0), axes.size / 50.0);
    glColor3d(0.0, 0.0, 1.0);
    drawArrow(p, p + Vec(0.0, 0.0, axes.size), axes.size / 50.0);
  }
}

void Viewer::drawWithNames() {
  if (axes.shown) {
    using qglviewer::Vec;
    const auto &p = Vec(axes.position);
    glPushName(0);
    drawArrow(p, p + Vec(axes.size, 0.0, 0.0), axes.size / 50.0);
    glPopName();
    glPushName(1);
    drawArrow(p, p + Vec(0.0, axes.size, 0.0), axes.size / 50.0);
    glPopName();
    glPushName(2);
    drawArrow(p, p + Vec(0.0, 0.0, axes.size), axes.size / 50.0);
    glPopName();
  } else {
    for (size_t i = 0; i < objects.size(); ++i) {
      glPushName(i);
      objects[i]->drawWithNames(vis);
      glPopName();
    }
  }
}

// Same as the default implementation,
// but when there are 2 names for a hit,
// the first is treated as the selected object id.
void Viewer::endSelection(const QPoint &) {
  glFlush();
  GLint nbHits = glRenderMode(GL_RENDER);
  if (nbHits <= 0)
    setSelectedName(-1);
  else {
    const GLuint *ptr = selectBuffer();
    GLuint zMin = std::numeric_limits<GLuint>::max();
    for (int i = 0; i < nbHits; ++i, ptr += 4) {
      GLuint names = ptr[0];
      if (ptr[1] < zMin) {
        zMin = ptr[1];
        if (names == 2) {
          selected_object = ptr[3];
          ptr++;
        }
        setSelectedName(ptr[3]);
      } else if (names == 2)
        ptr++;
    }
  }
}

static inline Vector toVector(const qglviewer::Vec &v) {
  return Vector(static_cast<const qreal *>(v));
}

void Viewer::postSelection(const QPoint &p) {
  int sel = selectedName();
  if (sel == -1) {
    axes.shown = false;
    return;
  }

  if (axes.shown) {
    axes.selected_axis = sel;
    bool found;
    axes.grabbed_pos = toVector(camera()->pointUnderPixel(p, found));
    axes.original_pos = axes.position;
    if (!found)
      axes.shown = false;
    return;
  }

  using qglviewer::Vec;
  selected_vertex = sel;
  axes.position = objects[selected_object]->postSelection(sel);
  double depth = camera()->projectedCoordinatesOf(Vec(axes.position))[2];
  Vec q1 = camera()->unprojectedCoordinatesOf(Vec(0.0, 0.0, depth));
  Vec q2 = camera()->unprojectedCoordinatesOf(Vec(width(), height(), depth));
  axes.size = (q1 - q2).norm() / 10.0;
  axes.shown = true;
  axes.selected_axis = -1;
}

void Viewer::keyPressEvent(QKeyEvent *e) {
  if (e->key() == Qt::Key_Question) {
    help();
    return;
  }
  if (e->modifiers() == Qt::NoModifier)
    switch (e->key()) {
    case Qt::Key_R:
      if (!reload())
        QMessageBox::critical(this, "Reload failed", "Error reloading file");
      break;
    case Qt::Key_O:
      if (camera()->type() == qglviewer::Camera::PERSPECTIVE)
        camera()->setType(qglviewer::Camera::ORTHOGRAPHIC);
      else
        camera()->setType(qglviewer::Camera::PERSPECTIVE);
      update();
      break;
    case Qt::Key_H:
      white_back = !white_back;
      update();
      break;
    case Qt::Key_P:
      vis.type = VisType::PLAIN;
      update();
      break;
    case Qt::Key_M:
      vis.type = VisType::MEAN;
      update();
      break;
    case Qt::Key_G:
      vis.type = VisType::GAUSS;
      update();
      break;
    case Qt::Key_L:
      vis.type = VisType::SLICING;
      update();
      break;
    case Qt::Key_I:
      vis.type = VisType::ISOPHOTES;
      vis.current_isophote_texture = vis.isophote_texture;
      update();
      break;
    case Qt::Key_E:
      vis.type = VisType::ISOPHOTES;
      vis.current_isophote_texture = vis.environment_texture;
      update();
      break;
    case Qt::Key_C:
      switch (vis.cage) {
      case Visualization::CageType::NONE:
        vis.cage = Visualization::CageType::NET; break;
      case Visualization::CageType::NET:
        vis.cage = Visualization::CageType::SURFACE; break;
      case Visualization::CageType::SURFACE:
        vis.cage = Visualization::CageType::NONE; break;
      }
      update();
      break;
    case Qt::Key_V:
      vis.show_offsets = !vis.show_offsets;
      update();
      break;
    case Qt::Key_A:
      vis.show_auxiliary = !vis.show_auxiliary;
      update();
      break;
    case Qt::Key_F:
      switch (vis.chamfers) {
      case Visualization::ChamferType::NONE:
        vis.chamfers = Visualization::ChamferType::NET; break;
      case Visualization::ChamferType::NET:
        vis.chamfers = Visualization::ChamferType::SURFACE; break;
      case Visualization::ChamferType::SURFACE:
        vis.chamfers = Visualization::ChamferType::NONE; break;
      }
      update();
      break;
    case Qt::Key_D:
      switch (vis.boundaries) {
      case Visualization::BoundaryType::NONE:
        vis.boundaries = Visualization::BoundaryType::CP; break;
      case Visualization::BoundaryType::CP:
        vis.boundaries = Visualization::BoundaryType::CURVE; break;
      case Visualization::BoundaryType::CURVE:
        vis.boundaries = Visualization::BoundaryType::NONE; break;
      }
      update();
      break;
    case Qt::Key_B:
      switch (vis.ribbons) {
      case Visualization::RibbonType::NONE:
        vis.ribbons = Visualization::RibbonType::NET; break;
      case Visualization::RibbonType::NET:
        vis.ribbons = Visualization::RibbonType::SURFACE; break;
      case Visualization::RibbonType::SURFACE:
        vis.ribbons = Visualization::RibbonType::NONE; break;
      }
      update();
      break;
    case Qt::Key_S:
      vis.show_solid = !vis.show_solid;
      update();
      break;
    case Qt::Key_K:
      vis.show_misc_lines = !vis.show_misc_lines;
      update();
      break;
    case Qt::Key_W:
      vis.show_wireframe = !vis.show_wireframe;
      update();
      break;
    case Qt::Key_T:
      vis.transparent = !vis.transparent;
      update();
      break;
    case Qt::Key_X:
      camera()->setViewDirection(qglviewer::Vec(1.0, 0.0, 0.0));
      camera()->setUpVector(qglviewer::Vec(0.0, 1.0, 0.0));
      camera()->showEntireScene();
      update();
      break;
    case Qt::Key_Y:
      camera()->setViewDirection(qglviewer::Vec(0.0, 1.0, 0.0));
      camera()->setUpVector(qglviewer::Vec(0.0, 0.0, 1.0));
      camera()->showEntireScene();
      update();
      break;
    case Qt::Key_Z:
      camera()->setViewDirection(qglviewer::Vec(0.0, 0.0, 1.0));
      camera()->setUpVector(qglviewer::Vec(1.0, 0.0, 0.0));
      camera()->showEntireScene();
      update();
      break;
    default:
      QGLViewer::keyPressEvent(e);
    }
  else if (e->modifiers() == Qt::KeypadModifier)
    switch (e->key()) {
    case Qt::Key_Plus:
      vis.slicing_scaling *= 2;
      update();
      break;
    case Qt::Key_Minus:
      vis.slicing_scaling /= 2;
      update();
      break;
    case Qt::Key_Asterisk:
      vis.slicing_dir = Vector(static_cast<double *>(camera()->viewDirection()));
      update();
      break;
    } else
    QGLViewer::keyPressEvent(e);
}

static Vector intersectLines(const Vector &ap, const Vector &ad,
                             const Vector &bp, const Vector &bd) {
  // always returns a point on the (ap, ad) line
  double a = ad.sqrnorm(), b = ad | bd, c = bd.sqrnorm();
  double d = ad | (ap - bp), e = bd | (ap - bp);
  if (a * c - b * b < 1.0e-7)
    return ap;
  double s = (b * e - c * d) / (a * c - b * b);
  return ap + s * ad;
}

void Viewer::mouseMoveEvent(QMouseEvent *e) {
  if (!axes.shown ||
      (axes.selected_axis < 0 && !(e->modifiers() & Qt::ControlModifier)) ||
      !(e->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier)) ||
      !(e->buttons() & Qt::LeftButton))
    return QGLViewer::mouseMoveEvent(e);

  using qglviewer::Vec;
  if (e->modifiers() & Qt::ControlModifier) {
    // move in screen plane
    double depth = camera()->projectedCoordinatesOf(Vec(axes.position))[2];
    auto pos = camera()->unprojectedCoordinatesOf(Vec(e->pos().x(), e->pos().y(), depth));
    axes.position = toVector(pos);
  } else {
    Vec from, dir, axis(axes.selected_axis == 0, axes.selected_axis == 1, axes.selected_axis == 2);
    camera()->convertClickToLine(e->pos(), from, dir);
    auto p = intersectLines(axes.grabbed_pos, toVector(axis), toVector(from), toVector(dir));
    float d = (p - axes.grabbed_pos) | toVector(axis);
    axes.position[axes.selected_axis] = axes.original_pos[axes.selected_axis] + d;
  }

  objects[selected_object]->movement(selected_vertex, axes.position);
  objects[selected_object]->updateBaseMesh();
  updateCurvatureMinMax();
  update();
}

QString Viewer::helpString() const {
  QString text("<h2>Polyhedron-based Model Visualization</h2>"
               "<p>The following hotkeys are available:</p>"
               "<ul>"
               "<li>&nbsp;R: Reload model</li>"
               "<li>&nbsp;O: Toggle orthographic projection</li>"
               "<li>&nbsp;H: Toggle white background</li>"
               "<li>&nbsp;T: Toggle surface transparency</li>"
               "<li>&nbsp;P: Set plain map (no coloring)</li>"
               "<li>&nbsp;M: Set mean curvature map</li>"
               "<li>&nbsp;G: Set Gaussian curvature map</li>"
               "<li>&nbsp;L: Set slicing map<ul>"
               "<li>&nbsp;+: Increase slicing density</li>"
               "<li>&nbsp;-: Decrease slicing density</li>"
               "<li>&nbsp;*: Set slicing direction to view</li></ul></li>"
               "<li>&nbsp;I: Set isophote line map</li>"
               "<li>&nbsp;E: Set environment texture</li>"
               "<li>&nbsp;C: Toggle cage visualization (none / net / polyhedron)</li>"
               "<li>&nbsp;A: Toggle auxiliary polyhedron visualization</li>"
               "<li>&nbsp;V: Toggle offset visualization</li>"
               "<li>&nbsp;F: Toggle chamfer visualization (none / net / surface)</li>"
               "<li>&nbsp;D: Toggle boundary visualization (none / CP / curve)</li>"
               "<li>&nbsp;B: Toggle ribbon visualization (none / net / surface)</li>"
               "<li>&nbsp;S: Toggle solid (filled polygon) visualization</li>"
               "<li>&nbsp;W: Toggle wireframe visualization</li>"
               "<li>&nbsp;K: Toggle miscellaneous lines</li>"
               "<li>&nbsp;X/Y/Z: Set standard view direction</li>"
               "<li>&nbsp;?: This help</li>"
               "</ul>"
               "<p align=\"right\">Peter Salvi</p>");
  return text;
}

void Viewer::updateCurvatureMinMax() {
  std::vector<double> mean, gauss;
  for (auto o : objects) {
    const auto &mesh = o->baseMesh();
    for (auto v : mesh.vertices()) {
      mean.push_back(mesh.data(v).mean);
      gauss.push_back(mesh.data(v).gauss);
    }
  }

  size_t n = mean.size();
  if (n < 3)
    return;

  std::sort(mean.begin(), mean.end());
  std::sort(gauss.begin(), gauss.end());
  size_t k = (double)n * vis.mean_cutoff_ratio;
  vis.mean_min = std::min(mean[k ? k-1 : 0], 0.0);
  vis.mean_max = std::max(mean[k ? n-k : n-1], 0.0);

  // Use symmetric min/max
  auto max = std::max(-vis.mean_min, vis.mean_max);
  vis.mean_min = -max;
  vis.mean_max = max;

  k = (double)n * vis.gauss_cutoff_ratio;
  vis.gauss_min = std::min(gauss[k ? k-1 : 0], 0.0);
  vis.gauss_max = std::max(gauss[k ? n-k : n-1], 0.0);

  // Use symmetric min/max
  max = std::max(-vis.gauss_min, vis.gauss_max);
  vis.gauss_min = -max;
  vis.gauss_max = max;
}

void Viewer::setupCamera() {
  double large = std::numeric_limits<double>::max();
  Vector box_min(large, large, large), box_max(-large, -large, -large);
  for (auto o : objects) {
    const auto &mesh = o->baseMesh();
    for (auto v : mesh.vertices()) {
      box_min.minimize(mesh.point(v));
      box_max.maximize(mesh.point(v));
    }
  }
  using qglviewer::Vec;
  camera()->setSceneBoundingBox(Vec(box_min.data()), Vec(box_max.data()));
  camera()->showEntireScene();

  vis.slicing_scaling = 20 / (box_max - box_min).max();

  setSelectedName(-1);
  axes.shown = false;

  update();
}

std::vector<std::shared_ptr<Object>> Viewer::getObjects() const {
  return objects;
}
