#pragma once

#include <QtWidgets/QMainWindow>

#include "viewer.hh"

class QApplication;
class QProgressBar;

class Window : public QMainWindow {
  Q_OBJECT

public:
  explicit Window(QApplication *parent);

private slots:
  void open(bool clear_others);
  void setMeanCutoff();
  void setMeanRange();
  void setGaussCutoff();
  void setGaussRange();
  void setSlicing();
  void startComputation(QString message);
  void midComputation(int percent);
  void endComputation();

private:
  void setCutoff(bool mean);
  void setRange(bool mean);

  QApplication *parent;
  Viewer *viewer;
  QProgressBar *progress;
  QString last_directory;
};
