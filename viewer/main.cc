#include <QtWidgets/QApplication>

#include <libguile.h>

#include "window.hh"

int main(int argc, char **argv) {
  scm_init_guile();
  QApplication app(argc, argv);
  Window window(&app);
  window.show();
  std::setlocale(LC_ALL, "C");
  QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
  return app.exec();
}
