#ifndef TSH_JOB_H
#define TSH_JOB_H

#include <sys/types.h>
#include <sys/termios.h>

#include <vector>
#include <string>

#include "Process.h"

class Job {
 public:
  int stdin;
  int stdout;
  int stderr;
  pid_t processGroupId;
  std::vector<Process> processList;

  bool Completed();
  bool Stopped();
  void LaunchProcess(
      const Process &process,
      const int &inFile,
      const int &outFile,
      const int &errFile,
      const int &terminal,
      const bool &isInteractive,
      const bool &isForeground);

 private:
  std::string command;
  bool notified;
  struct termios terminalModes;
};

#endif // TSH_JOB_H
