#pragma once

#include <QWidget>

class QSpinBox;
class Viewer;

class Options : public QWidget {
  Q_OBJECT

public:
  Options(Viewer *viewer);

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
  void exportClicked();

private:
  Viewer *viewer;
  QSpinBox *selectedBox;
};
