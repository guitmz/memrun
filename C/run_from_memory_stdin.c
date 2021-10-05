#!/bin/bash

echo "foo"
#./memrun <(gcc -o /dev/fd/1 -x c <(sed -n "/^\/\*\*$/,\$p" $0)) 42
bin/gcc -run <(sed -n "/^\/\*\*$/,\$p" $0) 42

exit
/**
*/
#include <stdio.h>

int main(int argc, char *argv[])
{
  printf("bar %s\n", argc>1 ? argv[1] : "(undef)");

  for(int c=getchar(); EOF!=c; c=getchar())  { putchar(c); }

  return 0;
}
