#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
  printf("My process ID : %d\n", getpid());
  printf("argv[0] : %s\n", argv[0]);
  printf(argc>1 ? "argv[1] : %s\n" : "no argv[1]", argv[1]);
  char buf[100];
  sprintf(buf, "/proc/%d/fd", getpid());
  char *args[4]={"/usr/bin/ls", "-l", buf, NULL};
  printf("\nevecve --> %s %s %s\n", args[0], args[1], args[2]);
  execve(args[0], args, args+2);
  return 0;
}
