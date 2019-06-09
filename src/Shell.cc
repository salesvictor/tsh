#include "Shell.h"

#include <sys/signal.h>

#include <string>
#include <iostream>
#include <sstream>

// TODO(Victor): forward declarations to shut compiler, implement them!
// Comment the following lines to run the shell.
//extern void format_job_info(const Job&, const std::string&);
//extern void wait_for_job(const Job&);
//extern void put_job_in_foreground(const Job&, const bool &sendContinueSignal);
//extern void put_job_in_background(const Job&, const bool &sendContinueSignal);

void Shell::InitShell() {
  // See if we are running interactively.
  terminal = STDIN_FILENO;
  isInteractive = isatty(terminal);

  std::cout << "PID: " << getpid() 
            << "\nPGID: " << getpgrp() 
            << "\nTerminal Foreground PGID: "
            << tcgetpgrp(terminal) << std::endl;

  if (isInteractive) {
    std::cout << "Shell is interactive!" << std::endl;

    // Loop until we are in the foreground.
    while (tcgetpgrp(terminal) != (processGroupId = getpgrp())) {
      std::cout << "Shell not in foreground, interrupting" << std::endl;
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
    processGroupId = getpid();
    if (setpgid(processGroupId, processGroupId) < 0) {
      perror("Couldn't put the shell in its own process group");
      exit(1);
    }

    // Grab control of the terminal.
    tcsetpgrp(terminal, processGroupId);

    // Save default terminal attributes for shell.
    tcgetattr(terminal, &terminalModes);
  }

  std::cout << "PID: " << getpid() 
            << "\nPGID: " << processGroupId
            << "\nTerminal Foreground PGID: "
            << tcgetpgrp(terminal) << std::endl;
  std::cout << "\n\nFinished shell initialisation!\n"
            << "Type 'exit' without the ticks to get out!\n" << std::endl;
}

void Shell::GetInput() {
  std::string line;
  while (true) {
    std::cout << "#: ";
    std::getline(std::cin, line);
    std::istringstream lineStream(line);
    if (line != "exit") {
      std::cout << "Job: " << line << std::endl;
      std::string token;
      std::vector<Process> processList;
      std::vector<std::string> argv;
      while (lineStream >> token) {
        if (token != "|") {
          argv.push_back(token);
        } else {
          processList.push_back({argv});
          argv.clear();
        }
      }
      processList.push_back({argv});
      for (const auto &process : processList) {
        for (size_t i=0;i<process.argv.size();++i) {
          if (i==0) {
            std::cout << "\tProcess: ";
          } else if (i==1) {
            std::cout << "\t\tArgs: ";
          }
          std::cout << process.argv[i] << " ";
          if (i==0) {
            std::cout << std::endl;
          }
        }
        std::cout << std::endl;
      }
      std::cout << std::endl;

      // TODO(Victor): launch job sucefully.
    } else {
      break;
    }
  }
}

void Shell::LaunchJob(
    Job &job,
    const bool &isForeground) {
  Process *process;
  pid_t processId;
  int processPipe[2];
  int inFile;
  int outFile;

  inFile = job.stdin;
  for (Process &process : job.processList) {
    // Set up pipes, if necessary.
    if (&process != &job.processList.back()) {
      if (pipe(processPipe) < 0) {
        perror("pipe");
        exit(1);
      }
      outFile = processPipe[1];
    } else {
      outFile = job.stdout;
    }

    // Fork the child processes.
    processId = fork();
    if (processId == 0) {
      // This is the child process.
      job.LaunchProcess(
          process,
          inFile,
          outFile,
          job.stderr,
          terminal,
          isInteractive,
          isForeground);
    } else if (processId < 0) {
      // The fork failed.
      perror("fork");
      exit(1);
    } else {
      // This is the parent process.
      process.processId = processId;
      if (isInteractive) {
        if (!job.processGroupId) {
          job.processGroupId = processId;
        }
        setpgid(processId, job.processGroupId);
      }
    }

    // Clean up after pipes.
    if (inFile != job.stdin) {
      close(inFile);
    }
    if (outFile != job.stdout) {
      close(outFile);
    }
    inFile = processPipe[0];
  }

  // TODO(Victor): Make these snake_case functions in Shell.
  // Info: Stopped-and-Terminated-Jobs.html#Stopped-and-Terminated-Jobs
  // Root: https://www.gnu.org/software/libc/manual/html_node/
  // Comment the following lines to run the shell.
  //format_job_info(job, "launched");

  //if (!isInteractive) {
  //  wait_for_job(job);
  //} else if (isForeground) {
  //  put_job_in_foreground(job, 0);
  //} else {
  //  put_job_in_background(job, 0);
  //}
}

void Shell::Exit() {
  std::cout << "\nCya! :)\n\n";
}
