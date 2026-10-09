# Phoenix Technical Assignment — Progress Log

## Day 2 — Environment Setup
**Date:** October 2, 2026

### Goal
Prepare a Linux development environment for kernel module development.

### Tasks completed
- Installed Ubuntu 24.04.5 LTS in VMware Workstation.
- Enabled Intel VT-x in the HP laptop BIOS/UEFI.
- Verified that virtualization is enabled in Windows Task Manager.
- Configured Ubuntu with 4 GB RAM and a 50 GB virtual disk.
- Enabled copy and paste between the host and guest systems.
- Installed `open-vm-tools` and `open-vm-tools-desktop`.
- Installed development tools and kernel headers.
- Created a VMware snapshot before kernel development.
- Created the project directories: `src`, `tests`, and `report`.

### Environment
- Operating system: Ubuntu 24.04.5 LTS
- Kernel: 7.0.0-38-generic
- RAM available to the VM: approximately 3.8 GiB
- Virtual disk: 49 GiB
- GCC: 13.3.0
- GNU Make: 4.3
- Git: 2.43.0
- Kernel headers: available for the running kernel

### Verification
The following tools and kernel headers were checked successfully:
- GCC: OK
- Make: OK
- Git: OK
- Kernel headers: OK

The environment details were also saved in `environment.txt`.

### Issues and solutions
- VMware initially could not start the VM because Intel VT-x was disabled.
- Enabled Intel VT-x in BIOS/UEFI and verified that Windows reported virtualization as enabled.
- Copy and paste between Ubuntu and Windows did not initially work.
- Enabled Guest Isolation settings and installed VMware tools.

### Notes and next steps
The development environment is ready for the next stage.
The next task is to study the Phoenix paper and understand its problem, main idea, and approach before starting implementation.

## Day 4 — First Linux Kernel Module (October 3, 2026)

### Goal
Build, load, and unload a basic Linux kernel module inside the Ubuntu virtual machine.

### Work completed
- Checked kernel configuration and confirmed that Kprobes are enabled:
  - CONFIG_KPROBES=y
  - CONFIG_KPROBES_ON_FTRACE=y
  - CONFIG_KPROBE_EVENTS=y
- Created `src/hello.c`, a basic kernel module with initialization and cleanup functions.
- Created a Makefile for building the module.
- Successfully compiled the module and generated `hello.ko`.
- Loaded the module using `insmod`.
- Confirmed that the module was loaded using `lsmod`.
- Verified the kernel log message: `Phoenix: Hello! Kernel module loaded.`
- Unloaded the module using `rmmod`.
- Confirmed that `lsmod` no longer listed the module.
- Verified the kernel log message: `Phoenix: Goodbye! Kernel module unloaded.`

### Notes
- The first `insmod` attempt failed because the terminal was not in the directory containing `hello.ko`. The command succeeded after changing to `~/phoenix-work/src`.
- The kernel reported that the unsigned out-of-tree module tainted the kernel. The module nevertheless loaded successfully in the VM.
- The BTF information was skipped during compilation because the required `vmlinux` file was unavailable. This did not prevent building or loading this simple module.

### Outcome
The basic kernel module build-load-unload cycle was completed successfully in the Ubuntu VM.

### Next step
Learn how to use Kprobes to observe a kernel function, before implementing syscall monitoring.
## Day 5 — Observing System Calls with Kprobes

### Objective
The goal was to observe selected system calls from a kernel module and identify the process that triggered them.

### Work Completed

1. Checked the running kernel version and searched for the relevant kernel symbols:
   - `__x64_sys_openat`
   - `__x64_sys_read`
   - `__x64_sys_write`

2. Performed an initial Kprobe experiment using the kernel tracing interface to verify that probing the `openat` function was possible.

3. Implemented a kernel module named `phoenix_kprobe.c`.
   - Registered Kprobes for `openat`, `read`, and `write`.
   - Used a shared pre-handler to record the kernel function name, process name, and process ID.
   - Added cleanup logic to unregister previously registered probes if registration failed.
   - Unregistered all probes when the module was removed.

4. Created and compiled a user-space test program named `syscall_test.c`.
   - The program opens a file, writes a message, and reads the message back.
   - The program ran successfully and displayed the expected output.

5. Loaded the kernel module and ran the test program.
   - Verified that the kernel log contained entries for all three system calls.
   - Confirmed that the entries identified the test process as `syscall_test`.
   - Successfully removed the kernel module after testing.

### Issue Encountered

When logging every observed `read` and `write` operation, inspecting the kernel log did not terminate as expected. A possible feedback loop was identified: log-reading tools can themselves trigger the probed system calls, while the probe handler generates additional log messages.

To reduce unrelated logging during the controlled test, the handler was modified to log events only when the current process name was `syscall_test`.

### Result

Successfully demonstrated that a kernel module using Kprobes can observe `openat`, `read`, and `write` and record the process name and PID.

### Limitations and Next Steps

- System-call arguments have not yet been recorded.
- The `off`, `log`, and `block` modes have not yet been implemented.
- Runtime control through `ioctl` remains to be implemented.
- Further work is needed to avoid excessive logging and to make the observation mechanism suitable for the assignment requirements.

## Day 6 — Implementing Off and Log Modes

### Objective

The goal of this day was to add basic operating modes to the Phoenix kernel module:

- `off`: no Phoenix event should be logged.
- `log`: selected events should be logged.

The goal was also to keep the Kprobe handler lightweight and test the behavior of both modes.

### Work Completed

1. Added two module modes:
   - `PHOENIX_MODE_OFF = 0`
   - `PHOENIX_MODE_LOG = 1`

2. Added a module parameter named `phoenix_mode`.
   - The default mode is `off`.
   - The parameter can be changed at runtime through the module parameter interface.
   - This mechanism is used as a temporary testing interface; runtime control will later be implemented using `ioctl` as required by the assignment.

3. Updated the Kprobe handler.
   - The handler first checks the current mode.
   - If the mode is not `log`, it returns immediately.
   - When logging is enabled, only events generated by the controlled `syscall_test` process are recorded.

4. The logged event currently contains:
   - The probed kernel function name.
   - The process name.
   - The process ID.

5. Recompiled the kernel module successfully.

6. Tested the `off` mode.
   - Verified that the module parameter was `0`.
   - Executed `syscall_test`.
   - The test program completed successfully.
   - No new Phoenix log entries were generated for the new execution.

7. Tested the `log` mode.
   - Changed the module parameter from `0` to `1` at runtime.
   - Executed `syscall_test` again.
   - Verified new Phoenix log entries for `openat`, `read`, and `write`.
   - The entries correctly identified the process as `syscall_test` and included its PID.

### Example Logged Events

The module generated entries similar to:

    Phoenix: syscall=__x64_sys_openat process=syscall_test pid=4716
    Phoenix: syscall=__x64_sys_read process=syscall_test pid=4716
    Phoenix: syscall=__x64_sys_write process=syscall_test pid=4716

### Design Considerations

The Kprobe handler performs an early mode check so that no logging-related work is performed when the module is in `off` mode.

Logging is restricted to the controlled test process to avoid excessive kernel log activity from unrelated system processes.

The current implementation uses `pr_info()` for basic kernel logging. A more complete design may require a more efficient kernel-to-userspace notification mechanism, especially when event rates increase.

### Current Limitations

- Syscall arguments are not yet captured.
- The `block` mode is not implemented.
- Runtime control currently uses a module parameter rather than `ioctl`.
- Kernel-to-userspace notification has not yet been implemented.
- The current logging mechanism is intended for the initial prototype and testing stage.

### Result

Both `off` and `log` modes were successfully implemented and tested.

In `off` mode, the controlled system calls continued to execute normally but no new Phoenix events were logged.

In `log` mode, selected `openat`, `read`, and `write` events generated by `syscall_test` were recorded with the syscall name, process name, and process ID.

## Day 7 — Runtime Control with ioctl

### Objective

The goal of this day was to understand how a user-space program can communicate with a kernel module and change its configuration at runtime.

The main mechanism used for this communication was ioctl.

### Work Completed

1. Created a simple character device kernel module named `phoenix_device`.

2. Created the device:

       /dev/phoenix

3. Implemented an ioctl interface for runtime configuration.

4. Created a shared header file:

       phoenix_ioctl.h

   This file defines the ioctl commands shared by the kernel module and the user-space test program.

5. Implemented the following ioctl commands:

   - SET_MODE
   - SET_SYSCALL
   - SET_PROCESS

6. Created a user-space test program:

       device_test.c

   The program opens `/dev/phoenix` and sends the ioctl commands to the kernel module.

### Testing

The kernel module was compiled and loaded successfully.

The user-space test program was then executed with root privileges.

The following operations completed successfully:

       SET_MODE → mode = 1
       SET_SYSCALL → syscall = read
       SET_PROCESS → process = syscall_test

The kernel log confirmed that all three commands were received:

       Phoenix: mode changed to 1
       Phoenix: syscall changed to read
       Phoenix: process changed to syscall_test

### Architecture

The communication path implemented during this stage is:

       device_test.c
            |
            | ioctl()
            v
       /dev/phoenix
            |
            v
       phoenix_device.c

The shared ioctl definitions are provided by:

       phoenix_ioctl.h

### Design Understanding

The character device provides a controlled interface between user space and the kernel module.

The user-space program does not directly modify kernel variables. Instead, it sends commands using ioctl, and the kernel module validates and applies the requested configuration.

The current implementation stores three runtime configuration values:

- Phoenix mode
- Selected syscall
- Selected process

### Current Limitations

This stage is a prototype for the runtime control mechanism.

The ioctl interface is currently implemented in a separate device module and has not yet been integrated with the Kprobe-based syscall monitoring logic.

The block mode is also not implemented yet.

Kernel-to-user-space event notification is not implemented yet.

### Result

The runtime control path was successfully implemented and tested.

The user-space program can communicate with the kernel module through `/dev/phoenix` and change the mode, selected syscall, and selected process at runtime.

## Day 8 — User-Space Command-Line Utility

### Objective

The goal of this day was to develop a user-space C utility for controlling the Phoenix kernel module through command-line options and ioctl.

The utility was designed to support the required runtime control operations and provide basic input validation.

### Work Completed

1. Created a user-space C utility:

       phoenix.c

2. Implemented the following command-line options:

       --log
       --off
       --block
       --syscall <name>

3. Implemented command-line argument validation.

   The utility detects:
   - Missing arguments.
   - Missing syscall names after --syscall.
   - Unknown command-line options.

4. Connected the command-line utility to the Phoenix character device:

       /dev/phoenix

5. Used ioctl to send runtime configuration changes to the kernel module.

6. The existing SET_MODE ioctl was extended to support three modes:

       0 = OFF
       1 = LOG
       2 = BLOCK

7. The existing SET_SYSCALL ioctl was used to configure the selected syscall.

### Testing

The utility was compiled successfully using:

       gcc -Wall -Wextra -o phoenix phoenix.c

The following commands were successfully tested:

       sudo ./phoenix --log
       sudo ./phoenix --off
       sudo ./phoenix --syscall read
       sudo ./phoenix --block

The kernel log confirmed that the commands reached the kernel module.

Examples of verified kernel messages:

       Phoenix: mode changed to 1
       Phoenix: mode changed to 0
       Phoenix: syscall changed to read
       Phoenix: mode changed to 2

### Input Validation

The following invalid inputs were also tested:

       sudo ./phoenix

       sudo ./phoenix --syscall

       sudo ./phoenix --hello

The utility correctly displayed usage information or an unknown-option error.

### Architecture

The current user-space control path is:

       Command line
            |
            v
       phoenix.c
            |
            | ioctl()
            v
       /dev/phoenix
            |
            v
       phoenix_device.c
            |
            v
       Runtime configuration

The current configuration values include:

- Phoenix mode
- Selected syscall
- Selected process

### Design Understanding

The user-space utility acts as a control interface for the kernel module.

The utility does not directly modify kernel variables. Instead, it opens the Phoenix character device and sends commands through ioctl.

The kernel module validates the received values and updates its internal runtime configuration.

This allows the configuration to be changed while the module remains loaded.

### Current Limitations

The BLOCK mode currently represents a runtime configuration state only.

The actual mechanism for preventing a selected syscall from executing has not yet been implemented.

The Kprobe monitoring module and the ioctl control module have not yet been fully integrated.

The JSON FSM functionality and kernel-to-user-space event notification required by the complete user-space component remain for later stages.

### Result

The basic Phoenix command-line control utility was successfully implemented and tested.

The utility can communicate with the kernel module through ioctl and configure OFF, LOG, BLOCK, and selected syscall settings at runtime.
## Day 9 — FSM and Kernel-to-User Event Notification

### Objective

The goal of Day 9 was to connect the kernel-side syscall monitoring mechanism to the user-space FSM and establish a communication path from the kernel to user space.

### Work completed

#### 1. FSM implementation

A JSON-based FSM was implemented using the cJSON library.

The FSM configuration is stored in:

`~/phoenix-work/fsm/fsm.json`

The configuration currently contains three states:

- open → openat
- read → read
- write → write

The user-space FSM program loads and validates the JSON configuration and maintains the current state.

#### 2. FSM state transitions

The FSM was first tested with artificial syscall events.

The following behavior was verified:

- Matching syscall → move to the next state.
- Non-matching syscall → remain in the current state.
- Reaching the final state → return to State 0.

This confirmed that the FSM logic works independently.

#### 3. Kernel-to-user communication

The existing `/dev/phoenix` character device was extended with a `read()` operation.

A simple kernel event buffer was added to store the latest observed syscall event.

The communication path is:

Kernel Kprobe → event buffer → `/dev/phoenix` → user-space `read()`.

#### 4. Integration with Kprobes

The main kernel module was consolidated into:

`~/phoenix-work/src/phoenix_kprobe.c`

The module now contains:

- Kprobes for openat, read, and write.
- `/dev/phoenix` character device.
- IOCTL interface.
- OFF/LOG mode control.
- Kernel-to-user event notification.

The Makefile was also updated so that `phoenix_kprobe.c` is compiled as the kernel module.

#### 5. Syscall name normalization

Kernel Kprobe symbols such as:

`__x64_sys_write`

are different from the syscall names used by the FSM.

A normalization function was therefore added in user space to convert:

- `__x64_sys_openat` → `openat`
- `__x64_sys_read` → `read`
- `__x64_sys_write` → `write`

#### 6. End-to-end test

The complete communication path was tested successfully.

The test program generated real syscalls:

`syscall_test → Kprobe → kernel event buffer → /dev/phoenix → user space → FSM`

The FSM successfully received a real kernel event:

`__x64_sys_write`

which was normalized to:

`write`

The FSM then compared the received syscall with the expected syscall for the current state.

The test correctly produced:

`NO MATCH: state does not change.`

This confirms that the real kernel event is successfully reaching the user-space FSM.

### Files created or modified

- `src/phoenix_kprobe.c`
- `src/phoenix_ioctl.h`
- `src/Makefile`
- `fsm/fsm.json`
- `fsm/fsm.c`
- `tests/event_test.c`

### Current limitations

The current kernel event buffer is a simple single-slot buffer. A new event may overwrite an older event if it has not yet been consumed.

Actual syscall blocking has not yet been implemented. The BLOCK mode currently represents the configuration state only.

Process and syscall filtering still need to be integrated fully with the Kprobe logic.

### Result

Day 9 successfully established the main kernel-to-user event path and connected real kernel syscall events to the user-space FSM.
