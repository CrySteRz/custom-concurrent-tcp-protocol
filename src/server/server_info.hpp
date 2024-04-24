#pragma once

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/sysinfo.h>

struct MemoryInfo {
  double total_memory;
  double free_memory;
};

class ServerInfo {
public:
  // TODO: Vezi de ce nu merge bine
  static MemoryInfo get_memory_usage() {
    struct sysinfo memory_info;
    sysinfo(&memory_info);

    long long total_physical_memory = memory_info.totalram;
    total_physical_memory *= memory_info.mem_unit;

    long long free_physical_memory = memory_info.freeram;

    free_physical_memory *= memory_info.mem_unit;

    MemoryInfo m;
    m.total_memory =
        static_cast<double>(total_physical_memory) / (1024 * 1024 * 1024);
    m.free_memory =
        static_cast<double>(free_physical_memory) / (1024 * 1024 * 1024);

    return m;
  }

  static int8_t get_cpu_usage_percentage() {
    std::ifstream fileStat("/proc/stat");
    std::string line;
    std::getline(fileStat, line);
    std::istringstream iss(line);
    std::string cpu;
    long long user, nice, system, idle, iowait, irq, softirq, steal, guest,
        guest_nice;
    if (iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >>
        softirq >> steal >> guest >> guest_nice) {
      long long totalIdle = idle + iowait;
      long long totalActive = user + nice + system + irq + softirq + steal;
      long long total = totalActive + totalIdle;
      double percentage = static_cast<double>(totalActive) / total * 100.0;
      return percentage;
    } else {
      return -1;
    }
  }

  static long get_system_uptime_seconds() {
    struct sysinfo info;
    sysinfo(&info);
    return info.uptime;
  }
};
