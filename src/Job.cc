#include "Job.h"

bool Job::Completed() {
  for (auto &process : processList) {
    if (!process.Completed()) {
      return false;
    }
  }

  return true;
}

bool Job::Stopped() {
  for (auto &process : processList) {
    if (!process.Completed() && !process.Stopped()) {
      return false;
    }
  }

  return true;
}
