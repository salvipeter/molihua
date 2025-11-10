#include <libguile.h>

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
#include "viewer.hh"

bool Options::hsplit() const {
  return hsplitCheck->checkState() == Qt::Checked;
}

bool Options::C1() const {
  return C1Check->checkState() == Qt::Checked;
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

  geometryLayout->addWidget(new QLabel("Fullness:"));
  auto fullBox = new QDoubleSpinBox();
  fullBox->setRange(0.01, 0.99);
  fullBox->setValue(0.7);
  fullBox->setSingleStep(0.1);
  fullnessChanged(fullBox->value());
  geometryLayout->addWidget(fullBox);
  connect(fullBox, &QDoubleSpinBox::valueChanged, this, &Options::fullnessChanged);

  auto shrinkCheck = new QCheckBox("Shrink chamfers");
  geometryLayout->addWidget(shrinkCheck);
  connect(shrinkCheck, &QCheckBox::checkStateChanged, this, &Options::shrinkChanged);
  shrinkChanged(shrinkCheck->checkState());

  geometryLayout->addWidget(new QLabel("Tangent scaling:"));
  auto tanscaleBox = new QDoubleSpinBox();
  tanscaleBox->setRange(0.01, 10.0);
  tanscaleBox->setValue(1.33);
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
  dblendCombo->addItem("Cubic - linear");
  dblendCombo->addItem("Quartic - simple");
  dblendCombo->addItem("Quartic - no alpha");
  dblendCombo->addItem("QuarticTomi - simple");
  dblendCombo->addItem("QuarticTomi - no alpha");
  dblendCombo->addItem("Quintic");
  dblendCombo->addItem("QuinticTomi");
  dblendCombo->setCurrentIndex(4);
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
  C1Check->setChecked(false);
  geometryLayout->addWidget(C1Check);
  connect(C1Check, &QCheckBox::checkStateChanged, viewer, &Viewer::reload);

  geometryLayout->addWidget(new QLabel("Loop scaling factor:"));
  scalingBox = new QDoubleSpinBox();
  scalingBox->setRange(0.1, 1.0);
  scalingBox->setValue(0.7);
  scalingBox->setSingleStep(0.1);
  geometryLayout->addWidget(scalingBox);
  connect(scalingBox, &QDoubleSpinBox::valueChanged, viewer, &Viewer::reload);
  connect(scalingBox, &QDoubleSpinBox::valueChanged, this, &Options::updateDomain);

  auto misc = new QGroupBox("Miscellaneous");
  master->addWidget(misc);
  auto miscLayout = new QVBoxLayout();
  misc->setLayout(miscLayout);

  auto exportButton = new QPushButton("Export selected patch");
  connect(exportButton, &QPushButton::clicked, this, &Options::exportClicked);
  miscLayout->addWidget(exportButton);

  setLayout(master);
}

void Options::fullnessChanged(double full) {
  scm_c_define("fullness", scm_from_double(full));
  viewer->reload();
}

void Options::mvecScaleChanged(double scale) {
  scm_c_define("midvector-scale", scm_from_double(scale));
  viewer->reload();
}

void Options::onePatchChanged(Qt::CheckState state) {
  if (state == Qt::Checked) {
    // selectedBox->setValue(1);
    selectedBox->setEnabled(true);
    selectedChanged(selectedBox->value());
  } else {
    selectedBox->setEnabled(false);
    scm_c_define("only-one-patch", SCM_BOOL_F);
    viewer->update();
    domain_window->setDomain(std::optional<libcdgbs::Mesh>());
  }
}

void Options::resolutionChanged(int res) {
  scm_c_define("resolution", scm_from_uint(res));
  viewer->reload();
}

void Options::selectedChanged(int selected) {
  scm_c_define("only-one-patch", scm_from_uint(selected));
  viewer->update();
  updateDomain();
}

void Options::shrinkChanged(Qt::CheckState state) {
  scm_c_define("shrink-chamfers?", state == Qt::Checked ? SCM_BOOL_T : SCM_BOOL_F);
  viewer->reload();
}

void Options::tangentScaleChanged(double scale) {
  scm_c_define("tangent-scale", scm_from_double(scale));
  viewer->reload();
}

void Options::dblendChanged(int index) {
  std::array<std::string, 12> types = {
    "none", "cubic", "cubic-simple", "cubic-no-alpha", "cubic-tomi-simple", "cubic-linear",
    "quartic-simple", "quartic-no-alpha", "quartic-tomi-simple", "quartic-tomi-no-alpha",
    "quintic", "quintic-tomi"
  };
  scm_c_define("direction-blend-type", scm_from_utf8_symbol(types[index].c_str()));
  viewer->reload();
}

void Options::quinticCubicChanged(Qt::CheckState state) {
  scm_c_define("cubic-cross-degree?", state == Qt::Checked ? SCM_BOOL_T : SCM_BOOL_F);
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

double Options::scaling() const {
  return scalingBox->value();
}

void Options::exportClicked() {
  // Check if selection is valid
  size_t selected = selectedBox->value() - 1;
  SCM ribbons = scm_variable_ref(scm_c_lookup("ribbons"));
  size_t n_ribbons = scm_to_uint(scm_vector_length(ribbons));
  if (selected >= n_ribbons)
    return;

  // Get a file name
  auto name = QFileDialog::getSaveFileName(this, "Export Patch", ".", "GBS patches (*.mgbs)");
  if (name.isEmpty())
    return;
  if (!name.endsWith(".mgbs"))
    name += ".mgbs";

  // Export
  scm_c_eval_string(("(write-patch-mgbs (vector-ref ribbons %1) \"" + name + "\")")
                    .arg(selected).toUtf8().data());
}

void Options::updateDomain() const {
  const auto &objects = viewer->getObjects();
  if (!objects.empty()) {
    const auto &ph = dynamic_cast<PHGB *>(objects.front().get());
    domain_window->setDomain(ph->getDomain(selectedBox->value()));
  }
}
