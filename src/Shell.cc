#include "Shell.h"

#include <sys/signal.h>
#include <sys/unistd.h>
#include <sys/wait.h>

#include <iostream>
#include <regex>
#include <sstream>
#include <string>

void Shell::InitShell() {
  // See if we are running interactively.
  terminal_ = STDIN_FILENO;
  is_interactive_ = isatty(terminal_);

  if (is_interactive_) {
    std::cout << "Shell is interactive!" << std::endl;

    // Loop until we are in the foreground.
    while (tcgetpgrp(terminal_) != (process_group_id_ = getpgrp())) {
      std::cout << "Shell not in foreground, interrupting" << std::endl;
      kill(-process_group_id_, SIGTTIN);
    }

    // Ignore interactive and job-control signals.
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    // signal(SIGCHLD, SIG_IGN);

    // Put ourselves in our own process group.
    process_group_id_ = getpid();
    if (setpgid(process_group_id_, process_group_id_) < 0) {
      perror("Couldn't put the shell in its own process group");
      exit(1);
    }

    // Grab control of the terminal.
    tcsetpgrp(terminal_, process_group_id_);

    // Save default terminal attributes for shell.
    tcgetattr(terminal_, &terminal_modes_);
  }

  std::cout << "\n\nFinished shell initialisation!\n"
            << "Type 'exit' without the ticks to get out!\n"
            << std::endl;
}

void Shell::GetInput() {
  std::string command;
  while (true) {
    // Standard parameters.
    // TODO(Victor): deduce them from command.
    int in_file = STDIN_FILENO;
    int out_file = STDOUT_FILENO;
    int err_file = STDERR_FILENO;
    bool is_foreground = true;
    pid_t process_group_id = 0;
    struct termios terminal_modes = {};

    std::cout << "tsh-0.1.0$ ";
    std::getline(std::cin, command);
    if (command == "exit" || std::cin.eof()) {
      break;
    } else if (command == "jobs") {
      int i = 0;
      for (auto &job : job_list_) {
        std::cout << "[" << i << "]  ";
        if (job.IsCompleted()) {
          std::cout << "Completed";
        } else if (job.IsStopped()) {
          std::cout << "Stopped  ";
        } else {
          std::cout << "Executing";
        }
        std::cout << "     " << job.command_ << std::endl;
        ++i;
      }
    } else {
      std::string token;
      auto parse_information = ParseCommand(command);
      std::vector<Process> process_list = parse_information.first;
      bool is_foreground = parse_information.second;

      Job job{in_file,        out_file, err_file,    process_group_id,
              terminal_modes, command,  process_list};
      job_list_.push_back(job);
      LaunchJob(job_list_.back(), is_foreground);
    }

    // Perform jobs status updates and notifications
    DoJobNotification();
  }
}

std::pair<std::vector<Process>, bool> Shell::ParseCommand(std::string command) {
  bool is_foreground = true;

  // First regex explanation:
  //   First part `[^("|')]\S*` matches any non whitespace and non quotes;
  //   Second part `("|').+?\2*` matches quoted arguments;
  //
  // Second regex explanation:
  //   Matches anything with pipes, with the first group ending at the first
  //   pipe.
  std::regex process_regex{R"(\s*([^"']\S*|("|').+?\2)\s*)"};
  std::regex pipe_regex{R"((?:\s)*(.*?)\|(.*))"};
  std::smatch process_match;
  std::smatch pipe_match;
  std::vector<Process> process_list;
  while (std::regex_match(command, pipe_match, pipe_regex)) {
    std::string process_command = pipe_match[1].str();

    Process process{};
    while (std::regex_search(process_command, process_match, process_regex)) {
      process.argv.push_back(process_match[1]);
      process_command = process_match.suffix().str();
    }
    process_list.push_back(process);

    process_command = process_match.suffix().str();
    command = pipe_match[2].str();
  }

  // The tailling command is processed separately and may indicate background
  // job.
  if (!command.empty()) {
    std::string process_command{command};
    Process process{};
    while (std::regex_search(process_command, process_match, process_regex)) {
      if (process_match[1] == "&") {
        is_foreground = false;
      } else {
        process.argv.push_back(process_match[1]);
      }
      process_command = process_match.suffix().str();
    }
    process_list.push_back(process);

    process_command = process_match.suffix().str();
  }
  return {process_list, is_foreground};
}

void Shell::LaunchJob(Job &job, const bool &is_foreground) {
  int in_file{job.stdin_};
  int out_file;
  int process_pipe[2]{};
  pid_t process_id{0};

  for (auto &process : job.process_list_) {
    // Check if we aren't on the last process of the chain
    if (&process != &job.process_list_.back()) {
      // Set up pipes, if necessary.
      if (pipe(process_pipe) < 0) {
        perror("pipe");
        exit(1);
      }
      out_file = process_pipe[1];
    } else {
      out_file = job.stdout_;
    }

    // Fork the child processes.
    process_id = fork();
    if (process_id == 0) {
      job.LaunchProcess(process, in_file, out_file, job.stderr_, terminal_,
                        is_interactive_, is_foreground);
    } else if (process_id < 0) {
      // The fork failed.
      perror("fork");
      exit(1);
    } else {
      // This is the parent process.
      process.process_id = process_id;
      if (is_interactive_) {
        if (!job.process_group_id_) {
          job.process_group_id_ = process_id;
        }
        setpgid(process_id, job.process_group_id_);
      }
    }

    // Clean up after pipes.
    if (in_file != job.stdin_) {
      close(in_file);
    }
    if (out_file != job.stdout_) {
      close(out_file);
    }
    in_file = process_pipe[0];
  }

  FormatJobInfo(job, "launched");

  if (!is_interactive_) {
    std::cout << "Wait for job" << std::endl;
    WaitForJob(job);
  } else if (is_foreground) {
    std::cout << "Putting job in foreground" << std::endl;
    PutJobInForeground(job, false);
  } else {
    std::cout << "Putting job in background" << std::endl;
    PutJobInBackground(job, false);
  }
  std::cout << std::endl;
}

void Shell::PutJobInForeground(Job &job, const bool &send_sig_cont) {
  // Put the job into the foreground.
  tcsetpgrp(terminal_, job.process_group_id_);

  // Send the job a continue signal, if necessary.
  if (send_sig_cont) {
    tcsetattr(terminal_, TCSADRAIN, &job.terminal_modes_);
    if (kill(-job.process_group_id_, SIGCONT) < 0) {
      std::perror("kill (SIGCONT)");
    }
  }

  // Wait for it to report.
  WaitForJob(job);

  // Put the shell back in the foreground.
  tcsetpgrp(terminal_, process_group_id_);

  // Restore the shell’s terminal modes.
  tcgetattr(terminal_, &job.terminal_modes_);
  tcsetattr(terminal_, TCSADRAIN, &terminal_modes_);
}

void Shell::PutJobInBackground(const Job &job, const bool &send_sig_cont) {
  // Send the job a continue signal, if necessary.
  if (send_sig_cont) {
    if (kill(-job.process_group_id_, SIGCONT) < 0) {
      std::perror("kill (SIGCONT)");
    }
  }
}

int Shell::MarkProcessStatus(const pid_t &process_id, const int &status) {
  if (process_id > 0) {
    // Update the record for the process.
    for (auto &job : job_list_) {
      for (auto &process : job.process_list_) {
        if (process.process_id == process_id) {
          process.status = status;
          if (WIFSTOPPED(status)) {
            process.is_stopped = true;
          } else {
            process.is_completed = true;
            if (WIFSIGNALED(status)) {
              std::cerr << std::endl
                        << process_id << ": Terminated by signal"
                        << WTERMSIG(process.status) << "." << std::endl;
            }
          }
          return 0;
        }
      }
    }
    std::cerr << std::endl
              << "No child process " << process_id << "." << std::endl;
    return -1;
  } else if (process_id == 0 || errno == ECHILD) {
    // No processes ready to report.
    return -1;
  } else {
    // Other weird errors.
    std::perror("waitpid");
    return -1;
  }
}

void Shell::UpdateStatus() {
  int status;
  pid_t process_id;

  do {
    process_id = waitpid(WAIT_ANY, &status, WUNTRACED | WNOHANG);
  } while (!MarkProcessStatus(process_id, status));
}

void Shell::WaitForJob(const Job &job) {
  int status;
  pid_t process_id;

  do {
    process_id = waitpid(WAIT_ANY, &status, WUNTRACED);
  } while (!MarkProcessStatus(process_id, status) && !job.IsStopped() &&
           !job.IsCompleted());
}

void Shell::FormatJobInfo(const Job &job, const std::string &status) {
  std::cerr << job.process_group_id_ << " (" << status << "): " << job.command_
            << std::endl;
}

void Shell::DoJobNotification() {
  // Update status information for child processes.
  UpdateStatus();

  auto iter = std::begin(job_list_);
  for (auto &job : job_list_) {
    ++iter;

    // If all processes have completed, tell the user the job has
    // completed and delete it from the list of active jobs.
    if (job.IsCompleted()) {
      FormatJobInfo(job, "completed");
      job_list_.erase(iter);
    }

    // Notify the user about stopped jobs,
    // marking them so that we won’t do this more than once.
    else if (job.IsStopped() && !job.is_notified_) {
      FormatJobInfo(job, "stopped");
      job.is_notified_ = true;
    }

    // Don’t say anything about jobs that are still running.
  }
}

void Shell::MarkJobAsRunning(Job &job) {
  for (auto &process : job.process_list_) {
    process.is_stopped = false;
  }
  job.is_notified_ = false;
}

void Shell::ContinueJob(Job &job, int foreground) {
  MarkJobAsRunning(job);
  if (foreground) {
    PutJobInForeground(job, 1);
  } else {
    PutJobInBackground(job, 1);
  }
}

void Shell::Exit() { std::cout << "\nCya! :)\n\n"; }
