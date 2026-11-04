#pragma once

#include "dice.h"

#include <QtCore/QFutureWatcher>
#include <QtWidgets/QMainWindow>

class BarChartWidget;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTextBrowser;
class QToolButton;

// Результат фонового расчёта; error непуст при неудаче.
struct RollOutcome {
  bool ok = false;
  QString error;
  long long throws = 0;
  dice::SimulationResult sim;
  dice::TheoreticalResult theory;
};

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow();

private:
  void onRollClicked();
  void onRollFinished();
  void onToggleChart(bool checked);
  void setProcessing(bool busy);
  void updateViews();

  QLineEdit *diceEdit_ = nullptr;
  QLineEdit *rollsEdit_ = nullptr;
  QPushButton *rollButton_ = nullptr;
  QToolButton *chartToggle_ = nullptr;

  // Каждая панель: [0] текст, [1] график.
  QStackedWidget *simStack_ = nullptr;
  QStackedWidget *theoryStack_ = nullptr;
  QTextBrowser *simText_ = nullptr;
  QTextBrowser *theoryText_ = nullptr;
  BarChartWidget *simChart_ = nullptr;
  BarChartWidget *theoryChart_ = nullptr;

  QFutureWatcher<RollOutcome> watcher_;

  bool hasResults_ = false;
  bool chartView_ = false;
  dice::SimulationResult sim_;
  dice::TheoreticalResult theory_;
  long long throws_ = 0;
};
