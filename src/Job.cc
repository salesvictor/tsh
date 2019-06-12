#include "Job.h"

#include <sys/signal.h>
#include <sys/unistd.h>

#include <iostream>

bool Job::IsCompleted() const {
  for (const auto &process : process_list_) {
    if (!process.is_completed) {
      return false;
    }
  }

  return true;
}

bool Job::IsStopped() const {
  for (const auto &process : process_list_) {
    if (!process.is_completed && !process.is_stopped) {
      return false;
    }
  }

  return true;
}

void Job::LaunchProcess(const Process &process, const int &in_file,
                        const int &out_file, const int &err_file,
                        const int &terminal, const bool &is_interactive,
                        const bool &is_foreground) {
  pid_t process_id;

  if (is_interactive) {
    // Put the process into the process group and give the process group
    // the terminal, if appropriate.
    //
    // This has to be done both by the shell and in the individual
    // child processes because of potential race conditions.
    process_id = getpid();
    if (process_group_id_ == 0) {
      process_group_id_ = process_id;
    }
    setpgid(process_id, process_group_id_);
    if (is_foreground) {
      tcsetpgrp(terminal, process_group_id_);
    }

    // Set the handling for job control signals back to the default.
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
  }

  // Set the standard input/output channels of the new process.
  if (in_file != STDIN_FILENO) {
    dup2(in_file, STDIN_FILENO);
    close(in_file);
  }
  if (out_file != STDOUT_FILENO) {
    dup2(out_file, STDOUT_FILENO);
    close(out_file);
  }
  if (err_file != STDERR_FILENO) {
    dup2(err_file, STDERR_FILENO);
    close(err_file);
  }

  // Be aware of memory leaks!
  std::vector<char *> c_argv;
  c_argv.reserve(process.argv.size());
  for (auto &arg : process.argv) {
    c_argv.push_back(const_cast<char *>(arg.c_str()));
  }

  // execvp() requires null-terminated array
  c_argv.push_back(nullptr);
  execvp(c_argv[0], &c_argv[0]);
  perror("execvp");
  exit(1);
}
