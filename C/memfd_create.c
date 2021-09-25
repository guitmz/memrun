// create memory file by memfd_create system call, for use in bash scripts
//
#include <stdio.h>
#define __USE_GNU
#include <sys/mman.h>
#include <unistd.h>

int main(void)
{
  // create memory file
  int mfd = memfd_create("rab.oof", MFD_CLOEXEC);

  // child waits for signal, keeps memory file alive
  int pid;
  if (!(pid = fork()))  { pause(); }

  // return pid and mfd
  printf("%d %d\n", pid, mfd);
  fflush(stdout);

  return 0;
}
