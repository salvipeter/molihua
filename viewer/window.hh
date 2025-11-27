#pragma once

#include <QtWidgets/QMainWindow>

#include "viewer.hh"

class QApplication;
class QProgressBar;
class DomainWindow;

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
  void setRibbons();
  void startComputation(QString message);
  void midComputation(int percent);
  void endComputation();
  DomainWindow *getDomainWindow() const;

private:
  void setCutoff(bool mean);
  void setRange(bool mean);

  QApplication *parent;
  Viewer *viewer;
  DomainWindow *domain_window;
  QProgressBar *progress;
  QString last_directory;
};
