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

  // return proc access name
  char buf[100];
  sprintf(buf, "/proc/%d/fd/%d", getpid(), mfd);
  puts(buf);
  fflush(stdout);

  // keep memory file active, until killed
  for(;;)  { sleep(3600); }

  return 0;
}
