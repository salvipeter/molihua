#include <memory>

#include <libguile.h>

#include <QtWidgets>

#include "options.hh"
#include "window.hh"

Window::Window(QApplication *parent) :
  QMainWindow(), parent(parent), last_directory(".")
{
  setWindowTitle(tr("Polyhedral Modeling with CD-GB Patches"));
  setStatusBar(new QStatusBar);
  progress = new QProgressBar;
  progress->setMinimum(0); progress->setMaximum(100);
  progress->hide();
  statusBar()->addPermanentWidget(progress);

  viewer = new Viewer(this);
  connect(viewer, &Viewer::startComputation, this, &Window::startComputation);
  connect(viewer, &Viewer::midComputation, this, &Window::midComputation);
  connect(viewer, &Viewer::endComputation, this, &Window::endComputation);
  setCentralWidget(viewer);

  auto openAction = new QAction(tr("&Open"), this);
  openAction->setShortcut(tr("Ctrl+O"));
  openAction->setStatusTip(tr("Load a model from a file"));
  connect(openAction, &QAction::triggered, [this](){ open(true); });

  auto quitAction = new QAction(tr("&Quit"), this);
  quitAction->setShortcut(tr("Ctrl+Q"));
  quitAction->setStatusTip(tr("Quit the program"));
  connect(quitAction, &QAction::triggered, this, &Window::close);

  auto cutoffAction = new QAction(tr("Set mean &cutoff ratio"), this);
  cutoffAction->setStatusTip(tr("Set mean map cutoff ratio"));
  connect(cutoffAction, &QAction::triggered, this, &Window::setMeanCutoff);

  auto rangeAction = new QAction(tr("Set mean &range"), this);
  rangeAction->setStatusTip(tr("Set mean map range"));
  connect(rangeAction, &QAction::triggered, this, &Window::setMeanRange);

  auto gcutoffAction = new QAction(tr("Set Gaussian &cutoff ratio"), this);
  gcutoffAction->setStatusTip(tr("Set Gaussian map cutoff ratio"));
  connect(gcutoffAction, &QAction::triggered, this, &Window::setGaussCutoff);

  auto grangeAction = new QAction(tr("Set Gaussian &range"), this);
  grangeAction->setStatusTip(tr("Set Gaussian map range"));
  connect(grangeAction, &QAction::triggered, this, &Window::setGaussRange);

  auto slicingAction = new QAction(tr("Set &slicing parameters"), this);
  slicingAction->setStatusTip(tr("Set contouring direction and scaling"));
  connect(slicingAction, &QAction::triggered, this, &Window::setSlicing);

  auto fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(openAction);
  fileMenu->addAction(quitAction);

  auto visMenu = menuBar()->addMenu(tr("&Visualization"));
  visMenu->addAction(cutoffAction);
  visMenu->addAction(rangeAction);
  visMenu->addAction(gcutoffAction);
  visMenu->addAction(grangeAction);
  visMenu->addAction(slicingAction);

  auto scroll = new QScrollArea();
  scroll->setWidget(new Options(viewer));

  auto dock = new QDockWidget("Options", this);
  dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  dock->setFeatures(QDockWidget::DockWidgetMovable);
  dock->setWidget(scroll);
  addDockWidget(Qt::RightDockWidgetArea, dock);
}

void Window::open(bool clear_others) {
  auto filename =
    QFileDialog::getOpenFileName(this, tr("Open File"), last_directory,
                                 tr("Cages (*.obj);;"
                                    "All files (*.*)"));
  if (filename.isEmpty())
    return;
  last_directory = QFileInfo(filename).absolutePath();

  if (clear_others)
    viewer->deleteObjects();

  if (!viewer->open(filename.toUtf8().data()))
    QMessageBox::warning(this, tr("Cannot open file"),
                         tr("Could not open file: ") + filename + ".");
}

void Window::setMeanCutoff() {
  setCutoff(true);
}

void Window::setMeanRange() {
  setRange(true);
}

void Window::setGaussCutoff() {
  setCutoff(false);
}

void Window::setGaussRange() {
  setRange(false);
}

void Window::setCutoff(bool mean) {
  auto dlg = std::make_unique<QDialog>(this);
  auto *hb1    = new QHBoxLayout,
       *hb2    = new QHBoxLayout;
  auto *vb     = new QVBoxLayout;
  auto *text   = new QLabel(tr("Cutoff ratio:"));
  auto *sb     = new QDoubleSpinBox;
  auto *cancel = new QPushButton(tr("Cancel"));
  auto *ok     = new QPushButton(tr("Ok"));

  sb->setDecimals(3);
  sb->setRange(0.001, 0.5);
  sb->setSingleStep(0.01);
  sb->setValue(mean ? viewer->getMeanCutoffRatio() : viewer->getGaussCutoffRatio());
  connect(cancel, &QPushButton::pressed, dlg.get(), &QDialog::reject);
  connect(ok,     &QPushButton::pressed, dlg.get(), &QDialog::accept);
  ok->setDefault(true);

  hb1->addWidget(text);
  hb1->addWidget(sb);
  hb2->addWidget(cancel);
  hb2->addWidget(ok);
  vb->addLayout(hb1);
  vb->addLayout(hb2);

  dlg->setWindowTitle(tr("Set ratio"));
  dlg->setLayout(vb);

  if(dlg->exec() == QDialog::Accepted) {
    if (mean)
      viewer->setMeanCutoffRatio(sb->value());
    else
      viewer->setGaussCutoffRatio(sb->value());
    viewer->update();
  }
}

void Window::setRange(bool mean) {
  QDialog dlg(this);
  auto *grid   = new QGridLayout;
  auto *text1  = new QLabel(tr("Min:")),
       *text2  = new QLabel(tr("Max:"));
  auto *sb1    = new QDoubleSpinBox,
       *sb2    = new QDoubleSpinBox;
  auto *cancel = new QPushButton(tr("Cancel"));
  auto *ok     = new QPushButton(tr("Ok"));

  // The range of the spinbox controls the number of displayable digits,
  // so setting it to a large value results in a very wide window.
  double max = 1000.0; // std::numeric_limits<double>::max();
  sb1->setDecimals(5);                 sb2->setDecimals(5);
  sb1->setRange(-max, 0.0);            sb2->setRange(0.0, max);
  sb1->setSingleStep(0.01);            sb2->setSingleStep(0.01);
  if (mean) {
    sb1->setValue(viewer->getMeanMin());
    sb2->setValue(viewer->getMeanMax());
  } else {
    sb1->setValue(viewer->getGaussMin());
    sb2->setValue(viewer->getGaussMax());
  }
  connect(cancel, &QPushButton::pressed, &dlg, &QDialog::reject);
  connect(ok,     &QPushButton::pressed, &dlg, &QDialog::accept);
  ok->setDefault(true);

  grid->addWidget( text1, 1, 1, Qt::AlignRight);
  grid->addWidget(   sb1, 1, 2);
  grid->addWidget( text2, 2, 1, Qt::AlignRight);
  grid->addWidget(   sb2, 2, 2);
  grid->addWidget(cancel, 3, 1);
  grid->addWidget(    ok, 3, 2);

  dlg.setWindowTitle(tr("Set range"));
  dlg.setLayout(grid);

  if(dlg.exec() == QDialog::Accepted) {
    if (mean) {
      viewer->setMeanMin(sb1->value());
      viewer->setMeanMax(sb2->value());
    } else {
      viewer->setGaussMin(sb1->value());
      viewer->setGaussMax(sb2->value());
    }
    viewer->update();
  }
}

void Window::setSlicing() {
  auto dlg = std::make_unique<QDialog>(this);
  auto *hb1    = new QHBoxLayout,
       *hb2    = new QHBoxLayout,
       *hb3    = new QHBoxLayout;
  auto *vb     = new QVBoxLayout;
  auto *text_v = new QLabel(tr("Slicing direction:"));
  QDoubleSpinBox *sb_v[3];
  auto *text_s = new QLabel(tr("Slicing scaling:"));
  auto *sb_s   = new QDoubleSpinBox;
  auto *cancel = new QPushButton(tr("Cancel"));
  auto *ok     = new QPushButton(tr("Ok"));

  for (int i = 0; i < 3; ++i) {
    sb_v[i] = new QDoubleSpinBox;
    sb_v[i]->setDecimals(3);
    sb_v[i]->setRange(-1, 1);
    sb_v[i]->setSingleStep(0.01);
    sb_v[i]->setValue(viewer->getSlicingDir()[i]);
  }
  sb_s->setDecimals(6);
  sb_s->setRange(0, 10000);
  sb_s->setSingleStep(1);
  sb_s->setValue(viewer->getSlicingScaling());
  connect(cancel, &QPushButton::pressed, dlg.get(), &QDialog::reject);
  connect(ok,     &QPushButton::pressed, dlg.get(), &QDialog::accept);
  ok->setDefault(true);

  hb1->addWidget(text_v);
  hb1->addWidget(sb_v[0]); hb1->addWidget(sb_v[1]); hb1->addWidget(sb_v[2]);
  hb2->addWidget(text_s);
  hb2->addWidget(sb_s);
  hb3->addWidget(cancel);
  hb3->addWidget(ok);
  vb->addLayout(hb1);
  vb->addLayout(hb2);
  vb->addLayout(hb3);

  dlg->setWindowTitle(tr("Set slicing"));
  dlg->setLayout(vb);

  if(dlg->exec() == QDialog::Accepted) {
    viewer->setSlicingDir(sb_v[0]->value(), sb_v[1]->value(), sb_v[2]->value());
    viewer->setSlicingScaling(sb_s->value());
    viewer->update();
  }
}

void Window::startComputation(QString message) {
  statusBar()->showMessage(message);
  progress->setValue(0);
  progress->show();
  parent->processEvents(QEventLoop::ExcludeUserInputEvents);
}

void Window::midComputation(int percent) {
  progress->setValue(percent);
  parent->processEvents(QEventLoop::ExcludeUserInputEvents);
}

void Window::endComputation() {
  progress->hide();
  statusBar()->clearMessage();
}
