Here is an updated, highly structured, and polished `README.md` formatted cleanly for your repository:

```markdown
# Win32 Process Orchestration & Sandboxing Architecture

A lightweight, high-performance C++ Master-Worker orchestration framework for Windows platforms. 

This repository provides a production-ready baseline for supervising modular background applications (`A.exe`, `B.exe`, `C.exe`) from a single central orchestrator (`Oil.exe`). The system enforces strict process sandboxing via Windows Job Objects, prevents duplicate instances, manages windowless background execution, and streams all logs into a centralized console via Win32 Named Pipes.

---

## 🏗️ Architecture Overview

```text
                        +----------------------------+
                        |     User / Launcher        |
                        +----------------------------+
                                      |
                           Executes A.exe / B.exe
                                      v
                        +----------------------------+
                        |    Child Common Bootstrap  |
                        +----------------------------+
                                      |
                     Checks Pipe: Is Oil.exe active?
                                    / \
                                NO /   \ YES
                                  /     \
    Spawns Oil.exe (Visible) <---+       +---> Sends "SPAWN:<exe>" via IPC
               |                                            |
               v                                            v
+-----------------------------------------------------------------------+
|                             Oil.exe                                   |
| - Hosts Single Master Console Window                                  |
| - Enforces Single-Instance Guard via Named Pipe                       |
| - Tracks Active Children (Rejects Duplicates)                         |
+-----------------------------------------------------------------------+
        |                                                 |
        | Spawns Hidden                                   | Spawns Hidden
        v                                                 v
+-------------------------------+               +-------------------------------+
|     Sandboxed Child (A.exe)   |               |     Sandboxed Child (B.exe)   |
| - Mode: --child               |               | - Mode: --child               |
| - Bound to Job Object         |               | - Bound to Job Object         |
| - Redirected STDOUT Pipe      |               | - Redirected STDOUT Pipe      |
+-------------------------------+               +-------------------------------+
                \                                               /
                 \---> [ Streams stdout live to Oil.exe ] <---/

```

---

## Key Features

* **Self-Healing Launcher Chain:** Double-clicking any worker executable (`A.exe`, `B.exe`, `C.exe`) automatically checks if the host orchestrator (`Oil.exe`) is running. If missing, the worker boots `Oil.exe` automatically and hands off execution cleanly.
* **Single-Instance Enforcement:**
* **Master Guard:** `Oil.exe` uses `FILE_FLAG_FIRST_PIPE_INSTANCE` on a dedicated named pipe (`\\.\pipe\OilSingleInstanceGuard`) to block duplicate orchestrators from opening.
* **Worker Guard:** `Oil.exe` keeps an in-memory active set of child processes. Requests to launch a worker that is already active are cleanly rejected until the running process exits.


* **Win32 Job Object Sandboxing:** Workers run within an isolated Windows Job Object restricting UI handles, clipboard read/write access, and system parameter modifications.
* **Windowless Worker Execution:** Background workers launch silently (`CREATE_NO_WINDOW`) while their standard outputs (`stdout`) stream live into `Oil.exe`'s master console through anonymous Win32 pipes.

---

## 📁 Repository Structure

```text
├── Oil.cpp           # Master orchestrator & console application source
├── ChildCommon.h     # Shared bootstrap logic for worker binaries
├── A.cpp             # Worker module A implementation
├── B.cpp             # Worker module B implementation
├── C.cpp             # Worker module C implementation
├── .gitignore        # Git ignore rules for C++/Win32 build artifacts
└── README.md         # Framework documentation

```

---

## ⚙️ Core Mechanics

### 1. IPC & Pipe Infrastructure

* **`\\.\pipe\OilSingleInstanceGuard`:** Used by worker binaries to check for `Oil.exe`'s existence, and by `Oil.exe` to enforce single-instance binding.
* **`\\.\pipe\OilConsolePipe`:** Communication channel for requesting worker launches from `Oil.exe`.
* **Anonymous Redirect Pipes:** Created per spawned child to pipe `stdout` back to `Oil.exe` reader threads.

### 2. Execution Flow

1. User executes `A.exe`.
2. `A.exe` probes `OilSingleInstanceGuard`. If unreachable, it executes `Oil.exe` via `CreateProcessW`.
3. `A.exe` connects to `OilConsolePipe`, sends `SPAWN:A.exe`, and immediately exits.
4. `Oil.exe` accepts the request, verifies `A.exe` isn't already running, binds the target binary to a Win32 Job Object, and executes `A.exe --child` with `CREATE_NO_WINDOW`.
5. Unbuffered output streams continuously into `Oil.exe`'s console window.
6. Upon process exit, `Oil.exe` updates its active worker list.

---

## 🛠️ Build & Compilation

### Prerequisites

* **OS:** Windows 10 / 11 / Server
* **Compiler:** MinGW-w64 (`g++`)

### GCC / MinGW

```bash
windres resource.rc -O coff -o resource.res

g++ Oil.cpp resource.res -o Oil.exe -O3 -s -DNDEBUG -static -static-libgcc -static-libstdc++ -mwindows
g++ A.cpp -o A.exe -O3 -s -DNDEBUG -static -static-libgcc -static-libstdc++ -mwindows
g++ B.cpp -o B.exe -O3 -s -DNDEBUG -static -static-libgcc -static-libstdc++ -mwindows
g++ C.cpp -o C.exe -O3 -s -DNDEBUG -static -static-libgcc -static-libstdc++ -mwindows

```

> **Important:** All binaries (`Oil.exe`, `A.exe`, `B.exe`, `C.exe`) must reside in the **same folder** for relative execution path resolution.

---

## 🚀 How to Run

1. Open your build directory in File Explorer.
2. Double-click **`A.exe`**.
3. **`Oil.exe`** will automatically launch its master console window and initiate `A.exe`.
4. While running, double-click **`B.exe`** or **`C.exe`**—their output will seamlessly append to `Oil.exe`'s console.
5. Attempting to launch **`A.exe`** again while it is currently active will produce a duplicate execution rejection notice in `Oil.exe`.

```

```