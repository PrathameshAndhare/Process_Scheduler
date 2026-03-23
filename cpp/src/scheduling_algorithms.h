#pragma once

#include <vector>

struct ProcessInput {
  int process_id;
  long long arrival_time;
  long long burst_time;
};

// A single (possibly partial) execution slice in the resulting Gantt timeline.
// For idle slices: process_id == -1 and arrival_time == -1.
struct ProcessSlice {
  int process_id;
  long long arrival_time; // start time of this slice (idle uses -1)
  long long burst_time;   // length of this slice
};

std::vector<ProcessSlice> firstComeFirstServe(
    const std::vector<ProcessInput>& processes);

std::vector<ProcessSlice> roundRobin(const std::vector<ProcessInput>& processes,
                                      long long quantum);

std::vector<ProcessSlice> shortestJobFirst(
    const std::vector<ProcessInput>& processes);

std::vector<ProcessSlice> shortestRemainingTimeFirst(
    const std::vector<ProcessInput>& processes);

