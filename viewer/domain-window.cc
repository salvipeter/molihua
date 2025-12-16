#include "domain-window.hh"

#include <QPainter>

DomainWindow::DomainWindow(QWidget *parent) : QDialog(parent) {
  setMinimumSize(300, 300);
  setSizeGripEnabled(true);
  hide();
}

void DomainWindow::setDomain(const std::optional<libcdgbs::Mesh> &mesh) {
  if (!mesh)
    domain.clear();
  else
    domain = *mesh;
  update();
}

void DomainWindow::closeEvent(QCloseEvent *) {
  window_geom = frameGeometry();
  hide();
}

void DomainWindow::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.fillRect(rect(), Qt::white);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(Qt::black, 2));

  int w = width(), h = height();
  double minx = std::numeric_limits<double>::max(), miny = minx;
  double maxx = std::numeric_limits<double>::lowest(), maxy = maxx;
  for (auto v : domain.vertices()) {
    auto p = domain.point(v);
    if (p[0] < minx) minx = p[0];
    if (p[0] > maxx) maxx = p[0];
    if (p[1] < miny) miny = p[1];
    if (p[1] > maxy) maxy = p[1];
  }
  double scale = std::min(w / (maxx - minx), h / (maxy - miny));
  auto tf = [=](const libcdgbs::Mesh::Point &p) -> std::pair<int, int> {
    return { std::round((p[0] - minx) * scale),
             h - std::round((p[1] - miny) * scale) };
  };
  for (auto e : domain.edges()) {
    auto p1 = domain.point(e.vertex(0));
    auto p2 = domain.point(e.vertex(1));
    auto [x1,y1] = tf(p1);
    auto [x2,y2] = tf(p2);
    painter.drawLine(x1, y1, x2, y2);
  }
}

void DomainWindow::showEvent(QShowEvent *event) {
  QDialog::showEvent(event);
  setSizeGripEnabled(true);
  if (!window_geom.isEmpty())
    setGeometry(window_geom);
}
