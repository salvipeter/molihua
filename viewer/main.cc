#include <QtWidgets/QApplication>

#include "scheme-wrapper.hh"
#include "window.hh"

int main(int argc, char **argv) {
  SchemeWrapper::initialize();
  QApplication app(argc, argv);
  Window window(&app);
  window.show();
  std::setlocale(LC_ALL, "C");
  QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
  return app.exec();
}
