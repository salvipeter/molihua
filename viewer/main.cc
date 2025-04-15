#include <QtWidgets/QApplication>

#include <libguile.h>

#include "window.hh"

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  Window window(&app);
  window.show();
  scm_init_guile();
  return app.exec();
}
