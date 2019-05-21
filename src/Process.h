#ifndef TSH_PROCESS_H
#define TSH_PROCESS_H

struct Process {
  char **argv; // for exec
  pid_t processId; // process ID
  bool completed; // true if process has completed
  bool stopped; // true if process has stopped
  int status; // reported status value
};

#endif // TSH_PROCESS_H
