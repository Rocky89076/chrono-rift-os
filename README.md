\# ⚔️ Chrono Rift OS



\*\*A retro 16-bit JRPG that serves as a real-time visual simulation of a multi-process POSIX operating system.\*\*



Chrono Rift OS isn't just a JRPG; it is a distributed, multi-process C++ application built to demonstrate core Operating System concepts. Behind the SFML graphical interface, isolated user-space processes communicate with a Kernel arbiter using POSIX Shared Memory, Semaphores, and Inter-Process Communication (IPC) queues.



\## 🧠 Core OS Concepts Implemented



Instead of a standard game loop, this project relies entirely on OS-level architecture:



\* \*\*Strict Process Isolation:\*\* Heroes (Human Interfacing Process) and Enemies (Artificial System Process) run as completely isolated C++ executables.

\* \*\*Shared Memory \& IPC:\*\* Entities cannot modify each other's memory. Combat is handled by generating `ActionRequests` and pushing them into a lock-protected Circular IPC Queue for the Kernel (Arbiter) to process safely.

\* \*\*Thread \& CPU Scheduling:\*\* Implements a Stamina-based CPU time-slicing algorithm using `pthread`. Threads wait, execute, and sleep based on calculated arrival times.

\* \*\*Deadlock Detection \& Resolution:\*\* Artifacts are critical resources protected by `PTHREAD\_MUTEX\_ROBUST`. A background Watchdog thread actively monitors for circular waits and forces deadlock resolution.

\* \*\*Contiguous Memory Management:\*\* The player inventory is a simulated RAM block utilizing a First-Fit memory allocation strategy, complete with Page Fault penalties and Disk Swapping.

\* \*\*POSIX Signals:\*\* Game interrupts (Stun, Flee, Ultimates) are handled using raw OS hardware signals (`SIGUSR1`, `SIGTERM`, `SIGSTOP`, `SIGCONT`).



\## 🛠️ Tech Stack

\* \*\*C++ (POSIX Standard):\*\* Core systems programming, multithreading, and memory mapping.

\* \*\*SFML:\*\* Frontend rendering and audio isolated to a read-only visualizer thread.

\* \*\*Docker:\*\* Containerized execution environment.

\* \*\*X11 / XLaunch:\*\* GUI forwarding from the Linux container to the host machine.



\## 🚀 How to Run (Docker + XLaunch)



Because this relies on strict POSIX Linux headers, it is containerized via Docker. 



\*\*Prerequisites (Windows):\*\*

1\. Install \[Docker Desktop](https://www.docker.com/products/docker-desktop/).

2\. Install an X-Server like \*\*VcXsrv (XLaunch)\*\*.



\*\*Launch Instructions:\*\*

1\. Start \*\*XLaunch\*\*. Ensure "Multiple windows" is selected, "Display number" is set to `0`, and critically, check \*\*"Disable access control"\*\*.

2\. Open your terminal and build the Docker image:

&#x20;  ```bash

&#x20;  docker build -t chrono\_rift\_os .

