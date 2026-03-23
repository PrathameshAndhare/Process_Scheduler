#pragma once

#include <QColor>
#include <QComboBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QWidget>
#include <vector>

#include "../../cpp/src/metrics.h"

class GanttWidget;

class MainWindow : public QWidget {
public:
  MainWindow(QWidget* parent = nullptr);

private:
  struct GuiProcess {
    int process_id;
    long long arrival_time;
    long long burst_time;
    QColor color;
  };

  void runSimulation();
  void updateMetricsTable(
      const std::unordered_map<int, PerProcessMetrics>& perProc);

  std::vector<GuiProcess> processes_;

  // UI
  QComboBox* algorithmCombo_ = nullptr;
  QSpinBox* quantumSpin_ = nullptr;
  QSpinBox* arrivalSpin_ = nullptr;
  QSpinBox* burstSpin_ = nullptr;
  QPushButton* addColorBtn_ = nullptr;
  QColor pendingColor_;
  QPushButton* addProcessBtn_ = nullptr;
  QPushButton* removeSelectedBtn_ = nullptr;
  QTableWidget* processesTable_ = nullptr;
  QPushButton* runBtn_ = nullptr;

  QLabel* avgWaitingLabel_ = nullptr;
  QLabel* avgTurnaroundLabel_ = nullptr;
  QLabel* throughputLabel_ = nullptr;
  QLabel* cpuUtilLabel_ = nullptr;

  QTableWidget* metricsTable_ = nullptr;
  GanttWidget* gantt_ = nullptr;
};

