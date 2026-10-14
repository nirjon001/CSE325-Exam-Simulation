# CSE325 — IELTS & GRE Exam Simulation (Operating System Project)

A multithreaded C program that simulates **300 students taking an exam in 10 rooms**,
using core Operating-System concurrency primitives: **threads, mutexes, condition
variables**, and **semaphores**.

This project was built for **CSE 325 – Operating System** (Spring 2026), Section 05,
Group 05.

---

## The problem

When students sit for a real exam, three things must be coordinated:

1. **Everyone starts together** — no one may begin before the invigilator says "go".
2. **Rooms have a fixed capacity** — a room that seats 30 people must never hold more.
3. **Everyone finishes together** — students stay in the room until the exam ends,
   then leave so the room can be reused.

The program models all three with threads and synchronisation primitives. The scenario
is framed as the real-world *"IELTS vs GRE"* confusion problem, but the code is a
general simulation of concurrent, capacity-limited resource access.

---

## How it works

| Concept | Used for |
|---|---|
| `pthread_create` | one thread per student (300 threads) |
| `pthread_mutex_t` (`global_mutex`) | protects shared counters and flags |
| `pthread_cond_t` (`start_cond`) | makes every student wait until the exam starts |
| `pthread_cond_t` (`end_cond`) | keeps students in the room until the exam ends |
| `sem_t room_sem[10]` | per-room capacity control (30 seats per room) |
| `student_id % NUM_ROOMS` | round-robin room assignment (load balancing) |

**Lifecycle of a student thread**

1. Wait on `start_cond` until `exam_started == 1`.
2. `sem_wait()` on its room's semaphore — blocks if the room is already full.
3. Enter the room: lock `global_mutex`, update occupancy/attendance, print an entry
   message, unlock.
4. Wait on `end_cond` until `exam_ended == 1`.
5. Leave: lock the mutex, update occupancy, print an exit message, unlock.
6. `sem_post()` the room semaphore so a waiting student can take the freed seat.
7. Free its argument struct and terminate.

The `main` thread creates all students, sleeps 2 s, broadcasts the start signal,
sleeps 3 s (the exam), broadcasts the end signal, `pthread_join`s every student, and
prints a per-room attendance summary.

> **Note on the student count.** `NUM_STUDENTS` is **300**, which exactly fills the
> 10 rooms × 30 seats, so every student gets a seat during the exam. Increase it
> (for example to `1000`) to stress-test the capacity control — the extra students
> then block on the room semaphore until a seat becomes free. This is the behaviour
> the project was demonstrated with during the lab test.

---

## Files

| File | Description |
|---|---|
| `exam_simulation2.c` | The complete simulation (single source file). |
| `CSE325_report_grp_5.docx` | Full project report (introduction, objectives, methodology, workflow, result analysis, appendix). |

---

## Build & run

The program uses POSIX threads, so it needs `gcc`/`clang` with `-pthread`.

**Linux / macOS**

```bash
gcc -pthread exam_simulation2.c -o exam_simulation
./exam_simulation
```

**Windows (MinGW-w64, e.g. the one bundled with Code::Blocks / MSYS2)**

```powershell
gcc -pthread exam_simulation2.c -o exam_simulation2.exe
.\exam_simulation2.exe
```

> The program prints colour with ANSI escape codes. On modern Windows terminals
> (Windows Terminal, VS Code, most MinGW builds) these render as green/red; on an
> old `cmd.exe` you may see the raw escape sequences instead.

---

## Sample output

```
All students waiting. Exam starts in 2 seconds...
Exam is in progress (3 seconds)...
--> Student 246 enters Room  7  | total attended:   1 | now inside:  1 | seats free: 29
--> Student 276 enters Room  7  | total attended:   2 | now inside:  2 | seats free: 28
...
<-- Student 246 leaves Room  7  | now inside: 29 | seats free:  1
...
===== EXAM ATTENDANCE SUMMARY =====
Room 1 : 30 students attended
Room 2 : 30 students attended
...
Room 10 : 30 students attended
```

Every room ends with exactly **30** students, and the per-room attendance totals
sum to 300 — confirming the semaphore capacity control and the mutex-protected
counters both work correctly.

---

## Notes on correctness

- The shared counters (`room_occupancy`, `room_attendance`, `exam_started`,
  `exam_ended`) are only ever touched while holding `global_mutex`, so there are no
  data races.
- The `while (!flag)` loops around `pthread_cond_wait` guard against spurious wake-ups.
- `pthread_create` return values are checked so thread-creation failure is reported
  instead of silently ignored.

---

## Course info

- **Course:** CSE 325 — Operating System
- **Section:** 05, Group 05
- **Instructor:** Shatabdi Roy Moon (Lecturer, Dept. of CSE)
- **Group members:**
  - Farhana Hasin — 2023-2-60-303
  - Ratul Hasan Nirjon — 2023-3-60-498
  - Fahim Shaharear — 2023-3-60-077
- **Tools:** C, Code::Blocks / GCC, `pthread`, POSIX semaphores
