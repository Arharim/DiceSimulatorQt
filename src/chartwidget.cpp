#include "chartwidget.h"

#include <QtGui/QPainter>

#include <algorithm>

BarChartWidget::BarChartWidget(QColor barColor, QWidget *parent)
    : QWidget(parent), barColor_(std::move(barColor)) {
  setMinimumSize(200, 150);
}

void BarChartWidget::setData(std::vector<std::pair<int, double>> data, const QString &seriesName) {
  data_ = std::move(data);
  seriesName_ = seriesName;
  update();
}

void BarChartWidget::paintEvent(QPaintEvent *) {
  QPainter painter(this);
  painter.fillRect(rect(), palette().window());

  constexpr int kLeft = 64, kBottom = 28, kTop = 30, kRight = 12;
  const QRectF plot(kLeft, kTop, width() - kLeft - kRight, height() - kTop - kBottom);
  if (data_.empty() || plot.width() <= 1 || plot.height() <= 1) {
    return;
  }

  double maxValue = 0.0;
  for (const auto &[sum, value] : data_) {
    maxValue = std::max(maxValue, value);
  }
  if (maxValue <= 0.0) {
    maxValue = 1.0;
  }

  // Заголовок
  QFont boldFont = font();
  boldFont.setBold(true);
  painter.setFont(boldFont);
  painter.setPen(palette().windowText().color());
  painter.drawText(QRect(0, 4, width(), kTop - 6), Qt::AlignCenter, seriesName_);

  QFont baseFont = font();
  baseFont.setBold(false);
  painter.setFont(baseFont);

  // Сетка и подписи оси Y
  for (int i = 0; i <= 4; ++i) {
    const double frac = i / 4.0;
    const double y = plot.bottom() - frac * plot.height();
    painter.setPen(QColor(210, 210, 210));
    painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
    painter.setPen(QColor(96, 96, 96));
    painter.drawText(QRectF(0, y - 9, plot.left() - 6, 18),
                     Qt::AlignRight | Qt::AlignVCenter,
                     QString::number(maxValue * frac, 'g', 3));
  }

  // Столбцы
  const int n = static_cast<int>(data_.size());
  const double barWidth = plot.width() / n;
  painter.setPen(Qt::NoPen);
  for (int i = 0; i < n; ++i) {
    const double h = data_[i].second / maxValue * plot.height();
    if (h <= 0.0) {
      continue;
    }
    const double gap = barWidth > 2.0 ? 1.0 : 0.0;
    painter.fillRect(
        QRectF(plot.left() + i * barWidth, plot.bottom() - h, std::max(barWidth - gap, 0.5), h),
        barColor_);
  }

  // Подписи оси X: не более ~16, чтобы не слипались
  painter.setPen(QColor(96, 96, 96));
  const int step = std::max(1, (n + 15) / 16);
  for (int i = 0; i < n; i += step) {
    const double cx = plot.left() + (i + 0.5) * barWidth;
    painter.drawText(QRectF(cx - 40, plot.bottom() + 4, 80, 18),
                     Qt::AlignCenter,
                     QString::number(data_[i].first));
  }
}
