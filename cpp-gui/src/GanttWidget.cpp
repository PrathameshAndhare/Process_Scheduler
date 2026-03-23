#include "GanttWidget.h"

#include <cmath>
#include <QColor>
#include <QPainter>
#include <QPaintEvent>
#include <QString>

GanttWidget::GanttWidget(QWidget* parent) : QWidget(parent) {
  setMinimumHeight(140);
}

void GanttWidget::setTimeline(
    const std::vector<ProcessSlice>& timeline,
    const std::unordered_map<int, QColor>& pidToColor) {
  timeline_ = timeline;
  pidToColor_ = pidToColor;
  update();
}

void GanttWidget::paintEvent(QPaintEvent* /*event*/) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);

  const int w = width();
  const int h = height();

  // Top row: colored rectangles
  const int topY = 20;
  const int rectH = 70;
  const int marginX = 10;
  const int availableW = std::max(1, w - 2 * marginX);

  long long total = 0;
  for (const auto& s : timeline_) total += s.burst_time;

  if (total <= 0 || timeline_.empty()) {
    p.drawText(marginX, topY + rectH / 2, "Run a simulation to see the timeline");
    return;
  }

  long long xAcc = 0;
  long long t = 0;

  for (const auto& s : timeline_) {
    const long long sliceW = (s.burst_time * availableW) / total;
    xAcc += sliceW;
    t += s.burst_time;
  }

  // Draw rectangles; use floating widths for visual accuracy (tick labels just need approximate x).
  int cursorX = marginX;
  for (size_t i = 0; i < timeline_.size(); i++) {
    const auto& s = timeline_[i];

    const double frac = static_cast<double>(s.burst_time) / static_cast<double>(total);
    int sliceW = static_cast<int>(std::round(frac * availableW));
    sliceW = std::max(1, sliceW);

    QColor fill;
    QString label;
    if (s.process_id == -1) {
      fill = QColor(220, 220, 220);
      label = "Idle";
    } else {
      auto it = pidToColor_.find(s.process_id);
      fill = (it != pidToColor_.end()) ? it->second : QColor(100, 149, 237);
      label = "P" + QString::number(s.process_id);
    }

    QRect rect(cursorX, topY, sliceW, rectH);
    p.setPen(Qt::NoPen);
    p.setBrush(fill);
    p.drawRect(rect);

    p.setPen(QColor(30, 30, 30));
    QFont f = p.font();
    f.setPointSize(10);
    p.setFont(f);
    p.drawText(rect, Qt::AlignCenter, label);

    cursorX += sliceW;
  }

  // Bottom row: tick labels (slice start times + final time)
  const int tickY = topY + rectH + 18;
  QFont tf = p.font();
  tf.setPointSize(9);
  p.setFont(tf);
  p.setPen(QColor(40, 40, 40));

  // We draw at boundaries[0..n] using the same algorithm order.
  long long cumulative = 0;
  int x = marginX;

  // Label each slice start time, and label the end time at the final boundary.
  for (size_t i = 0; i < timeline_.size(); i++) {
    p.drawText(x, tickY, QString::number(cumulative));
    const long long sliceW = (timeline_[i].burst_time * availableW) / total;
    cumulative += timeline_[i].burst_time;
    x += static_cast<int>(sliceW);
  }
  p.drawText(x, tickY, QString::number(cumulative));
}

