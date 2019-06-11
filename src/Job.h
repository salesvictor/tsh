#ifndef TSH_JOB_H
#define TSH_JOB_H

#include <sys/termios.h>
#include <sys/types.h>

#include <string>
#include <vector>

#include "Process.h"

class Job {
 public:
  int stdin_;
  int stdout_;
  int stderr_;
  pid_t process_group_id_;
  bool is_notified_;  
  struct termios terminal_modes_;
  std::string command_;
  std::vector<Process> process_list_;

  Job(int in_file, int out_file, int err_file, pid_t process_group_id,
      struct termios terminal_modes, std::string command,
      std::vector<Process> process_list)
      : stdin_{in_file},
        stdout_{out_file},
        stderr_{err_file},
        process_group_id_{process_group_id},
        terminal_modes_{terminal_modes},
        command_{command},
        process_list_{process_list} {}

  bool IsCompleted() const;
  bool IsStopped() const;
  void LaunchProcess(const Process &process, const int &inFile,
                     const int &outFile, const int &errFile,
                     const int &terminal, const bool &isInteractive,
                     const bool &isForeground);
};

#endif  // TSH_JOB_H
