# Airport Simulator

## Overview
The Airport Simulator is a concurrent systems exercise that coordinates arrivals and departures using POSIX message queues, shared
memory segments, and condition variables. It exposes a FIFO interface so that test clients (such as `bin/send_info`) can submit
commands that the tower core (`bin/airport_simulator`) validates, schedules, and logs.

## Project structure
```
.
├── Makefile             # Builds binaries into bin/
├── config.txt           # Runtime configuration parameters read by the simulator
├── include/             # Shared headers (currently airport.h)
├── src/                 # All C sources for the simulator and helper tools
└── bin/                 # Build output after running `make`
```

## Building
The repository uses a single Makefile that builds both the simulator and the optional FIFO helper:

```sh
make            # builds bin/airport_simulator and bin/send_info
make clean      # removes bin/ and build/
```

Compilation enables warnings (`-Wall -Wextra -Wpedantic`) and targets C11 to keep the code portable.

## Running the simulator
1. Ensure the named pipe expected by `src/main.c` exists (by default `input_pipe`).
2. Launch the simulator and, optionally, the send_info helper:
   ```sh
   mkfifo input_pipe
   ./bin/airport_simulator &
   ./bin/send_info         # feeds scripted commands for quick smoke tests
   ```
3. Watch the log output (`airport_log.txt`) and use `kill -SIGUSR1 <pid>` to print runtime statistics.
4. Press `Ctrl+C` or send `SIGINT` when you want the simulator to gracefully release shared resources.

## Configuration
All timing information (unit time, runway durations, holding limits, etc.) is stored inside `config.txt`. Update it according to
your scenario before launching the simulator. Whenever you change this file you must restart the simulator to reload the values.

## Troubleshooting
- **Message queue errors** – confirm that System V message queues are enabled on your system and that you are not exceeding the
  default limits. Remove stale queues via `ipcrm` if necessary.
- **Shared-memory leaks** – the signal handler now destroys all shared memory segments, but if the program terminates abruptly you
  can remove them with `ipcs` + `ipcrm`.
- **FIFO write failures** – make sure the simulator is already running and has opened `input_pipe` for reading before executing
  `bin/send_info` or your own scripts.

## Contributing
1. Open an issue describing the fix or enhancement.
2. Submit a pull request that includes:
   - Updated tests or repro steps.
   - Clear comments/docstrings for any new public functions or data structures.
   - A concise summary in the README if behavior changes.
