#ifndef TSH_SHELL_H
#define TSH_SHELL_H

#include <sys/termios.h>
#include <sys/types.h>
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
  int terminal_;
  bool is_interactive_;
  pid_t process_group_id_;
  termios terminal_modes_;
  std::vector<Job> job_list_;

  // LauchJob() might change job.processGroupId.
  void LaunchJob(Job &job, const bool &is_foreground);

  std::pair<std::vector<Process>, bool> ParseCommand(std::string command);

  // Format information about job status for the user to look at.
  void FormatJobInfo(const Job &job, const std::string &status);

  // Check for processes that have status information available,
  // blocking until all processes in the given job have reported.
  void WaitForJob(const Job &job);

  // Store the status of the process pid that was returned by waitpid.
  // Return 0 if all went well, nonzero otherwise.
  int MarkProcessStatus(const pid_t &process_id, const int &status);

  // If sendSigCont is nonzero, restore the saved terminal modes and send the
  // process group a SIGCONT signal to wake it up before we block.
  //
  // job.terminal_modes_ may change.
  void PutJobInForeground(Job &job, const bool &send_sig_cont);

  // If the cont argument is true, send the process group a SIGCONT signal
  // to wake it up.
  void PutJobInBackground(const Job &job, const bool &send_sig_cont);
};

#endif  // TSH_SHELL_H
