# Airport Simulator

**Overview:**

AirportSimulator is a program that simulates the operation of an airport using pipes, shared memory, mutexes, and condition variables. It receives commands from a pipe, logs the events to a file, and shows statistics when signaled.

**Usage:**

1. Open two terminals:

   - Terminal 1: Compile and run the program (e.g., `./main`).
   - Terminal 2: Send commands from the pipe (e.g., `echo 'DEPARTURE TP40 init: 20 takeoff: 100' > input_pipe`).

2. After starting the program, it creates a log file (airport_log.txt) to save all necessary information.

3. To view statistics, send a signal of type SIGUSR1 to the program.

4. To gracefully terminate the program, use Ctrl+C, and all resources used will be released.

**Concurrency Handling:**

Mutexes are used to lock functions and threads accessing or modifying the same data. Condition variables are used to notify threads when an event occurs and they need to resume execution.

**Shared Memory:**

Shared memory is used to store data shared between different parts of the program, and a condition variable is placed in shared memory to notify the simulation manager when the data has changed.
