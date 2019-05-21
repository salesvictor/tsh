#ifndef TSH_SHELL_H
#define TSH_SHELL_H

#include "Job.h"
#include "Process.h"
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <vector.h>


class Shell {
public:
  void init_shell();

private:
  pid_t shell_pgid;
  struct termios shell_tmodes;
  int shell_terminal;
  int shell_is_interactive;
  
  std::vector<Job> jobList;
};

#endif