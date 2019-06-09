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
  void InitShell();
  void GetInput();
  void Exit();

 private:
  pid_t processGroupId;
  termios terminalModes;
  int terminal;
  bool isInteractive;
  std::vector<Job> jobList;

  // lauchJob() might change job.processGroupId.
  void LaunchJob(
      Job &job,
      const bool &foreground);
};

#endif
