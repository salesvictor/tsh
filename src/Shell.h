#ifndef TSH_SHELL_H
#define TSH_SHELL_H

#include <sys/termios.h>
#include <sys/types.h>
#include <sys/unistd.h>

#include <iostream>
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
  bool is_reading_;
  pid_t process_group_id_;
  termios terminal_modes_;
  std::vector<Job> job_list_;

  inline void PrintPrompt() {
    static char working_dir[100];
    static char hostname[100];
    static std::string username;

    getcwd(working_dir, 100);
    gethostname(hostname, 100);
    username = getlogin();
    std::cout << "tsh-0.1.0 [" << username << "@" << hostname << ": "
              << working_dir << "] $ ";
  }

  // LauchJob() might change job.processGroupId.
  void LaunchJob(Job &job, const bool &is_foreground);

  // Parse shell input.
  // Return job information about list of processes and foreground/background.
  std::pair<std::vector<Process>, bool> ParseCommand(std::string command);

  // If sendSigCont is nonzero, restore the saved terminal modes and send the
  // process group a SIGCONT signal to wake it up before we block.
  //
  // job.terminal_modes_ may change.
  void PutJobInForeground(Job &job, const bool &send_sig_cont);

  // If the cont argument is true, send the process group a SIGCONT signal
  // to wake it up.
  void PutJobInBackground(const Job &job, const bool &send_sig_cont);

  // Store the status of the process pid that was returned by waitpid.
  // Return 0 if all went well, nonzero otherwise.
  int MarkProcessStatus(const pid_t &process_id, const int &status);

  // Check for processes that have status information available, without
  // blocking
  void UpdateStatus();

  // Check for processes that have status information available,
  // blocking until all processes in the given job have reported.
  void WaitForJob(const Job &job);

  // Format information about job status for the user to look at.
  void FormatJobInfo(const Job &job, const std::string &status);

  // Mark a stopped job as being running again.
  void MarkJobAsRunning(Job &job);

  // Continue a stopped job.
  void ContinueJob(Job &job, int foreground);

  // Notify the user about stopped or terminated jobs.
  // Delete terminated jobs from the active job list.
  void DoJobNotification();
};

#endif  // TSH_SHELL_H
