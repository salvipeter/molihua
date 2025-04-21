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
  void onePatchChanged(Qt::CheckState state);
  void resolutionChanged(int res);
  void selectedChanged(int selected);
  void tangentScaleChanged(double scale);

private:
  Viewer *viewer;
  QSpinBox *selectedBox;
};
