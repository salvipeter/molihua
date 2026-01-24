#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "domain-window.hh"
#include "options.hh"
#include "ph-gb.hh"
#include "scheme-wrapper.hh"
#include "viewer.hh"

bool Options::hsplit() const {
  return hsplitCheck->checkState() == Qt::Checked;
}

bool Options::C1() const {
  return C1Check->checkState() == Qt::Checked;
}

bool Options::mergeC1() const {
  return mergeC1Check->checkState() == Qt::Checked;
}

bool Options::biharmonic() const {
  return biharmonicCheck->checkState() == Qt::Checked;
}

bool Options::hWidth() const {
  return hwidthCheck->checkState() == Qt::Checked;
}

Options *Options::instance(Viewer *viewer, DomainWindow *domain_window) {
  static Options *instance = new Options(viewer, domain_window);
  return instance;
}

Options::Options(Viewer *viewer, DomainWindow *domain_window) :
  viewer(viewer), domain_window(domain_window) {
  auto master = new QVBoxLayout();

  auto visual = new QGroupBox("Visualization");
  master->addWidget(visual);
  auto visualLayout = new QVBoxLayout();
  visual->setLayout(visualLayout);

  visualLayout->addWidget(new QLabel("Resolution:"));
  auto resBox = new QSpinBox();
  resBox->setRange(10, 1000);
  resBox->setValue(100);
  resBox->setSingleStep(10);
  resolutionChanged(resBox->value());
  visualLayout->addWidget(resBox);
  connect(resBox, &QSpinBox::valueChanged, this, &Options::resolutionChanged);

  auto oneCheck = new QCheckBox("One patch mode");
  visualLayout->addWidget(oneCheck);
  connect(oneCheck, &QCheckBox::checkStateChanged, this, &Options::onePatchChanged);
  visualLayout->addWidget(new QLabel("Selected patch:"));
  selectedBox = new QSpinBox();
  selectedBox->setRange(1, 1000);
  selectedBox->setValue(1);
  selectedBox->setEnabled(false);
  visualLayout->addWidget(selectedBox);
  onePatchChanged(oneCheck->checkState());
  connect(selectedBox, &QSpinBox::valueChanged, this, &Options::selectedChanged);

  auto geometry = new QGroupBox("Geometry");
  master->addWidget(geometry);
  auto geometryLayout = new QVBoxLayout();
  geometry->setLayout(geometryLayout);

  auto edgeCheck = new QCheckBox("Edge-based offsets");
  geometryLayout->addWidget(edgeCheck);
  connect(edgeCheck, &QCheckBox::checkStateChanged, this, &Options::edgeChanged);
  edgeChanged(edgeCheck->checkState());

  geometryLayout->addWidget(new QLabel("Fullness:"));
  auto fullBox = new QDoubleSpinBox();
  fullBox->setRange(0.01, 0.99);
  fullBox->setValue(0.7);
  fullBox->setSingleStep(0.1);
  fullnessChanged(fullBox->value());
  geometryLayout->addWidget(fullBox);
  connect(fullBox, &QDoubleSpinBox::valueChanged, this, &Options::fullnessChanged);

  auto shrinkCheck = new QCheckBox("Shrink inwards");
  shrinkCheck->setChecked(true);
  geometryLayout->addWidget(shrinkCheck);
  connect(shrinkCheck, &QCheckBox::checkStateChanged,
          [this](Qt::CheckState state) { shrinkChanged(state, true); });
  shrinkChanged(shrinkCheck->checkState(), true);

  auto shrinkCheck2 = new QCheckBox("Shrink outwards");
  shrinkCheck2->setChecked(true);
  geometryLayout->addWidget(shrinkCheck2);
  connect(shrinkCheck2, &QCheckBox::checkStateChanged,
          [this](Qt::CheckState state) { shrinkChanged(state, false); });
  shrinkChanged(shrinkCheck2->checkState(), false);

  geometryLayout->addWidget(new QLabel("Tangent scaling:"));
  auto tanscaleBox = new QDoubleSpinBox();
  tanscaleBox->setRange(0.01, 10.0);
  tanscaleBox->setValue(4.0/3.0);
  tanscaleBox->setSingleStep(0.1);
  tangentScaleChanged(tanscaleBox->value());
  geometryLayout->addWidget(tanscaleBox);
  connect(tanscaleBox, &QDoubleSpinBox::valueChanged, this, &Options::tangentScaleChanged);

  geometryLayout->addWidget(new QLabel("Midvector scaling:"));
  auto mvecscaleBox = new QDoubleSpinBox();
  mvecscaleBox->setRange(0.01, 10.0);
  mvecscaleBox->setValue(1);
  mvecscaleBox->setSingleStep(0.1);
  mvecScaleChanged(mvecscaleBox->value());
  geometryLayout->addWidget(mvecscaleBox);
  connect(mvecscaleBox, &QDoubleSpinBox::valueChanged, this, &Options::mvecScaleChanged);

  geometryLayout->addWidget(new QLabel("Direction blend type:"));
  auto dblendCombo = new QComboBox();
  dblendCombo->addItem("None");
  dblendCombo->addItem("Cubic");
  dblendCombo->addItem("Cubic - simple");
  dblendCombo->addItem("Cubic - no alpha");
  dblendCombo->addItem("CubicTomi - simple");
  dblendCombo->addItem("CubicPeti - simple");
  dblendCombo->addItem("Magic");
  dblendCombo->addItem("Cubic - linear");
  dblendCombo->addItem("Cubic - linear C0");
  dblendCombo->addItem("Quartic - simple");
  dblendCombo->addItem("Quartic - no alpha");
  dblendCombo->addItem("QuarticTomi - simple");
  dblendCombo->addItem("QuarticTomi - no alpha");
  dblendCombo->addItem("Quintic");
  dblendCombo->addItem("QuinticTomi");
  dblendCombo->setCurrentIndex(6);
  geometryLayout->addWidget(dblendCombo);
  connect(dblendCombo, &QComboBox::activated, this, &Options::dblendChanged);
  dblendChanged(dblendCombo->currentIndex());

  auto quinticCubicCheck = new QCheckBox("Cubic cross-degree");
  quinticCubicCheck->setChecked(true);
  geometryLayout->addWidget(quinticCubicCheck);
  connect(quinticCubicCheck, &QCheckBox::checkStateChanged, this, &Options::quinticCubicChanged);
  quinticCubicChanged(quinticCubicCheck->checkState());

  bsplineConcaveCheck = new QCheckBox("B-Spline concave edges");
  bsplineConcaveCheck->setChecked(true);
  geometryLayout->addWidget(bsplineConcaveCheck);
  geometryLayout->addWidget(new QLabel("B-spline reparameterization:"));
  reparamBox = new QDoubleSpinBox();
  reparamBox->setRange(-1.0, 1.0);
  reparamBox->setValue(0.5);
  reparamBox->setSingleStep(0.1);
  geometryLayout->addWidget(reparamBox);
  connect(bsplineConcaveCheck, &QCheckBox::checkStateChanged, 
          this, &Options::bsplineConcaveChanged);
  bsplineConcaveChanged(bsplineConcaveCheck->checkState());
  connect(reparamBox, &QDoubleSpinBox::valueChanged, viewer, &Viewer::reload);

  hsplitCheck = new QCheckBox("Restricted parameterization");
  hsplitCheck->setChecked(true);
  geometryLayout->addWidget(hsplitCheck);
  connect(hsplitCheck, &QCheckBox::checkStateChanged, viewer, &Viewer::reload);

  C1Check = new QCheckBox("C1 ribbon merge");
  C1Check->setChecked(true);
  geometryLayout->addWidget(C1Check);
  connect(C1Check, &QCheckBox::checkStateChanged, viewer, &Viewer::reload);

  mergeC1Check = new QCheckBox("C1 ribbon inner merge");
  mergeC1Check->setChecked(false);
  geometryLayout->addWidget(mergeC1Check);
  connect(mergeC1Check, &QCheckBox::checkStateChanged, viewer, &Viewer::reload);

  hwidthCheck = new QCheckBox("Cross-reparameterization");
  hwidthCheck->setChecked(false);
  geometryLayout->addWidget(hwidthCheck);
  connect(hwidthCheck, &QCheckBox::checkStateChanged, viewer, &Viewer::reload);

  geometryLayout->addWidget(new QLabel("Loop scaling factor:"));
  scalingCheck = new QCheckBox("Automatic");
  scalingCheck->setChecked(true);
  geometryLayout->addWidget(scalingCheck);
  scalingBox = new QDoubleSpinBox();
  scalingBox->setRange(0.1, 1.0);
  scalingBox->setValue(0.7);
  scalingBox->setSingleStep(0.1);
  geometryLayout->addWidget(scalingBox);
  connect(scalingCheck, &QCheckBox::checkStateChanged, this, &Options::scalingChanged);
  connect(scalingBox, &QDoubleSpinBox::valueChanged, viewer, &Viewer::reload);
  connect(scalingBox, &QDoubleSpinBox::valueChanged, this, &Options::updateDomain);
  scalingChanged(scalingCheck->checkState());
 
  biharmonicCheck = new QCheckBox("Biharmonic surfaces");
  biharmonicCheck->setChecked(false);
  geometryLayout->addWidget(biharmonicCheck);
  connect(biharmonicCheck, &QCheckBox::checkStateChanged, viewer, &Viewer::reload);

  auto misc = new QGroupBox("Miscellaneous");
  master->addWidget(misc);
  auto miscLayout = new QVBoxLayout();
  misc->setLayout(miscLayout);

  auto exportButton = new QPushButton("Export selected patch");
  connect(exportButton, &QPushButton::clicked, this, &Options::exportClicked);
  miscLayout->addWidget(exportButton);

  auto exportModelButton = new QPushButton("Export model");
  connect(exportModelButton, &QPushButton::clicked, this, &Options::exportModelClicked);
  miscLayout->addWidget(exportModelButton);

  setLayout(master);
}

void Options::fullnessChanged(double full) {
  SchemeWrapper::setVariable("fullness", SchemeWrapper::double2sexp(full));
  viewer->reload();
}

void Options::mvecScaleChanged(double scale) {
  SchemeWrapper::setVariable("midvector-scale", SchemeWrapper::double2sexp(scale));
  viewer->reload();
}

void Options::onePatchChanged(Qt::CheckState state) {
  if (state == Qt::Checked) {
    // selectedBox->setValue(1);
    selectedBox->setEnabled(true);
    selectedChanged(selectedBox->value());
  } else {
    selectedBox->setEnabled(false);
    SchemeWrapper::setVariable("only-one-patch", SchemeWrapper::bool2sexp(false));
    viewer->update();
    domain_window->setDomain(std::optional<libcdgbs::Mesh>());
  }
}

void Options::resolutionChanged(int res) {
  SchemeWrapper::setVariable("resolution", SchemeWrapper::uint2sexp(res));
  viewer->reload();
}

void Options::selectedChanged(int selected) {
  SchemeWrapper::setVariable("only-one-patch", SchemeWrapper::uint2sexp(selected));
  viewer->update();
  updateDomain();
}

void Options::edgeChanged(Qt::CheckState state) {
  SchemeWrapper::setVariable("edge-based-offsets?", SchemeWrapper::bool2sexp(state == Qt::Checked));
  viewer->reload();
}

void Options::shrinkChanged(Qt::CheckState state, bool inwards) {
  if (inwards)
    SchemeWrapper::setVariable("shrink-inwards?", SchemeWrapper::bool2sexp(state == Qt::Checked));
  else
    SchemeWrapper::setVariable("shrink-outwards?", SchemeWrapper::bool2sexp(state == Qt::Checked));
  viewer->reload();
}

void Options::tangentScaleChanged(double scale) {
  SchemeWrapper::setVariable("tangent-scale", SchemeWrapper::double2sexp(scale));
  viewer->reload();
}

void Options::dblendChanged(int index) {
  std::array<std::string, 15> types = {
    "none", "cubic", "cubic-simple", "cubic-no-alpha", "cubic-tomi-simple", "cubic-peti-simple",
    "magic",
    "cubic-linear", "cubic-linear-c0", "quartic-simple", "quartic-no-alpha", "quartic-tomi-simple",
    "quartic-tomi-no-alpha", "quintic", "quintic-tomi"
  };
  SchemeWrapper::setVariable("direction-blend-type", SchemeWrapper::string2symbol(types[index]));
  viewer->reload();
}

void Options::quinticCubicChanged(Qt::CheckState state) {
  SchemeWrapper::setVariable("cubic-cross-degree?", SchemeWrapper::bool2sexp(state == Qt::Checked));
  viewer->reload();
}

void Options::bsplineConcaveChanged(Qt::CheckState state) {
  if (state == Qt::Checked)
    reparamBox->setEnabled(true);
  else
    reparamBox->setEnabled(false);
  viewer->reload();
}

std::optional<double> Options::reparam() const {
  if (bsplineConcaveCheck->checkState() == Qt::Checked)
    return { reparamBox->value() };
  return {};
}

void Options::scalingChanged(Qt::CheckState state) {
  if (state == Qt::Checked)
    scalingBox->setEnabled(false);
  else
    scalingBox->setEnabled(true);
  viewer->reload();
}

double Options::scaling() const {
  if (scalingCheck->checkState() == Qt::Checked)
    return -1.0;
  return scalingBox->value();
}

void Options::exportClicked() {
  // Check if selection is valid
  size_t selected = selectedBox->value() - 1;
  auto ribbons = SchemeWrapper::getVariable("ribbons");
  size_t n_ribbons = SchemeWrapper::vectorLength(ribbons);
  if (selected >= n_ribbons)
    return;

  // Get a file name
  auto name = QFileDialog::getSaveFileName(this, "Export Patch", ".", "GBS patches (*.mgbs)");
  if (name.isEmpty())
    return;
  if (!name.endsWith(".mgbs"))
    name += ".mgbs";

  // Export
  QString cmd = ("(write-patch-mgbs (vector-ref ribbons %1) \"" + name + "\")").arg(selected);
  SchemeWrapper::evaluateString(cmd.toUtf8().data());
}

void Options::exportModelClicked() {
  if (viewer->getObjects().size() != 1)
    return;

  // Get a file name
  auto name = QFileDialog::getSaveFileName(this, "Export Model", ".", "Meshes (*.obj)");
  if (name.isEmpty())
    return;
  if (!name.endsWith(".obj"))
    name += ".obj";

  // Export
  viewer->getObjects()[0]->saveModel(name.toUtf8().data());
}

void Options::updateDomain() const {
  const auto &objects = viewer->getObjects();
  if (!objects.empty()) {
    const auto &ph = dynamic_cast<PHGB *>(objects.front().get());
    domain_window->setDomain(ph->getDomain(selectedBox->value()));
  }
}
