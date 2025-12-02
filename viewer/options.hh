#pragma once

#include <QWidget>

class QCheckBox;
class QDoubleSpinBox;
class QSpinBox;
class Viewer;
class DomainWindow;

class Options : public QWidget {
  Q_OBJECT

public:
  static Options *instance(Viewer *v = nullptr, DomainWindow *d = nullptr);
  std::optional<double> reparam() const;
  double scaling() const;
  bool hsplit() const;
  bool C1() const;
  bool biharmonic() const;
  bool hWidth() const;

public slots:
  void fullnessChanged(double full);
  void mvecScaleChanged(double scale);
  void onePatchChanged(Qt::CheckState state);
  void resolutionChanged(int res);
  void selectedChanged(int selected);
  void edgeChanged(Qt::CheckState state);
  void shrinkChanged(Qt::CheckState state, bool inwards);
  void tangentScaleChanged(double scale);
  void dblendChanged(int index);
  void quinticCubicChanged(Qt::CheckState state);
  void bsplineConcaveChanged(Qt::CheckState state);
  void exportClicked();

private slots:
  void updateDomain() const;

private:
  // Singleton pattern
  Options(Viewer *viewer, DomainWindow *domain_window);
  Options(const Options &) = delete;
  Options &operator=(const Options &) = delete;
  Options(Options &) = delete;
  Options &operator=(Options &) = delete;

  DomainWindow *domain_window;
  Viewer *viewer;
  QSpinBox *selectedBox;
  QCheckBox *bsplineConcaveCheck;
  QDoubleSpinBox *reparamBox, *scalingBox;
  QCheckBox *hsplitCheck, *C1Check, *biharmonicCheck, *hwidthCheck;
};
