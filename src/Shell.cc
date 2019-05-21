#include "Shell.h"

#include <sys/signal.h>

#include <iostream>

void Shell::initShell() {
  std::cout << "PID: " << getpid() << "\nPGID: " << getpgrp() << std::endl;

  // See if we are running interactively.
  terminal = STDIN_FILENO;
  interactive = isatty(terminal);

  if (interactive) {
    std::cout << "Debug!@#!@" << std::endl;

    // Loop until we are in the foreground.
    while (tcgetpgrp(terminal) != (processGroupId = getpgrp())) {
      std::cout << "aoskdjasok" << std::endl;
      kill(-processGroupId, SIGTTIN);
    }

    // Ignore interactive and job-control signals.
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    // Put ourselves in our own process group.
    processGroupId = getpid ();
    if (setpgid(processGroupId, processGroupId) < 0) {
      perror("Couldn't put the shell in its own process group");
      exit(1);
    }

    // Grab control of the terminal.
    tcsetpgrp(terminal, processGroupId);

    // Save default terminal attributes for shell.
    tcgetattr(terminal, &terminalModes);
  }

  std::cout << "PID: " << getpid() << "\nPGID: " << processGroupId << std::endl;
}
