<div align="center">

# 🧵 IELTS & GRE Exam Simulation

### Multithreaded Exam Room Simulation in C

**CSE 325 — Operating System · Spring 2026 · Section 05 · Group 05**

![Language](https://img.shields.io/badge/Language-C-00599C?style=for-the-badge&logo=c&logoColor=white)
![Concurrency](https://img.shields.io/badge/Concurrency-pthreads-6f42c1?style=for-the-badge)
![Sync](https://img.shields.io/badge/Sync-Mutex%20%7C%20CondVar%20%7C%20Semaphore-0e8a86?style=for-the-badge)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows-2e8b57?style=for-the-badge&logo=linux&logoColor=white)
![Status](https://img.shields.io/badge/Status-Complete-brightgreen?style=for-the-badge)

*A concurrent, capacity-limited simulation of 300 students sitting an exam across 10 rooms — built entirely with POSIX threads, mutexes, condition variables, and semaphores.*

[Overview](#-overview) · [How it works](#-how-it-works) · [Build & Run](#-build--run) · [Output](#-sample-output) · [Report](#-project-report)

</div>

---

## 📖 Overview

Real exams must coordinate three competing constraints at once:

> 1. **Everyone starts together** — no one begins before the invigilator says *go*.
> 2. **Rooms have fixed capacity** — a 30-seat room must never hold 31 people.
> 3. **Everyone finishes together** — students stay seated until the exam ends, then leave.

This project models all three with one thread per student and the classic
Operating-System synchronisation toolbox. The scenario is framed around the
real-world *"IELTS vs. GRE"* confusion problem, but the engine underneath is a
textbook demonstration of **concurrency control over a limited resource**.

---

## ⚙️ How it works

| Primitive | Role in the simulation |
|:--|:--|
| 🧵 `pthread_create` | spawns **one thread per student** (300 threads) |
| 🔒 `pthread_mutex_t` | protects every shared counter and flag (`global_mutex`) |
| 🚦 `pthread_cond_t start_cond` | makes all students **wait for the exam to begin** |
| 🛑 `pthread_cond_t end_cond` | holds students in the room **until the exam ends** |
| 🎟️ `sem_t room_sem[10]` | **per-room capacity** — exactly 30 tickets per room |
| 🔄 `student_id % 10` | **round-robin** room assignment (load balancing) |

### 🔄 Lifecycle of a student thread

```
        ┌──────────────────────┐
        │  wait on start_cond  │  ← everyone blocks here
        └──────────┬───────────┘
                   │  main() broadcasts "exam started"
                   ▼
        ┌──────────────────────┐
        │ sem_wait(room_sem)   │  ← blocks if the room is full (30 seats)
        └──────────┬───────────┘
                   ▼
        ┌──────────────────────┐
        │ enter room (mutex)   │  occupancy++, attendance++, print entry
        └──────────┬───────────┘
                   ▼
        ┌──────────────────────┐
        │  wait on end_cond    │  ← stay seated until exam ends
        └──────────┬───────────┘
                   │  main() broadcasts "exam ended"
                   ▼
        ┌──────────────────────┐
        │ leave room (mutex)   │  occupancy--, print exit
        └──────────┬───────────┘
                   ▼
        ┌──────────────────────┐
        │ sem_post(room_sem)   │  ← frees a seat for a waiting student
        └──────────────────────┘
```

The `main` thread creates everyone, sleeps **2 s**, broadcasts the *start* signal,
sleeps **3 s** (the exam), broadcasts the *end* signal, `pthread_join`s all students,
and prints a per-room attendance summary.

> 💡 **Why 300 students?** 10 rooms × 30 seats = exactly 300, so everyone is seated
> during the exam. Bump `NUM_STUDENTS` (e.g. to `1000`) to watch the semaphores make
> the overflow students **wait for a free seat** — the exact behaviour demonstrated
> during the lab test.

---

## 🚀 Build & Run

**Linux / macOS**

```bash
gcc -pthread exam_simulation2.c -o exam_simulation
./exam_simulation
```

**Windows (MinGW-w64 — bundled with Code::Blocks / MSYS2)**

```powershell
gcc -pthread exam_simulation2.c -o exam_simulation2.exe
.\exam_simulation2.exe
```

> 🎨 The program prints ANSI colour codes (green = entering, red = leaving).
> They render in Windows Terminal, VS Code, and most modern consoles.

---

## 🖥️ Sample Output

```text
All students waiting. Exam starts in 2 seconds...
Exam is in progress (3 seconds)...
--> Student 246 enters Room  7 | total attended:   1 | now inside:  1 | seats free: 29
--> Student  66 enters Room  7 | total attended:   3 | now inside:  3 | seats free: 27
...
<-- Student  12 leaves Room  3 | now inside: 29 | seats free:  1
...

===== EXAM ATTENDANCE SUMMARY =====
Room 1 : 30 students attended
Room 2 : 30 students attended
...
Room 10 : 30 students attended
```

✅ Every room ends with **exactly 30** students — the semaphore capacity control and
the mutex-protected counters both behave correctly.

---

## 🛡️ Correctness notes

- ✅ Shared state (`room_occupancy`, `room_attendance`, `exam_started`, `exam_ended`)
  is only touched while holding `global_mutex` → **no data races**.
- ✅ Every `pthread_cond_wait` sits inside a `while (!flag)` loop → **spurious wake-ups handled**.
- ✅ `pthread_create` return values are checked → **failures reported, never silent**.

---

## 📂 Project Report

| File | Description |
|:--|:--|
| 📄 [`exam_simulation2.c`](./exam_simulation2.c) | Complete single-file simulation |
| 📝 [`CSE325_report_grp_5.docx`](./CSE325_report_grp_5.docx) | Full report — intro, objectives, methodology, workflow, result analysis, appendix |

---

## 👥 Course Info

| | |
|:--|:--|
| **Course** | CSE 325 — Operating System |
| **Section** | 05 · Group 05 |
| **Instructor** | Shatabdi Roy Moon (Lecturer, Dept. of CSE) |
| **Members** | Farhana Hasin *(2023-2-60-303)* · Ratul Hasan Nirjon *(2023-3-60-498)* · Fahim Shaharear *(2023-3-60-077)* |
| **Tools** | C · GCC / Code::Blocks · `pthread` · POSIX semaphores |

---

<div align="center">

### ⭐ If this project helped you, consider giving it a star!

*Built with ☕ and a lot of `pthread_join`.*

</div>
