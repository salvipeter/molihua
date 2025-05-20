#include <libguile.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

#include "options.hh"
#include "viewer.hh"

Options::Options(Viewer *viewer) : viewer(viewer) {
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
  tanscaleBox->setValue(1.3);
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
  dblendCombo->addItem("Cubic");
  dblendCombo->addItem("Cubic - simple");
  dblendCombo->addItem("Cubic - no alpha");
  dblendCombo->addItem("Quintic");
  dblendCombo->addItem("Quintic - Tomi");
  geometryLayout->addWidget(dblendCombo);
  connect(dblendCombo, &QComboBox::activated, this, &Options::dblendChanged);
  dblendChanged(dblendCombo->currentIndex());

  auto quinticCubicCheck = new QCheckBox("Quintic w/cubic cross-degree");
  quinticCubicCheck->setChecked(true);
  geometryLayout->addWidget(quinticCubicCheck);
  connect(quinticCubicCheck, &QCheckBox::checkStateChanged, this, &Options::quinticCubicChanged);
  quinticCubicChanged(quinticCubicCheck->checkState());

  auto misc = new QGroupBox("Miscellaneous");
  master->addWidget(misc);
  auto miscLayout = new QVBoxLayout();
  misc->setLayout(miscLayout);

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
  }
}

void Options::resolutionChanged(int res) {
  scm_c_define("resolution", scm_from_uint(res));
  viewer->reload();
}

void Options::selectedChanged(int selected) {
  scm_c_define("only-one-patch", scm_from_uint(selected));
  viewer->update();
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
  std::array<std::string, 5> types = {
    "cubic", "cubic-simple", "cubic-no-alpha", "quintic", "quintic-tomi"
  };
  scm_c_define("direction-blend-type", scm_from_utf8_symbol(types[index].c_str()));
  viewer->reload();
}

void Options::quinticCubicChanged(Qt::CheckState state) {
  scm_c_define("quintic-with-cubic-cross-degree?", state == Qt::Checked ? SCM_BOOL_T : SCM_BOOL_F);
  viewer->reload();
}
