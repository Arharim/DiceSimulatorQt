#include "mainwindow.h"

#include <QtWidgets/QApplication>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("DiceSimulatorQt"));
  QApplication::setOrganizationName(QStringLiteral("Arharim"));
  MainWindow window;
  window.show();
  return app.exec();
}
