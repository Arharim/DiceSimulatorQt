#pragma once

#include <QtWidgets/QWidget>

#include <QColor>
#include <utility>
#include <vector>

// Простой виджет бар-чарта на QPainter: оси, подписи, сетка.
// Специально без QtCharts, чтобы не тянуть лишний модуль ради двух гистограмм.
class BarChartWidget : public QWidget {
  Q_OBJECT

public:
  explicit BarChartWidget(QColor barColor, QWidget *parent = nullptr);

  void setData(std::vector<std::pair<int, double>> data, const QString &seriesName);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  std::vector<std::pair<int, double>> data_; // (сумма, значение), по возрастанию суммы
  QString seriesName_;
  QColor barColor_;
};
