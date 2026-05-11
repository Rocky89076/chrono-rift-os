#Chrono Rift OS

> *A retro 16-bit JRPG built entirely on POSIX operating system primitives — processes, threads, shared memory, signals, and mutexes — visualized in real time through an SFML graphical engine.*

---

## What Is This?

Chrono Rift OS is a turn-based RPG where every game mechanic maps directly to a real OS concept. It is not a game with OS concepts bolted on — **the OS concepts ARE the game engine.**

| Game Concept | OS Equivalent |
|---|---|
| Hero & Enemy processes | Isolated user-space processes (`fork` + `exec`) |
| Combat actions | IPC via a circular action queue in shared memory |
| Turn order | Stamina-based CPU scheduling (`pthread` per entity) |
| Stun mechanic | Asynchronous hardware interrupt (`SIGUSR1` → `SIGUSR2`) |
| Ultimate Ability | Full process suspension (`SIGSTOP` / `SIGCONT` + `SIGALRM`) |
| Weapon inventory | First-fit contiguous memory allocator with swap-out |
| Legendary artifacts | Shared critical resources with robust cross-process mutexes |
| Deadlock scenario | Circular wait between Solar Core and Lunar Blade |
| Deadlock resolution | Background watchdog thread forces preemptive release |

---

## Architecture

The system runs as **four separate OS processes** that share a single memory-mapped region (`/dev/shm/chrono_rift_shm`) and never communicate through pipes.

```
┌─────────────────────────────────────────────────┐
│              /dev/shm/chrono_rift_shm            │
│   GameState struct — shared by all processes     │
│   [ Players | Enemies | ActionQueue | Mutexes ]  │
└──────────┬──────────────┬───────────────┬────────┘
           │              │               │
    ┌──────┴──────┐  ┌────┴────┐  ┌──────┴──────┐
    │   arbiter   │  │   hip   │  │     asp     │
    │ Game Arbiter│  │ Human   │  │  AI / NPC   │
    │ + Watchdog  │  │ Process │  │   Process   │
    │             │  │ 1 thread│  │ 1 thread    │
    │ Drains IPC  │  │ per hero│  │ per enemy   │
    │ action queue│  │         │  │             │
    └─────────────┘  └─────────┘  └─────────────┘
           │
    ┌──────┴──────┐
    │render_thread│
    │  SFML GUI   │
    │ reads state │
    │ draws screen│
    └─────────────┘
```

### Process Roles

**`arbiter`** — The kernel. Creates and owns shared memory, forks all child processes, runs the deadlock watchdog thread, and is the sole authority for applying combat results from the IPC queue. No other process can directly modify enemy or player combat state.

**`hip` (Human Interface Process)** — One process per human player session. Spawns one `pthread` per controlled character. Each thread accumulates stamina, waits for GUI input via shared memory, then enqueues an `ActionRequest` — it never modifies the game world directly.

**`asp` (Automated Strategic Process)** — The AI brain. Spawns one `pthread` per enemy. Each thread runs its own stamina accumulation and turn logic independently and concurrently.

**`render_thread`** — The SFML visual engine. Reads the shared memory state every frame and renders HP bars, stamina bars, an action log, sprite animations, and sound. Writes player keypress choices back to shared memory as integer action codes.

---

## OS Concepts in Detail

### Shared Memory & IPC
All inter-process communication goes through a single `GameState` struct mapped with `shm_open` + `mmap(MAP_SHARED)`. The only IPC mechanism is a **circular FIFO action queue** inside this struct — `hip` enqueues, `arbiter` dequeues and applies. Pipes are not used anywhere.

### Scheduling
Each entity (hero or enemy) runs in its own `pthread`. Every second, `speed` points are added to `stamina`. When stamina reaches its maximum (100 for heroes, 150 for enemies), the entity competes for `turn_mutex`. Only one entity holds this cross-process mutex at a time — enforcing serial turn execution despite concurrent stamina accumulation.

### Synchronization Layers

| Lock | Type | Protects |
|---|---|---|
| `memory_mutex` | `sem_t` (pshared, binary) | Action queue, logs, all general state writes |
| `turn_mutex` | `pthread_mutex_t` (PROCESS_SHARED) | Serial turn execution across all processes |
| `solar_core_mutex`, `lunar_blade_mutex`, `eclipse_relic_mutex` | `pthread_mutex_t` (ROBUST + PROCESS_SHARED) | Artifact ownership — survives process crashes via `EOWNERDEAD` |
| `floor_mutex` | `pthread_mutex_t` (ROBUST + PROCESS_SHARED) | Weapon pickup race condition |

### Signals
- **Stun:** Attacker calls `kill(target_pid, SIGUSR1)` → process-level handler reads `target_stun_id` → `pthread_kill(thread_id, SIGUSR2)` → target thread's handler calls `sleep(3)`, interrupting it mid-execution for exactly 3 seconds.
- **Ultimate Ability:** Watchdog detects both artifacts held by one player → `kill(asp_pid, SIGSTOP)` freezes the entire enemy process → `alarm(10)` → `SIGALRM` fires after 10 seconds → `SIGCONT` resumes enemies and artifacts are cleared.
- **Quit / Flee:** `hip` sends `kill(arbiter_pid, SIGTERM)` → arbiter's handler sets `game_over = true` for a clean shutdown.

### Memory Management (Inventory)
The player inventory is a 20-slot linear array — a simulated RAM block. Weapon allocation uses **first-fit contiguous allocation**. When no contiguous block is large enough, the allocator evicts the leftmost weapon to `long_term_storage[]` (simulated disk) and retries recursively. This directly demonstrates external fragmentation and swap-out behaviour.

### Deadlock
The Solar Core and Lunar Blade are exclusive resources, each protected by a cross-process robust mutex. If Player A holds Solar Core and waits for Lunar Blade while an NPC holds Lunar Blade and waits for Solar Core, a circular wait forms. The arbiter's **watchdog thread** detects this every 2 seconds by comparing `holder[]` and `waiter[]` arrays and forces the lower-priority entity to release all held resources.

---

## Project Structure

```
chrono-rift-os/
├── shared_data.h          # Shared contract — GameState struct, all sync primitives
├── arbiter/
│   └── arbiter.cpp        # Arbiter process — game logic, IPC processing, watchdog
├── hip/
│   ├── hip.cpp            # Human Interface Process — player threads, inventory
│   └── render_thread.cpp  # SFML graphical engine — standalone visualizer
├── asp/
│   └── asp.cpp            # AI process — NPC threads, enemy decision logic
├── Sprites/               # SFML sprite assets
├── Makefile               # Builds all four executables
├── Dockerfile             # Ubuntu 22.04 container with SFML + build tools
├── run.sh                 # One-command launcher (WSL + Docker + X11)
└── requirements.txt
```

---

## How to Run

This project uses Linux-specific syscalls (`shm_open`, `sem_init` with `pshared`, `PTHREAD_PROCESS_SHARED`) and must run inside a Linux environment. The included Docker setup handles this automatically on Windows via WSL2.

### Prerequisites

- [Docker Desktop](https://www.docker.com/products/docker-desktop/) with WSL2 backend enabled
- WSL2 (Windows Subsystem for Linux 2) — comes with Docker Desktop
- An X11 server on Windows — **[VcXsrv (XLaunch)](https://sourceforge.net/projects/vcxsrv/)** is recommended

> **If you are already on Linux or inside WSL2**, Docker and XLaunch are not required — you can build and run natively with `make && ./arbiter/arbiter & ./hip/render_thread`.

---

### Step 1 — Configure XLaunch (Windows only)

1. Open **XLaunch** (VcXsrv).
2. Select **"Multiple windows"**, set Display number to **`0`**, click Next.
3. Select **"Start no client"**, click Next.
4. On the Extra settings page, check **"Disable access control"** — this is required for Docker to forward the window.
5. Click Finish. XLaunch will run in the system tray.

---

### Step 2 — Build the Docker Image

Do this once. Only needs to be repeated if the Dockerfile changes.

```bash
docker build -t chrono_rift_os .
```

---

### Step 3 — Launch the Game

```bash
bash run.sh
```

That is all. `run.sh` detects that it is running on the host (not inside Docker), spins up the container with X11 forwarding, compiles the project with `make`, and launches the game. The SFML window will appear on your desktop.

When you close the game window, the container shuts down and cleans up automatically.

---

### What `run.sh` Does Internally

```
WSL shell → docker run (mounts project dir + X11 socket)
         → make (compiles all 4 executables inside container)
         → ./arbiter/arbiter & (starts arbiter in background, creates shared memory)
         → sleep 1 (waits for shared memory to be ready)
         → ./hip/render_thread (launches SFML window, foreground)
         → [window closed] → kills arbiter → container exits
```

---

### Troubleshooting

| Problem | Fix |
|---|---|
| SFML window does not appear | Make sure XLaunch is running and "Disable access control" was checked |
| `docker: image not found` | Run `docker build -t chrono_rift_os .` first |
| Black screen / crash on launch | Close any leftover containers with `docker ps` and `docker kill <id>`, then retry |
| Audio missing | Expected in some setups — PulseAudio forwarding via WSLg is best-effort |

---

## Build Details

```makefile
CC      = g++
CFLAGS  = -Wall -Wextra -std=c++17 -pthread
LDFLAGS = -lrt -pthread -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
```

| Flag | Purpose |
|---|---|
| `-std=c++17` | Required for `inline` variables in `shared_data.h` |
| `-pthread` | Enables POSIX threading at both compile and link time |
| `-lrt` | Links `librt` for `shm_open`, `shm_unlink`, `clock_gettime` |
| `-lsfml-*` | SFML graphics, windowing, system, and audio libraries |

To clean all compiled binaries:
```bash
make clean
```

---

## Group Members

| Name | Roll Number |
|---|---|
| Zainab Nisar | 24i-0838 |
| Abdullah Nadeem | 24i-0617 |

**Course:** CS 2006 — Operating Systems, Spring 2026
**Section:** BCSD
