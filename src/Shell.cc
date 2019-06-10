#include "Shell.h"

#include <sys/signal.h>
#include <sys/unistd.h>
#include <sys/wait.h>

#include <iostream>
#include <regex>
#include <sstream>
#include <string>

// TODO(Victor): forward declarations to shut compiler, implement them!
// Comment the following lines to run the shell.
// extern void format_job_info(const Job&, const std::string&);

void Shell::InitShell() {
  // See if we are running interactively.
  terminal_ = STDIN_FILENO;
  is_interactive_ = isatty(terminal_);

  // std::cout << "PID: " << getpid() << "\nPGID: " << getpgrp()
  //          << "\nTerminal Foreground PGID: " << tcgetpgrp(terminal_)
  //          << std::endl;

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
    signal(SIGCHLD, SIG_IGN);

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

  std::cout << "PID: " << getpid() << "\nPGID: " << process_group_id_
            << "\nTerminal Foreground PGID: " << tcgetpgrp(terminal_)
            << std::endl;
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
    if (command != "exit") {
      // std::cout << "Job: " << command << std::endl;
      std::string token;
      auto [process_list, is_foreground] = ParseCommand(command);
      // for (const auto &process : process_list) {
      //  for (size_t i = 0; i < process.argv.size(); ++i) {
      //    if (i == 0) {
      //      std::cout << "\tProcess: ";
      //    } else if (i == 1) {
      //      std::cout << "\t\tArgs: ";
      //    }
      //    std::cout << process.argv[i] << " ";
      //    if (i == 0) {
      //      std::cout << std::endl;
      //    }
      //  }
      //  std::cout << std::endl;
      // }
      // std::cout << std::endl;

      Job job{in_file,        out_file, err_file,    process_group_id,
              terminal_modes, command,  process_list};
      LaunchJob(job, is_foreground);
    } else {
      break;
    }
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
    // std::cout << "process_command (" << pipe_match.size() << "
    // matches):\n\t";
    std::string process_command = pipe_match[1].str();
    // std::cout << process_command << std::endl;

    Process process{};
    while (std::regex_search(process_command, process_match, process_regex)) {
      // std::cout << "\targument:\n\t\t";
      // std::cout << process_match[0] << std::endl;
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
    // std::cout << "process_command:\n\t";
    // std::cout << process_command << std::endl;
    Process process{};
    while (std::regex_search(process_command, process_match, process_regex)) {
      // std::cout << "\t2 - argument:\n\t\t";
      // std::cout << process_match[0] << std::endl;
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
    std::cout << "Putting job in bacground" << std::endl;
    PutJobInBackground(job, false);
  }
}

void Shell::FormatJobInfo(const Job &job, const std::string &status) {
  std::cerr << job.process_group_id_ << " (" << status << "): " << job.command_
            << std::endl;
}

void Shell::WaitForJob(const Job &job) {
  int status;
  pid_t process_id;

  do {
    process_id = waitpid(WAIT_ANY, &status, WUNTRACED);
  } while (!MarkProcessStatus(process_id, status) && !job.IsStopped() &&
           !job.IsCompleted());
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
              std::cerr << process_id << ": Terminated by signal"
                        << WTERMSIG(process.status) << "." << std::endl;
            }
          }
          return 0;
        }
      }
    }
    std::cerr << "No child process " << process_id << "." << std::endl;
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

void Shell::Exit() { std::cout << "\nCya! :)\n\n"; }
