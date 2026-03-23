#include "metrics.h"
#include "scheduling_algorithms.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void printHelp(const char* argv0) {
  std::cout
      << "Usage:\n"
      << "  " << argv0 << " --algo {FCFS|RR|SJF|SRTF} [--quantum N] \\\n"
      << "     --process id,arrival,burst [--process id,arrival,burst ...]\n\n"
      << "Examples:\n"
      << "  " << argv0 << " --algo FCFS --process 1,0,5 --process 2,2,3\n"
      << "  " << argv0 << " --algo RR --quantum 2 --process 1,0,5 --process 2,1,4\n"
      << "  " << argv0 << " --algo SRTF --process 1,0,8 --process 2,1,4\n";
}

std::string normalizeAlgo(std::string algo) {
  for (char& c : algo) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  if (algo == "FCFS" || algo == "RR" || algo == "SJF" || algo == "SRTF") return algo;
  return "";
}

ProcessInput parseProcessCsv(const std::string& s) {
  // Expected format: id,arrival,burst
  size_t p1 = s.find(',');
  if (p1 == std::string::npos) throw std::invalid_argument("Bad --process format");
  size_t p2 = s.find(',', p1 + 1);
  if (p2 == std::string::npos) throw std::invalid_argument("Bad --process format");

  ProcessInput p;
  p.process_id = std::stoi(s.substr(0, p1));
  p.arrival_time = std::stoll(s.substr(p1 + 1, p2 - (p1 + 1)));
  p.burst_time = std::stoll(s.substr(p2 + 1));

  if (p.process_id < 0) throw std::invalid_argument("process_id must be >= 0");
  if (p.arrival_time < 0) throw std::invalid_argument("arrival_time must be >= 0");
  if (p.burst_time <= 0) throw std::invalid_argument("burst_time must be > 0");
  return p;
}

std::vector<ProcessSlice> scheduleTimeline(const std::string& algo,
                                           const std::vector<ProcessInput>& processes,
                                           long long quantum) {
  if (algo == "FCFS") return firstComeFirstServe(processes);
  if (algo == "SJF") return shortestJobFirst(processes);
  if (algo == "SRTF") return shortestRemainingTimeFirst(processes);
  if (algo == "RR") return roundRobin(processes, quantum);
  return {};
}

std::string formatTimeline(const std::vector<ProcessSlice>& timeline) {
  // Print in the same "Gantt order" style as the UI: cumulative time on the x-axis.
  std::ostringstream oss;
  long long t = 0;
  for (const auto& s : timeline) {
    const std::string label = (s.process_id == -1) ? "Idle" : ("P" + std::to_string(s.process_id));
    oss << t << " -> " << (t + s.burst_time) << " : " << label << " (" << s.burst_time << ")\n";
    t += s.burst_time;
  }
  return oss.str();
}

} // namespace

int main(int argc, char** argv) {
  if (argc <= 1) {
    printHelp(argv[0]);
    return 1;
  }

  std::string algo;
  long long quantum = 0;
  std::vector<ProcessInput> processes;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "--help" || arg == "-h") {
      printHelp(argv[0]);
      return 0;
    } else if (arg == "--algo" && i + 1 < argc) {
      algo = normalizeAlgo(argv[++i]);
      if (algo.empty()) {
        std::cerr << "Invalid --algo value.\n";
        return 1;
      }
    } else if (arg == "--quantum" && i + 1 < argc) {
      quantum = std::stoll(argv[++i]);
      if (quantum <= 0) {
        std::cerr << "--quantum must be > 0\n";
        return 1;
      }
    } else if (arg == "--process" && i + 1 < argc) {
      try {
        processes.push_back(parseProcessCsv(argv[++i]));
      } catch (const std::exception& e) {
        std::cerr << "Failed to parse --process: " << e.what() << "\n";
        return 1;
      }
    } else {
      std::cerr << "Unknown or incomplete argument: " << arg << "\n";
      printHelp(argv[0]);
      return 1;
    }
  }

  if (algo.empty()) {
    std::cerr << "Missing required --algo.\n";
    printHelp(argv[0]);
    return 1;
  }
  if (processes.empty()) {
    std::cerr << "Missing required --process entries.\n";
    printHelp(argv[0]);
    return 1;
  }
  if (algo == "RR" && quantum <= 0) {
    std::cerr << "--quantum is required for RR.\n";
    return 1;
  }

  std::vector<ProcessSlice> timeline = scheduleTimeline(algo, processes, quantum);
  SummaryMetrics summary = computeSummaryMetrics(algo, processes, timeline);
  auto perProc = computePerProcessMetrics(algo, processes, timeline);

  std::cout << "Algorithm: " << algo << "\n";
  if (algo == "RR") std::cout << "Quantum: " << quantum << "\n";
  std::cout << "Processes: " << processes.size() << "\n\n";

  std::cout << "Timeline (Gantt-style):\n";
  std::cout << formatTimeline(timeline);

  std::cout << "\nSummary (matches web UI math):\n";
  std::cout << "Avg Waiting Time: " << summary.avg_waiting_time << "\n";
  std::cout << "Avg Turnaround Time: " << summary.avg_turnaround_time << "\n";
  std::cout << "Throughput: " << summary.throughput << "\n";
  std::cout << "CPU Utilization: " << summary.cpu_utilization_percent << "%\n";

  std::cout << "\nTotals:\n";
  std::cout << "Total Waiting Time: " << summary.total_waiting_time << "\n";
  std::cout << "Total Turnaround Time: " << summary.total_turnaround_time << "\n";
  std::cout << "Total Execution Time: " << summary.total_execution_time << "\n";

  std::cout << "\nPer-process metrics (like web UI SummaryTable):\n";
  std::cout << "PID\tArrival\tBurst\tWaiting\tTurnaround\n";
  for (const auto& p : processes) {
    auto it = perProc.find(p.process_id);
    const long double waiting = (it != perProc.end()) ? it->second.waiting_time : 0.0L;
    const long double turnaround = (it != perProc.end()) ? it->second.turnaround_time : 0.0L;
    std::cout << p.process_id << "\t" << p.arrival_time << "\t" << p.burst_time << "\t"
              << waiting << "\t" << turnaround << "\n";
  }

  return 0;
}

