#ifndef TSH_SHELL_H
#define TSH_SHELL_H

#include <sys/types.h>
#include <sys/termios.h>
#include <sys/unistd.h>

#include <vector>

#include "Job.h"
#include "Process.h"

class Shell {
 public:
  void initShell();

 private:
  pid_t processGroupId;
  termios terminalModes;
  int terminal;
  bool interactive;
  
  std::vector<Job> jobList;
};

#endif
