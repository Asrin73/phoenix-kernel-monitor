# Phoenix: Linux Kernel System Call Monitoring Prototype

## 1. Overview

This project implements a prototype for monitoring selected Linux system calls using kernel probes (Kprobes). It provides a user-space interface for configuring monitoring behavior and retrieving system-call events through a character device.

The project is inspired by the Phoenix research paper on surviving unpatched vulnerabilities through accurate and efficient filtering of system-call sequences. It is an educational prototype and does not implement the complete Phoenix system.

## 2. Features

* Monitors selected system calls using Linux Kprobes.
* Supports `OFF`, `LOG`, and `BLOCK` configuration modes.
* Provides runtime configuration through `ioctl`.
* Exposes a character device at `/dev/phoenix`.
* Records system-call events with the process name and process ID.
* Includes user-space test programs.
* Includes a JSON-based finite-state machine (FSM) prototype for evaluating system-call sequences.

**Important:** The `BLOCK` mode is currently a configuration option only. It does not prevent system calls from executing.

## 3. Repository Structure

```text
phoenix-work/
├── src/
│   ├── Makefile
│   ├── phoenix_ioctl.h
│   └── phoenix_kprobe.c
├── tests/
│   ├── phoenix.c
│   ├── device_test.c
│   ├── event_test.c
│   ├── read_test.c
│   └── syscall_test.c
├── fsm/
│   ├── fsm.c
│   └── fsm.json
├── report/
│   ├── environment.txt
│   └── progress.md
├── .gitignore
└── README.md
```

## 4. Requirements

* Linux with a compatible kernel and kernel headers.
* GCC and Make.
* Root privileges for loading the kernel module and accessing the device, as required by the system configuration.
* The `libcjson-dev` package for building the FSM utility.

The implementation currently relies on x86-64 system-call symbols, so compatibility with other architectures is not guaranteed.

## 5. Build Instructions

### 5.1 Build the Kernel Module

Open a terminal and run:

```bash
cd ~/phoenix-work/src
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

### 5.2 Build the User-Space Utilities

```bash
cd ~/phoenix-work/tests

gcc -Wall -Wextra -o phoenix phoenix.c
gcc -Wall -Wextra -o device_test device_test.c
gcc -Wall -Wextra -o event_test event_test.c
gcc -Wall -Wextra -o read_test read_test.c
gcc -Wall -Wextra -o syscall_test syscall_test.c
```

### 5.3 Build the FSM Utility

```bash
cd ~/phoenix-work/fsm
gcc -Wall -Wextra -o fsm fsm.c -lcjson
```

## 6. Basic Usage

Load the kernel module:

```bash
cd ~/phoenix-work/src
sudo insmod phoenix_kprobe.ko
```

Configure logging mode:

```bash
cd ~/phoenix-work/tests
sudo ./phoenix --log
```

Run the system-call test program:

```bash
./syscall_test
```

Configure monitoring to OFF mode:

```bash
sudo ./phoenix --off
```

Other configuration options supported by the utility include:

```bash
sudo ./phoenix --block
sudo ./phoenix --syscall read
```

The FSM utility expects `fsm.json` in the current working directory. Run it from the FSM directory:

```bash
cd ~/phoenix-work/fsm
sudo ./fsm
```

The kernel module must be loaded and configured appropriately before testing event retrieval.

Unload the kernel module when finished:

```bash
sudo rmmod phoenix_kprobe
```

**Note:** These commands assume the project is located at `~/phoenix-work`. Adjust the paths if you use a different directory.

## 7. Current Limitations

* `BLOCK` mode does not currently block system calls.
* Events are stored in a single-slot buffer, so a new event may overwrite an unread event.
* The implementation monitors only a limited set of system calls.
* The probe symbols are architecture- and kernel-dependent.
* The current FSM utility processes a received event rather than continuously monitoring an entire sequence.
* This prototype is intended for learning and experimentation, not production security enforcement.

## 8. References

1. Phoenix research paper: [Phoenix: Surviving Unpatched Vulnerabilities via Accurate and Efficient Filtering of Syscall Sequences](https://www.ndss-symposium.org/ndss-paper/phoenix-surviving-unpatched-vulnerabilities-via-accurate-and-efficient-filtering-of-syscall-sequences/)
2. Linux kernel Kprobes documentation: https://docs.kernel.org/trace/kprobes.html
3. `ioctl(2)` manual page: https://www.man7.org/linux/man-pages/man2/ioctl.2.html
4. cJSON library: https://github.com/DaveGamble/cJSON
