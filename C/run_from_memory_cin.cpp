#!/bin/bash

echo "foo"
./memrun <(g++ -o /dev/fd/1 -x c++ <(sed -n "/^\/\*\*$/,\$p" $0)) 42

exit
/**
*/
#include <iostream>

int main(int argc, char *argv[])
{
  printf("bar %s\n", argc>1 ? argv[1] : "(undef)");

  for(char c; std::cin.read(&c, 1); )  { std::cout << c; }

  return 0;
}
