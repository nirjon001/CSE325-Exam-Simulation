/*
 * exam_simulation2.c
 * Compile: gcc -pthread exam_simulation2.c -o exam_simulation2
 * Run:     ./exam_simulation2      (Linux / macOS)
 *          exam_simulation2.exe    (Windows, built with MinGW-w64)
 *
 * This program simulates 300 students taking an exam in 10 rooms.
 * Each room can hold only 30 students at a time (like a capacity limit).
 * The exam starts 2 seconds after all threads are created, then runs for 3 seconds.
 *
 * With 300 students the 10 rooms (10 x 30 = 300 seats) are exactly full, so
 * every student gets a seat during the exam. Increase NUM_STUDENTS (for
 * example to 1000) to stress-test the capacity control: the extra students
 * then block on the room semaphore until a seat becomes free.
 *
 * ALGORITHM EXPLAINED (in beginner words):
 * - We create one thread for each student.
 * - Each student is assigned a room based on student_id % 10 (simple round‑robin).
 * - A "global mutex" protects shared variables like exam_started, exam_ended, and room counters.
 * - Condition variables (start_cond, end_cond) make all students wait until the exam begins,
 *   and then wait again until the exam ends.
 * - A semaphore per room controls how many students can be inside that room at once
 *   (like a key that allows 30 students to enter, then blocks the 31st until someone leaves).
 * - When the exam starts, every student tries to enter their room (sem_wait).
 * - Inside the room they update counters and print a green "enters" message.
 * - Then they wait on the end_cond until the main thread signals that the exam is over.
 * - After that, they leave the room, update counters, print a red "leaves" message,
 *   and release the room semaphore (sem_post) so the next waiting student can enter.
 * - Finally, the main thread waits for all students to finish and prints a summary.
 *
 * This is like a combination of a "barrier" (start together) and a "bounded buffer"
 * (rooms with limited capacity) but with waiting students instead of a queue.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_STUDENTS   300    // total number of student threads (matches the project report)
#define ROOM_CAPACITY  30     // max students inside one room at the same time
#define NUM_ROOMS      10     // number of exam rooms

// Global variables for synchronization (I learned this from the textbook)
pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;  // protects shared data
pthread_cond_t  start_cond   = PTHREAD_COND_INITIALIZER;   // signals exam start
pthread_cond_t  end_cond     = PTHREAD_COND_INITIALIZER;   // signals exam end

int exam_started = 0;   // becomes 1 when main() says "go"
int exam_ended   = 0;   // becomes 1 after 3 seconds of exam

sem_t room_sem[NUM_ROOMS];          // semaphores for room capacity (like a key ring)
int room_attendance[NUM_ROOMS] = {0};  // total students that entered this room
int room_occupancy[NUM_ROOMS]  = {0};  // how many students are currently inside

// Structure to pass two arguments to each student thread
typedef struct {
    int student_id;
    int room;
} student_args;

// This is the function that every student thread runs
void* student_thread(void* arg) {
    student_args* args = (student_args*)arg;   // get the arguments
    int id   = args->student_id;
    int room = args->room;

    // --- WAIT FOR EXAM TO START ---
    // Lock mutex, then wait until exam_started becomes 1.
    // The while loop is needed because condition variables can wake up spuriously.
    pthread_mutex_lock(&global_mutex);
    while (!exam_started) {
        pthread_cond_wait(&start_cond, &global_mutex);
    }
    pthread_mutex_unlock(&global_mutex);

    // --- ENTER THE ROOM (use semaphore to respect capacity) ---
    sem_wait(&room_sem[room]);   // if room is full, this blocks until a seat is free

    // Now inside the room: update counters and print a nice green message
    pthread_mutex_lock(&global_mutex);
    room_occupancy[room]++;                // one more student inside
    room_attendance[room]++;               // count this student for the summary
    int current_occ = room_occupancy[room];
    int available = ROOM_CAPACITY - current_occ;   // free seats left
    printf("--> \033[32mStudent %3d enters Room %2d \033[0m | total attended: %3d | now inside: %2d | seats free: \033[32m%2d\033[0m\n",
           id, room+1, room_attendance[room], current_occ, available);
    pthread_mutex_unlock(&global_mutex);

    // --- WAIT FOR EXAM TO END ---
    // All students stay in their rooms until the main thread signals "exam_ended"
    pthread_mutex_lock(&global_mutex);
    while (!exam_ended) {
        pthread_cond_wait(&end_cond, &global_mutex);
    }
    pthread_mutex_unlock(&global_mutex);

    // --- LEAVE THE ROOM ---
    // Update counters again and print a red leaving message
    pthread_mutex_lock(&global_mutex);
    room_occupancy[room]--;                // one student leaves
    int after_leave_occ = room_occupancy[room];
    int available_after = ROOM_CAPACITY - after_leave_occ;
    printf("<-- \033[31mStudent %3d leaves Room %2d \033[0m | now inside: %2d | seats free: \033[32m%2d\033[0m\n",
           id, room+1, after_leave_occ, available_after);
    pthread_mutex_unlock(&global_mutex);

    // Give the semaphore back so another waiting student can enter (if exam had lasted longer)
    sem_post(&room_sem[room]);

    free(args);   // don't forget to free the memory we allocated for the arguments
    return NULL;
}



// MAIN PART..................
int main() {
    pthread_t students[NUM_STUDENTS];   // array to hold thread IDs

    // Initialize each room's semaphore with the room capacity (like 30 free tokens)
    for (int i = 0; i < NUM_ROOMS; i++) {
        sem_init(&room_sem[i], 0, ROOM_CAPACITY);
    }

    // Create all student threads. Each student gets a room ID = student_id % 10
    // so students are spread evenly across rooms.
    for (int i = 0; i < NUM_STUDENTS; i++) {
        student_args* args = (student_args*)malloc(sizeof(student_args));
        args->student_id = i;
        args->room = i % NUM_ROOMS;      // simple assignment: room0 gets 0,10,20,... ; room1 gets 1,11,21,...
        if (pthread_create(&students[i], NULL, student_thread, args) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    // Give threads a moment to start and then block on start_cond
    printf("All students waiting. Exam starts in 2 seconds...\n");
    sleep(2);

    // Signal all students that the exam has started
    pthread_mutex_lock(&global_mutex);
    exam_started = 1;
    pthread_cond_broadcast(&start_cond);   // wake up every thread waiting on start_cond
    pthread_mutex_unlock(&global_mutex);

    // Let the exam run for 3 seconds (during this time students are inside the rooms)
    printf("Exam is in progress (3 seconds)...\n");
    sleep(3);

    // Signal all students that the exam is over
    pthread_mutex_lock(&global_mutex);
    exam_ended = 1;
    pthread_cond_broadcast(&end_cond);     // wake up every thread waiting on end_cond
    pthread_mutex_unlock(&global_mutex);

    // Wait for all student threads to finish (they will all leave and then terminate)
    for (int i = 0; i < NUM_STUDENTS; i++) {
        pthread_join(students[i], NULL);
    }

    // Print a summary: how many students attended each room
    printf("\n===== EXAM ATTENDANCE SUMMARY =====\n");
    for (int r = 0; r < NUM_ROOMS; r++) {
        printf("Room %d : \033[32m%d\033[0m students attended\n", r+1, room_attendance[r]);
    }

    // Clean up semaphores
    for (int i = 0; i < NUM_ROOMS; i++) {
        sem_destroy(&room_sem[i]);
    }

    return 0;
}
