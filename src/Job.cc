#include "Job.h"

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
