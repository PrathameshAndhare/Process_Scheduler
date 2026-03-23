#include "metrics.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
long double round2(long double x) {
  return std::round(x * 100.0L) / 100.0L;
}
} // namespace

std::unordered_map<int, PerProcessMetrics> computePerProcessMetrics(
    const std::string& algorithm,
    const std::vector<ProcessInput>& originalProcesses,
    const std::vector<ProcessSlice>& scheduledTimeline) {
  std::unordered_map<int, PerProcessMetrics> out;
  out.reserve(originalProcesses.size());

  // TS SummaryTable:
  // - FCFS uses a different formula on original arrival order
  // - Other algos compute waiting/turnaround from scheduled intervals
  if (algorithm == "FCFS") {
    std::vector<ProcessInput> sorted = originalProcesses;
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const ProcessInput& a, const ProcessInput& b) {
                       return a.arrival_time < b.arrival_time;
                     });

    long long cumulativeTime = 0;
    for (const auto& p : sorted) {
      if (p.arrival_time == -1) continue; // idle sentinel (shouldn't appear in input)

      if (p.arrival_time > cumulativeTime) {
        cumulativeTime = p.arrival_time;
      }

      const long double waitingTime =
          std::max<long double>(0.0L, static_cast<long double>(cumulativeTime - p.arrival_time));
      const long double turnaroundTime = waitingTime + p.burst_time;

      out[p.process_id] = PerProcessMetrics{waitingTime, turnaroundTime};
      cumulativeTime += p.burst_time;
    }

    return out;
  }

  // For RR/SJF/SRTF: intervals-based computation.
  // TS logic (paraphrased):
  //  processStartTime = process.arrival_time
  //  intervals = scheduledProcesses.filter(pid)
  //  waiting += (nextInterval.arrival_time - processStartTime) if processStartTime < nextInterval.arrival_time
  //  processStartTime = interval.arrival_time + interval.burst_time
  //  turnaround = waiting + sum(interval.burst_time)
  for (const auto& p : originalProcesses) {
    long long processStartTime = p.arrival_time;
    long double waitingTime = 0;
    long double burstSum = 0;

    for (const auto& interval : scheduledTimeline) {
      if (interval.process_id != p.process_id) continue;

      burstSum += interval.burst_time;
      if (processStartTime < interval.arrival_time) {
        waitingTime += (interval.arrival_time - processStartTime);
      }
      processStartTime = interval.arrival_time + interval.burst_time;
    }

    const long double turnaroundTime = waitingTime + burstSum;
    out[p.process_id] = PerProcessMetrics{waitingTime, turnaroundTime};
  }

  return out;
}

SummaryMetrics computeSummaryMetrics(
    const std::string& algorithm,
    const std::vector<ProcessInput>& originalProcesses,
    const std::vector<ProcessSlice>& scheduledTimeline) {
  const int totalProcesses = static_cast<int>(originalProcesses.size());

  std::unordered_map<int, PerProcessMetrics> perProc =
      computePerProcessMetrics(algorithm, originalProcesses, scheduledTimeline);

  long double totalWaitingTime = 0;
  long double totalTurnaroundTime = 0;
  for (const auto& p : originalProcesses) {
    auto it = perProc.find(p.process_id);
    if (it == perProc.end()) continue;
    totalWaitingTime += it->second.waiting_time;
    totalTurnaroundTime += it->second.turnaround_time;
  }

  // TS SummaryTable: totalExecutionTime is derived from startTime/endTime, but
  // algebraically becomes sum(burst_time over scheduledTimeline).
  long long totalExecutionTime = 0;
  long double totalBurstTime = 0; // excludes idle slices (arrival_time == -1)
  long long minArrivalTime = std::numeric_limits<long long>::max();

  for (const auto& s : scheduledTimeline) {
    totalExecutionTime += s.burst_time;
    if (s.arrival_time != -1) {
      totalBurstTime += s.burst_time;
    }
    minArrivalTime = std::min(minArrivalTime, s.arrival_time);
  }

  // TS: throughput = totalProcesses / totalExecutionTime
  const long double throughput =
      (totalExecutionTime > 0)
          ? (static_cast<long double>(totalProcesses) / static_cast<long double>(totalExecutionTime))
          : 0.0L;

  // TS: cpuUtilization = (totalBurstTime / totalExecutionTime) * 100
  const long double cpuPercent =
      (totalExecutionTime > 0)
          ? (static_cast<long double>(totalBurstTime) / static_cast<long double>(totalExecutionTime)) * 100.0L
          : 0.0L;

  const long double avgWaiting =
      (totalProcesses > 0) ? totalWaitingTime / totalProcesses : 0.0L;
  const long double avgTurnaround =
      (totalProcesses > 0) ? totalTurnaroundTime / totalProcesses : 0.0L;

  SummaryMetrics m;
  m.total_waiting_time = totalWaitingTime;
  m.total_turnaround_time = totalTurnaroundTime;
  m.total_execution_time = totalExecutionTime;
  m.avg_waiting_time = round2(avgWaiting);
  m.avg_turnaround_time = round2(avgTurnaround);
  m.throughput = round2(throughput);
  m.cpu_utilization_percent = round2(cpuPercent);
  (void)minArrivalTime; // kept only because TS computed it
  return m;
}

