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
This is the only "run from memory" solution for C++ ("tcc -run" cannot help, because tcc is a C compiler only):  
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

## C/C++ interaction with bash in script

The C/C++ scripts seen sofar only showed how bash and C/C++ in single file can work.
2 years ago I created [4DoF robot arm](https://github.com/Hermann-SW/4DoF_robot_arm) repo.
That repo's "gamepad" tool is a C script, with executable created under "/tmp".
Its C code (Jason White's joystick.c from another repo) did low level gamepad event processing.
The bash code used the processed events to control 4 servo motors with pigpio library pigs commands.
Interaction was done, in that the stdout output of C code was input for the bash script.

I copied that tool over as [gamepad.c](gamepad.c) C script, and modified it to "run from memory", with minimal diff:  
```
pi@raspberrypi400:~/memrun/C $ diff ~/4DoF_robot_arm/tools/gamepad gamepad.c 
7,9d6
< js=/tmp/joystick
< if [ ! -f $js ]; then sed -n "/^\/\*\*$/,\$p" $0 | gcc -x c - -o $js; fi
< 
87c84
< done < <($js)
---
> done < <(./memrun <(gcc -o /dev/fd/1 -x c <(sed -n "/^\/\*\*$/,\$p" $0)))
pi@raspberrypi400:~/memrun/C $ 
```

This is how bash and C code parts work together:
```
...
while IFS= read -r line; do
    case $line in
        ...
    esac
    ...
    echo $g $u $l $p $d $calib

    pigs s 8 $g; pigs s 9 $u; pigs s 10 $l; pigs s 11 $p

done < <(./memrun <(gcc -o /dev/fd/1 -x c <(sed -n "/^\/\*\*$/,\$p" $0)))

exit
/**
 * Author: Jason White
...  
```


## Adding (tcc) "-run" option to gcc and g++

The symbolic links bin/gcc and bin/g++ to [bin/grun](bin/grun)
add tcc's "-run" option to gcc and g++. grun tests
whether "-run" option is present. If not normal gcc 
and g++ will be invoked as always. Otherwise gcc/g++
is used to compile into RAM, and memrun executes from there ([memrun.c](memrun.c) gets compiled automatically if needed).
The syntax is the same as tcc's syntax:
```
pi@raspberrypi400:~/memrun/C $ tcc | grep "\-run"
       tcc [options...] -run infile [arguments...]
  -run        run compiled source
pi@raspberrypi400:~/memrun/C $ 
```

First bin/gcc and bin/g++ need to be used instead of normal gcc and g++ (by preprending bin to $PATH):
```
pi@raspberrypi400:~/memrun/C $ which gcc
/usr/bin/gcc
pi@raspberrypi400:~/memrun/C $ export PATH=~/memrun/C/bin:$PATH
pi@raspberrypi400:~/memrun/C $ which gcc
/home/pi/memrun/C/bin/gcc
pi@raspberrypi400:~/memrun/C $ which g++
/home/pi/memrun/C/bin/g++
pi@raspberrypi400:~/memrun/C $ 
```

In case no "-run" is found, normal gcc is used ...
```
pi@raspberrypi400:~/memrun/C $ ls a.out 
ls: cannot access 'a.out': No such file or directory
pi@raspberrypi400:~/memrun/C $ gcc info.c 
pi@raspberrypi400:~/memrun/C $ ls a.out 
a.out
pi@raspberrypi400:~/memrun/C $
```

... as well as normal g++:
```
pi@raspberrypi400:~/memrun/C $ rm a.out 
pi@raspberrypi400:~/memrun/C $ sed -n "/^\/\*\*$/,\$p" run_from_memory_cin.cpp > demo.cpp
pi@raspberrypi400:~/memrun/C $ g++ demo.cpp 
pi@raspberrypi400:~/memrun/C $ ls a.out 
a.out
pi@raspberrypi400:~/memrun/C $ 
```


In case "-run" is present, gcc ...
```
pi@raspberrypi400:~/memrun/C $ uname -a | gcc -O3 -run info.c test
My process ID : 24787
argv[0] : /home/pi/memrun/C/bin/../memrun
argv[1] : test

evecve --> /usr/bin/ls -l /proc/24787/fd
total 0
lr-x------ 1 pi pi 64 Sep 20 00:48 0 -> 'pipe:[3396870]'
lrwx------ 1 pi pi 64 Sep 20 00:48 1 -> /dev/pts/0
lrwx------ 1 pi pi 64 Sep 20 00:48 2 -> /dev/pts/0
lr-x------ 1 pi pi 64 Sep 20 00:48 3 -> /proc/24787/fd
lr-x------ 1 pi pi 64 Sep 20 00:48 63 -> 'pipe:[3394399]'
pi@raspberrypi400:~/memrun/C $
```

... as well as g++ behave like tcc with "-run" (compile to and execute from RAM, no storing of executable in filesystem):
```
pi@raspberrypi400:~/memrun/C $ uname -a | g++ -O3 -Wall -run demo.cpp 42
bar 42
Linux raspberrypi400 5.10.60-v7l+ #1449 SMP Wed Aug 25 15:00:44 BST 2021 armv7l GNU/Linux
pi@raspberrypi400:~/memrun/C $ 
```

## Shebang processing

Like tcc, "-run" enabled gcc and g++ can be invoked from scripts (by [shebang](https://en.wikipedia.org/wiki/Shebang_(Unix)) character sequence).

gcc example:
```
pi@raspberrypi400:~/memrun/C $ ./HelloWorld.c
Hello, World!
pi@raspberrypi400:~/memrun/C $ cat HelloWorld.c
#!/home/pi/memrun/C/bin/gcc -run
#include <stdio.h>

int main(void)
{
  printf("Hello, World!\n");
  return 0;
}
pi@raspberrypi400:~/memrun/C $ 
```

g++ example:
```
pi@raspberrypi400:~/memrun/C $ ./HelloWorld.cpp 
Hello, World!
pi@raspberrypi400:~/memrun/C $ cat HelloWorld.cpp 
#!/home/pi/memrun/C/bin/g++ -run
#include <iostream>

int main(void)
{
  std::cout << "Hello, World!" << std::endl;
  return 0;
}
pi@raspberrypi400:~/memrun/C $ 
```

## memfd_create for bash

While memrun.c is fine, it does three things:
1. create memory file
2. copy compiled executable to memory file
3. execute memory file

2 and 3 can be done in bash or on command line. New [memfd_create.c](memfd_create.c) just exposes memfd_create system call for use with bash. After creating memory file, it creates the proc name for accessing the memory file and prints it to the calling process. That way bash script gets name for using the memory file. Memory file lives as long as memfd_create is running (it sleeps while waiting to be terminated). Bash script needs to extract pid from file name and just kill that pid, then memory file will not be accessible anymore.

Example script memfd_create4bash demonstrates all that:
```
#!/usr/bin/bash
#
# demo use of memory file (that lives in RAM), created with memfd_create.c

# read memory file name
read -r mf < <(./memfd_create &)

# show name
echo "$mf"

# fill memory file
echo foo > "$mf"
echo bar >> "$mf"

# output memory file
cat "$mf"

# terminate memfd_create.c by its pid, in order to close memory file
IFS="/"  read -ra mfa <<< "$mf"
kill "${mfa[2]}"

# memory file does not exist anymore after memfd_create.c ended
IFS=""  cat "$mf"
```

This is demo output:
```
pi@raspberrypi400:~/memrun/C $ ./memfd_create4bash 
/proc/1808/fd/3
foo
bar
cat: /proc/1808/fd/3: No such file or directory
pi@raspberrypi400:~/memrun/C $ 
```

[bin/grun](bin/grun) is used by "-run" enhanced gcc and g++. It now makes use of [memfd_create.c](memfd_create.c) as well. From git diff, these are the main steps for compiling into memory file and executing that. Finally memfd_create.c process gets terminated, because memory file should be freed. "${mfa[2]}" is pid extracted from memory file name ("/proc/**1808**/fd/3" in last example):
```
...
+# read memory file name
+read -r mf < <($mfc &)
+
+# compile $src, store executable in memory file ...
+"/usr/bin/$cmp" $opts -o $mf -x $lng <(shebang < "$src")
+
+# ... and execute it with args
+$mf "$@"
+
+# terminate memfd_create.c in order to close memory file
+IFS="/"
+read -ra mfa <<< "$mf"
+kill "${mfa[2]}"
```

## Now "-run" enabled gcc and g++ run completely from RAM

gcc/g++ creates some temporary files during compilation, that by default end up in "/tmp", which is not even a tmpfs under Raspberry Pi 32bit OS. For details see [this posting and following](https://www.raspberrypi.org/forums/viewtopic.php?f=31&t=320170&p=1918904#p1917142).

In order to run completely from RAM, a directory in RAM is needed for temporary files.  With [this commit](https://github.com/Hermann-SW/memrun/commit/5a2444ed5a82e562a8511f6a8a4201c5089dddf1) "/bin/grun" (bin/gcc and bin/g++ link to that file) does

- create memory file
- create filesyystem in memory file
- mount that filesystem (under "/proc/PID/fd")
- create executable "doit" in that filesystem, inclusive all temporary files
- execute "doit"
- release memory file&filesystem

So now "-run" enabled gcc/g++ runs completely in RAM!

```
pi@raspberrypi400:~/memrun/C $ fortune -s | bin/g++ -run demo.cpp foo 123
bar foo
Sorry.  I forget what I was going to say.
pi@raspberrypi400:~/memrun/C $ 
pi@raspberrypi400:~/memrun/C $ cat demo.cpp 
/**
*/
#include <iostream>

int main(int argc, char *argv[])
{
  printf("bar %s\n", argc>1 ? argv[1] : "(undef)");

  for(char c; std::cin.read(&c, 1); )  { std::cout << c; }

  return 0;
}
pi@raspberrypi400:~/memrun/C $
```
