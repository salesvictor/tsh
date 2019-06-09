#include "Shell.h"

#include <iostream>

int main(int argc, char *argv[]) {
  Shell tsh;
  tsh.InitShell();
  tsh.GetInput();
  tsh.Exit();

  return 0;
}
