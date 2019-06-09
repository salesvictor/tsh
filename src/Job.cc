#include "Job.h"

#include <sys/unistd.h>
#include <sys/signal.h>

bool Job::Completed() {
  for (auto &process : processList) {
    if (!process.completed) {
      return false;
    }
  }

  return true;
}

bool Job::Stopped() {
  for (auto &process : processList) {
    if (!process.completed && !process.stopped) {
      return false;
    }
  }

  return true;
}

void Job::LaunchProcess(
    const Process &process,
    const int &inFile,
    const int &outFile,
    const int &errFile,
    const int &terminal,
    const bool &isInteractive,
    const bool &isForeground) {
  pid_t processId;

  if (isInteractive) {
    // Put the process into the process group and give the process group
    // the terminal, if appropriate.
    //
    // This has to be done both by the shell and in the individual
    // child processes because of potential race conditions.
    processId = getpid();
    if (processGroupId == 0) {
      processGroupId = processId;
    }
    setpgid(processId, processGroupId);
    if (isForeground) {
      tcsetpgrp(terminal, processGroupId);
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
  if (inFile != STDIN_FILENO) {
    dup2(inFile, STDIN_FILENO);
    close(inFile);
  }
  if (outFile != STDOUT_FILENO) {
    dup2(outFile, STDOUT_FILENO);
    close(outFile);
  }
  if (errFile != STDERR_FILENO) {
    dup2(errFile, STDERR_FILENO);
    close(errFile);
  }

  // Execvp() the new process.  Make sure we exit.
  // TODO(Victor): Transform process.argv into a char**.
  // Be aware of memory leaks!
  //execvp(process.argv[0], process.argv);
  execvp(process.argv[0].data(), NULL);
  perror("execvp");
  exit(1);
}

