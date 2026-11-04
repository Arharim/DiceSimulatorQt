#include "mainwindow.h"

#include "chartwidget.h"

#include <QtConcurrent/QtConcurrent>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QToolButton>

#include <algorithm>
#include <random>

namespace {

constexpr int kTextIndex = 0;
constexpr int kChartIndex = 1;

QString buildSimulationHtml(const dice::SimulationResult &sim, long long throws) {
  QString html = QStringLiteral("<h3 style=\"color:#006400;\">&#127922; Симуляция %1 бросков</h3>")
                     .arg(throws);
  html += QStringLiteral("<p style=\"color:#0000FF;\">Средняя сумма: %1</p>")
              .arg(QString::number(sim.empirical_mean, 'f', 4));
  html += QStringLiteral("<p><b>Частоты:</b></p>");
  for (const auto &[sum, count] : sim.frequency) {
    const double share = static_cast<double>(count) / static_cast<double>(throws);
    html += QStringLiteral("<p>&nbsp;&nbsp;Сумма %1: "
                           "<b style=\"color:#008000;\">%2 раз</b> "
                           "<span style=\"color:#00008B;\">(%3)</span></p>")
                .arg(sum)
                .arg(count)
                .arg(QString::number(share, 'f', 4));
  }
  return html;
}

QString buildTheoryHtml(const dice::TheoreticalResult &theory) {
  QString html =
      QStringLiteral("<h3 style=\"color:#00008B;\">&#128204; Теоретические расчёты</h3>");
  const auto line = [](const QString &label, double value) {
    return QStringLiteral("<p style=\"color:#0000FF;\">&nbsp;&nbsp;%1: %2</p>")
        .arg(label, QString::number(value, 'f', 4));
  };
  html += line(QStringLiteral("Мат. ожидание (M)"), theory.mean);
  html += line(QStringLiteral("Дисперсия (D)"), theory.variance);
  html += line(QStringLiteral("Отклонение (&sigma;)"), theory.std_dev);
  double probSum = 0.0;
  for (const auto &[k, p] : theory.probabilities) {
    probSum += p;
  }
  html += line(QStringLiteral("Сумма вероятностей"), probSum);
  html += QStringLiteral("<p><b>Вероятности:</b></p>");
  for (const auto &[sum, p] : theory.probabilities) {
    html += QStringLiteral("<p>&nbsp;&nbsp;Сумма %1: "
                           "<b style=\"color:#00008B;\">%2</b></p>")
                .arg(sum)
                .arg(QString::number(p, 'f', 8));
  }
  return html;
}

// Готовит набор данных (сумма -> значение) для BarChartWidget.
std::vector<std::pair<int, double>> toChartData(const std::map<int, double> &data) {
  std::vector<std::pair<int, double>> out;
  out.reserve(data.size());
  for (const auto &[sum, value] : data) {
    out.emplace_back(sum, value);
  }
  return out;
}

QTextBrowser *makeTextBrowser() {
  auto *browser = new QTextBrowser;
  browser->setReadOnly(true);
  browser->setOpenExternalLinks(false);
  browser->setFrameShape(QFrame::NoFrame);
  return browser;
}

} // namespace

MainWindow::MainWindow() {
  setWindowTitle(QStringLiteral("Симулятор Дайсов"));
  resize(1100, 655);
  setWindowIcon(QIcon(QStringLiteral(":/dice_d20_icon_.ico")));

  auto *central = new QWidget;
  setCentralWidget(central);
  auto *rootLayout = new QVBoxLayout(central);
  rootLayout->setContentsMargins(16, 16, 16, 16);

  // --- Ввод ---
  auto *inputRow = new QHBoxLayout;
  inputRow->setSpacing(8);
  diceEdit_ = new QLineEdit;
  diceEdit_->setPlaceholderText(QStringLiteral("Дайсы (например, 1d6 2d8 1d4)"));
  diceEdit_->setClearButtonEnabled(true);
  rollsEdit_ = new QLineEdit(QStringLiteral("100"));
  rollsEdit_->setPlaceholderText(QStringLiteral("Количество бросков"));
  rollsEdit_->setClearButtonEnabled(true);
  rollsEdit_->setMaximumWidth(150);
  rollButton_ = new QPushButton(QStringLiteral("Бросить дайсы"));
  chartToggle_ = new QToolButton;
  chartToggle_->setText(QStringLiteral("&#128260;")); // 🔄
  chartToggle_->setToolTip(QStringLiteral("Переключить вид (Текст/График)"));
  chartToggle_->setCheckable(true);

  inputRow->addWidget(diceEdit_, 1);
  inputRow->addWidget(rollsEdit_);
  inputRow->addWidget(rollButton_);
  inputRow->addWidget(chartToggle_);
  rootLayout->addLayout(inputRow);

  // --- Результаты: две панели ---
  auto *columns = new QHBoxLayout;
  columns->setSpacing(8);

  auto *simGroup = new QGroupBox(QStringLiteral("Результаты симуляции"));
  simStack_ = new QStackedWidget;
  simText_ = makeTextBrowser();
  simChart_ = new BarChartWidget(QColor(0x4C, 0xAF, 0x50)); // зелёный, как в оригинале
  simStack_->addWidget(simText_);
  simStack_->addWidget(simChart_);
  auto *simLayout = new QVBoxLayout(simGroup);
  simLayout->addWidget(simStack_);

  auto *theoryGroup = new QGroupBox(QStringLiteral("Теоретические расчеты"));
  theoryStack_ = new QStackedWidget;
  theoryText_ = makeTextBrowser();
  theoryChart_ = new BarChartWidget(QColor(0x21, 0x96, 0xF3)); // синий, как в оригинале
  theoryStack_->addWidget(theoryText_);
  theoryStack_->addWidget(theoryChart_);
  auto *theoryLayout = new QVBoxLayout(theoryGroup);
  theoryLayout->addWidget(theoryStack_);

  columns->addWidget(simGroup, 1);
  columns->addWidget(theoryGroup, 1);
  rootLayout->addLayout(columns, 1);

  // --- Поведение ---
  connect(rollButton_, &QPushButton::clicked, this, &MainWindow::onRollClicked);
  connect(chartToggle_, &QToolButton::toggled, this, &MainWindow::onToggleChart);
  connect(&watcher_, &QFutureWatcher<RollOutcome>::finished, this, &MainWindow::onRollFinished);

  updateViews();
}

void MainWindow::onRollClicked() {
  std::vector<dice::DiceGroup> diceList;
  try {
    diceList = dice::parse(diceEdit_->text().toStdString());
  } catch (const std::exception &e) {
    QMessageBox::warning(this, QStringLiteral("Ошибка"), QString::fromUtf8(e.what()));
    return;
  }

  bool ok = false;
  const long long throws = rollsEdit_->text().toLongLong(&ok);
  if (!ok || throws < 1) {
    QMessageBox::warning(this,
                         QStringLiteral("Ошибка"),
                         QStringLiteral("Введите корректное положительное количество бросков!"));
    return;
  }
  if (throws > dice::kMaxThrows) {
    QMessageBox::warning(
        this,
        QStringLiteral("Ошибка"),
        QStringLiteral("Слишком много бросков (максимум %1)!").arg(qlonglong(dice::kMaxThrows)));
    return;
  }

  setProcessing(true);
  auto future = QtConcurrent::run([diceList, throws]() -> RollOutcome {
    RollOutcome out;
    out.throws = throws;
    try {
      std::mt19937_64 rng{std::random_device{}()};
      out.sim = dice::simulate(diceList, throws, rng);
      out.theory = dice::theoretical(diceList);
      out.ok = true;
    } catch (const std::exception &e) {
      out.error = QString::fromUtf8(e.what());
    }
    return out;
  });
  watcher_.setFuture(future);
}

void MainWindow::onRollFinished() {
  setProcessing(false);
  const RollOutcome out = watcher_.result();
  if (!out.ok) {
    QMessageBox::warning(this, QStringLiteral("Ошибка"), out.error);
    return;
  }
  sim_ = out.sim;
  theory_ = out.theory;
  throws_ = out.throws;
  hasResults_ = true;
  updateViews();
}

void MainWindow::onToggleChart(bool checked) {
  chartView_ = checked;
  updateViews();
}

void MainWindow::setProcessing(bool busy) {
  rollButton_->setEnabled(!busy);
  diceEdit_->setEnabled(!busy);
  rollsEdit_->setEnabled(!busy);
  rollButton_->setText(busy ? QStringLiteral("Вычисляю...") : QStringLiteral("Бросить дайсы"));
}

void MainWindow::updateViews() {
  simStack_->setCurrentIndex(chartView_ ? kChartIndex : kTextIndex);
  theoryStack_->setCurrentIndex(chartView_ ? kChartIndex : kTextIndex);

  if (!hasResults_) {
    simText_->setHtml(QStringLiteral("<p style=\"color:#808080;\">Нажмите 'Бросить дайсы' "
                                     "для начала симуляции.</p>"));
    theoryText_->clear();
    return;
  }

  if (chartView_) {
    // Симуляция нормализована в доли, чтобы оба графика были в одной шкале.
    std::map<int, double> simShares;
    for (const auto &[sum, count] : sim_.frequency) {
      simShares[sum] = static_cast<double>(count) / static_cast<double>(throws_);
    }
    simChart_->setData(toChartData(simShares), QStringLiteral("Частота выпадения (доля)"));
    theoryChart_->setData(toChartData(theory_.probabilities),
                          QStringLiteral("Теоретическая вероятность"));
  } else {
    simText_->setHtml(buildSimulationHtml(sim_, throws_));
    theoryText_->setHtml(buildTheoryHtml(theory_));
  }
}
