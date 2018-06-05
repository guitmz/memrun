package main

import (
	"fmt"
	"io/ioutil"
	"os"
	"syscall"
	"unsafe"
)

// the constant values below are valid for x86_64
const (
	mfdCloexec  = 0x0001
	memfdCreate = 319
)

func runFromMemory(displayName string, filePath string) {
	fdName := "" // *string cannot be initialized
	fd, _, _ := syscall.Syscall(memfdCreate, uintptr(unsafe.Pointer(&fdName)), uintptr(mfdCloexec), 0)

	buffer, _ := ioutil.ReadFile(filePath)
	_, _ = syscall.Write(int(fd), buffer)

	fdPath := fmt.Sprintf("/proc/self/fd/%d", fd)
	_ = syscall.Exec(fdPath, []string{displayName}, nil)
}

func main() {
	lenArgs := len(os.Args)
	if lenArgs < 3 || lenArgs > 3 {
		fmt.Println("Usage: memrun process_name elf_binary")
		os.Exit(1)
	}

	runFromMemory(os.Args[1], os.Args[2])
}
