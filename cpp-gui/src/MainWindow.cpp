#include "MainWindow.h"

#include "GanttWidget.h"

#include "../../cpp/src/metrics.h"
#include "../../cpp/src/scheduling_algorithms.h"

#include <QColorDialog>
#include <QComboBox>
#include <QFont>
#include <QGridLayout>
#include <QAbstractItemView>
#include <QLabel>
#include <QMessageBox>
#include <QModelIndexList>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent) : QWidget(parent) {
  setWindowTitle("Scheduling Algorithm Simulator (C++ GUI)");
  resize(980, 650);

  pendingColor_ = QColor(80, 160, 255);

  auto* root = new QVBoxLayout(this);

  auto* title = new QLabel("CPU Scheduling Algorithm Simulator");
  QFont tf = title->font();
  tf.setPointSize(16);
  tf.setBold(true);
  title->setFont(tf);
  root->addWidget(title);

  // Algorithm / Quantum row
  auto* algoRow = new QHBoxLayout();
  root->addLayout(algoRow);

  algoRow->addWidget(new QLabel("Algorithm:"));
  algorithmCombo_ = new QComboBox();
  algorithmCombo_->addItems({"FCFS", "RR", "SJF", "SRTF"});
  algoRow->addWidget(algorithmCombo_);

  algoRow->addWidget(new QLabel("Quantum:"));
  quantumSpin_ = new QSpinBox();
  quantumSpin_->setRange(1, 100);
  quantumSpin_->setValue(2);
  algoRow->addWidget(quantumSpin_);

  quantumSpin_->setEnabled(false); // default until RR chosen

  // Process input row
  auto* inputRow = new QHBoxLayout();
  root->addLayout(inputRow);

  inputRow->addWidget(new QLabel("Arrival:"));
  arrivalSpin_ = new QSpinBox();
  arrivalSpin_->setRange(0, 100);
  inputRow->addWidget(arrivalSpin_);

  inputRow->addWidget(new QLabel("Burst:"));
  burstSpin_ = new QSpinBox();
  burstSpin_->setRange(1, 100);
  burstSpin_->setValue(1);
  inputRow->addWidget(burstSpin_);

  addColorBtn_ = new QPushButton("Pick Color");
  addColorBtn_->setStyleSheet(
      QString("background-color: %1;").arg(pendingColor_.name()));
  inputRow->addWidget(addColorBtn_);

  addProcessBtn_ = new QPushButton("Add Process");
  inputRow->addWidget(addProcessBtn_);

  removeSelectedBtn_ = new QPushButton("Remove Selected");
  inputRow->addWidget(removeSelectedBtn_);

  processesTable_ = new QTableWidget();
  processesTable_->setColumnCount(4);
  processesTable_->setHorizontalHeaderLabels({"PID", "Arrival", "Burst", "Color"});
  processesTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
  processesTable_->setSelectionMode(QAbstractItemView::SingleSelection);
  processesTable_->setMinimumHeight(160);
  root->addWidget(processesTable_);

  // Gantt chart
  gantt_ = new GanttWidget();
  root->addWidget(gantt_);

  // Run button
  runBtn_ = new QPushButton("Run Simulation");
  root->addWidget(runBtn_);

  // Summary labels
  auto* metricsRow = new QHBoxLayout();
  root->addLayout(metricsRow);

  avgWaitingLabel_ = new QLabel("-");
  avgTurnaroundLabel_ = new QLabel("-");
  throughputLabel_ = new QLabel("-");
  cpuUtilLabel_ = new QLabel("-");

  metricsRow->addWidget(new QLabel("Avg Waiting:"));
  metricsRow->addWidget(avgWaitingLabel_);
  metricsRow->addWidget(new QLabel("Avg Turnaround:"));
  metricsRow->addWidget(avgTurnaroundLabel_);
  metricsRow->addWidget(new QLabel("Throughput:"));
  metricsRow->addWidget(throughputLabel_);
  metricsRow->addWidget(new QLabel("CPU Utilization:"));
  metricsRow->addWidget(cpuUtilLabel_);

  // Per-process metrics table
  metricsTable_ = new QTableWidget();
  metricsTable_->setColumnCount(5);
  metricsTable_->setHorizontalHeaderLabels(
      {"PID", "Arrival", "Burst", "Waiting", "Turnaround"});
  metricsTable_->setMinimumHeight(150);
  root->addWidget(metricsTable_);

  // Connections
  connect(addColorBtn_, &QPushButton::clicked, this, [this]() {
    QColor c = QColorDialog::getColor(pendingColor_, this, "Pick process color");
    if (!c.isValid()) return;
    pendingColor_ = c;
    addColorBtn_->setStyleSheet(
        QString("background-color: %1;").arg(pendingColor_.name()));
  });

  connect(addProcessBtn_, &QPushButton::clicked, this, [this]() {
    GuiProcess gp;
    gp.process_id = static_cast<int>(processes_.size()) + 1;
    gp.arrival_time = arrivalSpin_->value();
    gp.burst_time = burstSpin_->value();
    gp.color = pendingColor_;

    processes_.push_back(gp);

    const int r = processesTable_->rowCount();
    processesTable_->insertRow(r);

    auto* idItem = new QTableWidgetItem(QString::number(gp.process_id));
    auto* arrItem = new QTableWidgetItem(QString::number(gp.arrival_time));
    auto* burstItem = new QTableWidgetItem(QString::number(gp.burst_time));
    auto* colorItem = new QTableWidgetItem(gp.color.name());
    colorItem->setBackground(gp.color);

    processesTable_->setItem(r, 0, idItem);
    processesTable_->setItem(r, 1, arrItem);
    processesTable_->setItem(r, 2, burstItem);
    processesTable_->setItem(r, 3, colorItem);
  });

  connect(removeSelectedBtn_, &QPushButton::clicked, this, [this]() {
    auto* sel = processesTable_->selectionModel();
    if (!sel) return;
    QModelIndexList rows = sel->selectedRows();
    if (rows.isEmpty()) return;

    int row = rows.first().row();
    if (row < 0 || row >= static_cast<int>(processes_.size())) return;

    processes_.erase(processes_.begin() + row);
    processesTable_->setRowCount(0);

    // Reassign PIDs to keep UI tidy (algorithms just require consistent IDs).
    for (size_t i = 0; i < processes_.size(); i++) {
      processes_[i].process_id = static_cast<int>(i) + 1;
      const int r = static_cast<int>(i);
      processesTable_->insertRow(r);

      auto* idItem = new QTableWidgetItem(QString::number(processes_[i].process_id));
      auto* arrItem = new QTableWidgetItem(QString::number(processes_[i].arrival_time));
      auto* burstItem = new QTableWidgetItem(QString::number(processes_[i].burst_time));
      auto* colorItem = new QTableWidgetItem(processes_[i].color.name());
      colorItem->setBackground(processes_[i].color);

      processesTable_->setItem(r, 0, idItem);
      processesTable_->setItem(r, 1, arrItem);
      processesTable_->setItem(r, 2, burstItem);
      processesTable_->setItem(r, 3, colorItem);
    }
  });

  connect(algorithmCombo_, &QComboBox::currentTextChanged, this,
          [this](const QString& text) {
    const bool isRR = (text == "RR");
    quantumSpin_->setEnabled(isRR);
  });

  connect(runBtn_, &QPushButton::clicked, this, [this]() { runSimulation(); });
}

void MainWindow::runSimulation() {
  if (processes_.empty()) {
    QMessageBox::warning(this, "No processes", "Add at least one process before running.");
    return;
  }

  const QString algo = algorithmCombo_->currentText();

  std::vector<ProcessInput> inputs;
  inputs.reserve(processes_.size());
  std::unordered_map<int, QColor> pidToColor;
  pidToColor.reserve(processes_.size());

  for (const auto& gp : processes_) {
    inputs.push_back(ProcessInput{gp.process_id, gp.arrival_time, gp.burst_time});
    pidToColor[gp.process_id] = gp.color;
  }

  const std::string algoStd = algo.toStdString();
  long long quantum = quantumSpin_->value();

  std::vector<ProcessSlice> timeline;
  if (algoStd == "FCFS") {
    timeline = firstComeFirstServe(inputs);
  } else if (algoStd == "RR") {
    timeline = roundRobin(inputs, quantum);
  } else if (algoStd == "SJF") {
    timeline = shortestJobFirst(inputs);
  } else if (algoStd == "SRTF") {
    timeline = shortestRemainingTimeFirst(inputs);
  } else {
    QMessageBox::critical(this, "Unknown algorithm", "Unsupported algorithm selection.");
    return;
  }

  const SummaryMetrics summary = computeSummaryMetrics(algoStd, inputs, timeline);
  auto perProc = computePerProcessMetrics(algoStd, inputs, timeline);

  gantt_->setTimeline(timeline, pidToColor);
  updateMetricsTable(perProc);

  avgWaitingLabel_->setText(QString::number(static_cast<double>(summary.avg_waiting_time), 'f', 2));
  avgTurnaroundLabel_->setText(QString::number(static_cast<double>(summary.avg_turnaround_time), 'f', 2));
  throughputLabel_->setText(QString::number(static_cast<double>(summary.throughput), 'f', 2));
  cpuUtilLabel_->setText(QString::number(static_cast<double>(summary.cpu_utilization_percent), 'f', 2) + "%");
}

void MainWindow::updateMetricsTable(
    const std::unordered_map<int, PerProcessMetrics>& perProc) {
  metricsTable_->setRowCount(0);

  metricsTable_->setUpdatesEnabled(false);
  for (size_t i = 0; i < processes_.size(); i++) {
    const auto& gp = processes_[i];
    metricsTable_->insertRow(static_cast<int>(i));

    long double waiting = 0;
    long double turnaround = 0;
    auto it = perProc.find(gp.process_id);
    if (it != perProc.end()) {
      waiting = it->second.waiting_time;
      turnaround = it->second.turnaround_time;
    }

    auto* pidItem = new QTableWidgetItem(QString::number(gp.process_id));
    auto* arrItem = new QTableWidgetItem(QString::number(gp.arrival_time));
    auto* burstItem = new QTableWidgetItem(QString::number(gp.burst_time));
    auto* waitItem = new QTableWidgetItem(QString::number(static_cast<double>(waiting), 'f', 2));
    auto* tatItem = new QTableWidgetItem(QString::number(static_cast<double>(turnaround), 'f', 2));

    metricsTable_->setItem(static_cast<int>(i), 0, pidItem);
    metricsTable_->setItem(static_cast<int>(i), 1, arrItem);
    metricsTable_->setItem(static_cast<int>(i), 2, burstItem);
    metricsTable_->setItem(static_cast<int>(i), 3, waitItem);
    metricsTable_->setItem(static_cast<int>(i), 4, tatItem);
  }
  metricsTable_->setUpdatesEnabled(true);
}

