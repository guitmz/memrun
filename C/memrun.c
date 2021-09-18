#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#define __USE_GNU
#include <sys/mman.h>
#include <unistd.h>

void cpyfd(int tfd, int sfd);
void exc(int mfd, char *argv[], char *envp[]);

int main(int argc, char*argv[])
{
  int  fd = 0;
  if (argc > 1 && strncmp(argv[1], "/dev/fd/", 8) == 0)
  { 
    fd = open(argv[1], O_CLOEXEC);
    argv[1] = argv[0];
    ++argv;
  }
  int mflags = strchr(argv[0], '_') ? 0 : MFD_CLOEXEC; 
  int mfd = memfd_create("foo.bar", mflags);
  cpyfd(mfd, fd);
  exc(mfd, argv, argv+argc);
  return 0;
}

void exc(int mfd, char *argv[], char *envp[])
{
  char buf[]="/proc/self/fd/________________";
  sprintf(buf+14, "%d", mfd);
  execve(buf, argv, envp);
}

void cpyfd(int tfd, int sfd)
{
  char buf[1024];
  for(;;)
  {
    int r = read(sfd, buf, 1024);
    if      (r > 0)           { write(tfd, buf, r); }
    else if (r == 0)          { break; }
    else if (errno != EINTR)  { exit(errno); }
  }
}
