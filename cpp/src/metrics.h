#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "scheduling_algorithms.h"

struct PerProcessMetrics {
  long double waiting_time = 0;
  long double turnaround_time = 0;
};

struct SummaryMetrics {
  long double avg_waiting_time = 0;
  long double avg_turnaround_time = 0;
  long double throughput = 0;
  long double cpu_utilization_percent = 0;

  long double total_waiting_time = 0;
  long double total_turnaround_time = 0;
  long long total_execution_time = 0;
};

// `algorithm` must be one of: FCFS, RR, SJF, SRTF.
// For RR the quantum affects the scheduled timeline, so metrics are computed
// only from `originalProcesses` + `scheduledTimeline`.
std::unordered_map<int, PerProcessMetrics> computePerProcessMetrics(
    const std::string& algorithm,
    const std::vector<ProcessInput>& originalProcesses,
    const std::vector<ProcessSlice>& scheduledTimeline);

SummaryMetrics computeSummaryMetrics(
    const std::string& algorithm,
    const std::vector<ProcessInput>& originalProcesses,
    const std::vector<ProcessSlice>& scheduledTimeline);

