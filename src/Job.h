#ifndef TSH_JOB_H
#define TSH_JOB_H

#include <sys/types.h>
#include <sys/termios.h>

#include <vector>
#include <string>

class Process;

class Job {
 public:
  bool Completed();
  bool Stopped();

 private:
  std::vector<Process> processList;
  std::string command;
  pid_t processGroupId;
  bool notified;
  struct termios terminalModes;
  int stdin, stdout, stderr;
};

#endif // TSH_JOB_H
