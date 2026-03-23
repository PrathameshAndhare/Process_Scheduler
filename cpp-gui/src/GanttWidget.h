#pragma once

#include <QColor>
#include <QMap>
#include <QWidget>
#include <unordered_map>
#include <vector>

#include "../../cpp/src/scheduling_algorithms.h"

class GanttWidget : public QWidget {
public:
  explicit GanttWidget(QWidget* parent = nullptr);

  void setTimeline(const std::vector<ProcessSlice>& timeline,
                    const std::unordered_map<int, QColor>& pidToColor);

protected:
  void paintEvent(QPaintEvent* event) override;

private:
  std::vector<ProcessSlice> timeline_;
  std::unordered_map<int, QColor> pidToColor_;
};

