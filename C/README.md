# memrun
Small tool written in C to run ELF binaries (verified on x86_64 and armv7l), either from standard input, or via first argument process substitution. Works on Linux where kernel version is >= 3.17 (relies on the `memfd_create` syscall).


# Usage

Build it with `$ gcc memrun.c -o memrun`. Allows to run C source compiled with gcc passed via pipe to memrun without temporary filesystem files ("tcc -run" equivalent). Anonymous file created and executed lives in RAM, without link in filesystem. If you are not interested in [memrun.c](memrun.c) details, you can skip discussion wrt [info.c](info.c), and continue with [C script](#C-script) discussion.

Here gcc compiled ELF output gets stored in stdout (file descriptor 1), and piped to memrun that executes it:
```
pi@raspberrypi400:~/memrun/C $ gcc info.c -o /dev/fd/1 | ./memrun
My process ID : 20043
argv[0] : ./memrun
no argv[1]
evecve --> /usr/bin/ls -l /proc/20043/fd
total 0
lr-x------ 1 pi pi 64 Sep 18 22:27 0 -> 'pipe:[1601148]'
lrwx------ 1 pi pi 64 Sep 18 22:27 1 -> /dev/pts/4
lrwx------ 1 pi pi 64 Sep 18 22:27 2 -> /dev/pts/4
lr-x------ 1 pi pi 64 Sep 18 22:27 3 -> /proc/20043/fd
pi@raspberrypi400:~/memrun/C $ 
```

There is a simple reason why the memfd file descriptor is not visible. clang-tidy raised warning "'memfd_create' should use MFD_CLOEXEC where possible" when passing 0 as 2nd argument of "memfd_create()". When executable filename contains '_' character, intentionally 0 gets passed, and memfd gets not closed and is visible:
```
pi@raspberrypi400:~/memrun/C $ ln -s memrun memrun_
pi@raspberrypi400:~/memrun/C $ gcc info.c -o /dev/fd/1 | ./memrun_
My process ID : 20098
argv[0] : ./memrun_
no argv[1]
evecve --> /usr/bin/ls -l /proc/20098/fd
total 0
lr-x------ 1 pi pi 64 Sep 18 22:27 0 -> 'pipe:[1601207]'
lrwx------ 1 pi pi 64 Sep 18 22:27 1 -> /dev/pts/4
lrwx------ 1 pi pi 64 Sep 18 22:27 2 -> /dev/pts/4
lrwx------ 1 pi pi 64 Sep 18 22:27 3 -> '/memfd:foo.bar (deleted)'
lr-x------ 1 pi pi 64 Sep 18 22:27 4 -> /proc/20098/fd
pi@raspberrypi400:~/memrun/C $ 
```

## C script

Previous method to run ELF file via pipe makes it impossible for compiled C code to access "C script" stdin. Running compiled C code via first argument process substitution resolves the problem. gcc creates ELF to stdout, and since it cannot determine language from file name, "-x c" specifies it. 2nd process substitution extracts C code from "C script" [run_from_memory_stdin.c](run_from_memory_stdin.c):
```
...
echo "foo"
./memrun <(gcc -o /dev/fd/1 -x c <(sed -n "/^\/\*\*$/,\$p" $0)) 42
...
```

Here you see that this method allows to pass 42 as argv[1] to execution. C code does output stdin for verification (that really C script stdin is accessible to C code):
```
pi@raspberrypi400:~/memrun/C $ grep mfd memrun.c | ./run_from_memory_stdin.c 
foo
bar 42
void exc(int mfd, char *argv[], char *envp[]);
  int mfd = memfd_create("foo.bar", MFD_CLOEXEC);
  cpyfd(mfd, fd);
  exc(mfd, argv, argv+argc);
void exc(int mfd, char *argv[], char *envp[])
  sprintf(buf+14, "%d", mfd);
pi@raspberrypi400:~/memrun/C $ 
```

18 lines only C script [run_from_memory_stdin.c](run_from_memory_stdin.c) (it has to be executable!) shows how the previous output was created:

```
#!/bin/bash

echo "foo"
./memrun <(gcc -o /dev/fd/1 -x c <(sed -n "/^\/\*\*$/,\$p" $0)) 42

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
```

## C++ script

For those who prefer C++ over C, very few changes are needed for "C++ script" [run_from_memory_cin.cpp](run_from_memory_cin.cpp).
This is the only run from memory solution for C++ ("tcc -run" cannot help, because tcc is a C compiler only):  
```
pi@raspberrypi400:~/memrun/C $ diff run_from_memory_stdin.c run_from_memory_cin.cpp
4c4
< ./memrun <(gcc -o /dev/fd/1 -x c <(sed -n "/^\/\*\*$/,\$p" $0)) 42
---
> ./memrun <(g++ -o /dev/fd/1 -x c++ <(sed -n "/^\/\*\*$/,\$p" $0)) 42
9c9
< #include <stdio.h>
---
> #include <iostream>
15c15
<   for(int c=getchar(); EOF!=c; c=getchar())  { putchar(c); }
---
>   for(char c; std::cin.read(&c, 1); )  { std::cout << c; }
pi@raspberrypi400:~/memrun/C $ 
```
