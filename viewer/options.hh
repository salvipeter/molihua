#pragma once

#include <QWidget>

class QCheckBox;
class QDoubleSpinBox;
class QSpinBox;
class Viewer;

class Options : public QWidget {
  Q_OBJECT

public:
  static Options *instance(Viewer *v = nullptr); // Viewer needed only at construction time
  std::optional<double> reparam() const;

public slots:
  void fullnessChanged(double full);
  void mvecScaleChanged(double scale);
  void onePatchChanged(Qt::CheckState state);
  void resolutionChanged(int res);
  void selectedChanged(int selected);
  void shrinkChanged(Qt::CheckState state);
  void tangentScaleChanged(double scale);
  void dblendChanged(int index);
  void quinticCubicChanged(Qt::CheckState state);
  void bsplineConcaveChanged(Qt::CheckState state);
  void exportClicked();

private:
  // Singleton pattern
  Options(Viewer *viewer);
  Options(const Options &) = delete;
  Options &operator=(const Options &) = delete;
  Options(Options &) = delete;
  Options &operator=(Options &) = delete;

  Viewer *viewer;
  QSpinBox *selectedBox;
  QCheckBox *bsplineConcaveCheck;
  QDoubleSpinBox *reparamBox;
};
