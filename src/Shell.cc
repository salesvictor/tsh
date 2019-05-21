#include "Shell.h"

#include <sys/signal.h>

void Shell::initShell() {
  // See if we are running interactively.
  terminal = STDIN_FILENO;
  interactive = isatty(terminal);

  if (interactive) {
    // Loop until we are in the foreground.
    while (tcgetpgrp(terminal) != (processGroupId = getpgrp())) {
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
}
