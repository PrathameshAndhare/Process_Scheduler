#include "scheduling_algorithms.h"

#include <algorithm>
#include <limits>

namespace {
template <class T, class Cmp>
void stableSort(T& vec, Cmp cmp) {
  std::stable_sort(vec.begin(), vec.end(), cmp);
}

long long clampToNonNegative(long long v) {
  return v < 0 ? 0 : v;
}

ProcessSlice makeIdleSlice(long long duration) {
  return ProcessSlice{ -1, -1, duration };
}
} // namespace

std::vector<ProcessSlice> firstComeFirstServe(
    const std::vector<ProcessInput>& processes) {
  // Sort by arrival time (stable for tie-breaking like JS).
  std::vector<ProcessInput> sorted = processes;
  stableSort(sorted,
             [](const ProcessInput& a, const ProcessInput& b) {
               return a.arrival_time < b.arrival_time;
             });

  std::vector<ProcessSlice> result;
  long long currentTime = 0;

  for (const auto& currentProcess : sorted) {
    if (currentProcess.arrival_time > currentTime) {
      const long long gapDuration = currentProcess.arrival_time - currentTime;
      result.push_back(makeIdleSlice(gapDuration));
      currentTime = currentProcess.arrival_time;
    }

    // In the TS implementation, FCFS timeline slices keep the process'
    // original arrival_time. (The UI uses arrival_time only to detect idle.)
    result.push_back(ProcessSlice{currentProcess.process_id,
                                    currentProcess.arrival_time,
                                    currentProcess.burst_time});
    currentTime += currentProcess.burst_time;
  }

  return result;
}

std::vector<ProcessSlice> roundRobin(const std::vector<ProcessInput>& processes,
                                      long long quantum) {
  std::vector<ProcessInput> sorted = processes;
  stableSort(sorted,
             [](const ProcessInput& a, const ProcessInput& b) {
               return a.arrival_time < b.arrival_time;
             });

  std::vector<ProcessSlice> result;
  struct QueueItem {
    ProcessInput process;
    long long remaining_time;
  };
  std::vector<QueueItem> queue;

  long long currentTime = 0;
  size_t index = 0;

  while (!queue.empty() || index < sorted.size()) {
    // Enqueue newly arrived processes
    while (index < sorted.size() && sorted[index].arrival_time <= currentTime) {
      queue.push_back(QueueItem{sorted[index], sorted[index].burst_time});
      index++;
    }

    if (queue.empty()) {
      // Idle time until the next process arrives
      const auto& nextProcess = sorted[index];
      const long long gapDuration = nextProcess.arrival_time - currentTime;
      result.push_back(makeIdleSlice(gapDuration));
      currentTime += gapDuration;
      continue;
    }

    // Dequeue a process and execute it for the quantum or until it finishes
    QueueItem item = queue.front();
    queue.erase(queue.begin());

    const long long executionTime =
        std::min(item.remaining_time, quantum);

    // Add the process slice to the result
    result.push_back(ProcessSlice{item.process.process_id,
                                    currentTime,
                                    executionTime});
    currentTime += executionTime;

    // Re-check for newly arrived processes after execution
    while (index < sorted.size() && sorted[index].arrival_time <= currentTime) {
      queue.push_back(QueueItem{sorted[index], sorted[index].burst_time});
      index++;
    }

    // If the process has remaining time, requeue it; otherwise, it completes.
    // (TS checks `remaining_time > quantum` rather than `> executionTime`.)
    if (item.remaining_time > quantum) {
      queue.push_back(
          QueueItem{item.process, item.remaining_time - quantum});
    }
  }

  // Merge consecutive executions of the same process for clarity
  std::vector<ProcessSlice> mergedResult;
  for (const auto& s : result) {
    if (!mergedResult.empty() && mergedResult.back().process_id == s.process_id) {
      mergedResult.back().burst_time += s.burst_time;
    } else {
      mergedResult.push_back(s);
    }
  }

  return mergedResult;
}

std::vector<ProcessSlice> shortestJobFirst(
    const std::vector<ProcessInput>& processes) {
  std::vector<ProcessInput> sorted = processes;
  stableSort(sorted,
             [](const ProcessInput& a, const ProcessInput& b) {
               return a.arrival_time < b.arrival_time;
             });

  std::vector<ProcessSlice> result;
  std::vector<ProcessInput> availableProcesses;

  long long currentTime = 0;
  size_t index = 0;

  while (index < sorted.size() || !availableProcesses.empty()) {
    while (index < sorted.size() && sorted[index].arrival_time <= currentTime) {
      availableProcesses.push_back(sorted[index]);
      index++;
    }

    if (!availableProcesses.empty()) {
      stableSort(availableProcesses,
                 [](const ProcessInput& a, const ProcessInput& b) {
                   return a.burst_time < b.burst_time;
                 });
      ProcessInput nextProcess = availableProcesses.front();
      availableProcesses.erase(availableProcesses.begin());

      // Start time for this slice is `currentTime`.
      result.push_back(ProcessSlice{nextProcess.process_id,
                                      currentTime,
                                      nextProcess.burst_time});
      currentTime += nextProcess.burst_time;
    } else {
      const auto& nextProcess = sorted[index];
      const long long gapDuration = nextProcess.arrival_time - currentTime;
      result.push_back(makeIdleSlice(gapDuration));
      currentTime += gapDuration;
    }
  }

  // Merge consecutive slices of the same process_id (matches TS behavior)
  std::vector<ProcessSlice> mergedResult;
  for (const auto& s : result) {
    if (!mergedResult.empty() && mergedResult.back().process_id == s.process_id) {
      mergedResult.back().burst_time += s.burst_time;
    } else {
      mergedResult.push_back(s);
    }
  }

  return mergedResult;
}

std::vector<ProcessSlice> shortestRemainingTimeFirst(
    const std::vector<ProcessInput>& processes) {
  std::vector<ProcessInput> sorted = processes;
  stableSort(sorted,
             [](const ProcessInput& a, const ProcessInput& b) {
               return a.arrival_time < b.arrival_time;
             });

  std::vector<ProcessSlice> result;
  struct QueueItem {
    ProcessInput process;
    long long remaining_time;
  };
  std::vector<QueueItem> queue;

  long long currentTime = 0;
  size_t index = 0;

  while (!queue.empty() || index < sorted.size()) {
    while (index < sorted.size() && sorted[index].arrival_time <= currentTime) {
      queue.push_back(QueueItem{sorted[index], sorted[index].burst_time});
      index++;
    }

    stableSort(queue,
               [](const QueueItem& a, const QueueItem& b) {
                 return a.remaining_time < b.remaining_time;
               });

    if (queue.empty()) {
      const auto& nextProcess = sorted[index];
      const long long gapDuration = nextProcess.arrival_time - currentTime;
      result.push_back(makeIdleSlice(gapDuration));
      currentTime += gapDuration;
      continue;
    }

    QueueItem item = queue.front();
    queue.erase(queue.begin());

    const long long executionTime = 1; // preemption granularity
    result.push_back(ProcessSlice{item.process.process_id,
                                    currentTime,
                                    executionTime});
    currentTime += executionTime;

    if (item.remaining_time > executionTime) {
      queue.push_back(
          QueueItem{item.process, item.remaining_time - executionTime});
    }
  }

  // Merge consecutive processes with the same process_id
  std::vector<ProcessSlice> mergedResult;
  for (const auto& s : result) {
    if (!mergedResult.empty() && mergedResult.back().process_id == s.process_id) {
      mergedResult.back().burst_time += s.burst_time;
    } else {
      mergedResult.push_back(s);
    }
  }

  return mergedResult;
}

