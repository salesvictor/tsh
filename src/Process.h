#ifndef TSH_PROCESS_H
#define TSH_PROCESS_H

#include <string>
#include <vector>

struct Process {
  std::vector<std::string> argv;  // for exec
  pid_t process_id;
  bool is_completed;
  bool is_stopped;
  int status;  // reported status value
};

#endif  // TSH_PROCESS_H
