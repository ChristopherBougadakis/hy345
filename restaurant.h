#ifndef RESTAURANT_H
#define RESTAURANT_H

#include <pthread.h>
#include <semaphore.h>

// Δομή για κάθε τραπέζι
typedef struct {
    int id;                    // Αριθμός τραπεζιού
    int capacity;              // Μέγιστη χωρητικότητα
    int occupied;              // Πόσες θέσεις είναι κατειλημμένες
    pthread_mutex_t mutex;     // Mutex για προστασία του τραπεζιού
} Table;

// Δομή για κάθε ομάδα πελατών
typedef struct {
    int id;                    // Αριθμός ομάδας
    int size;                  // Μέγεθος ομάδας
    int assigned_table;        // Σε ποιο τραπέζι ανατέθηκε (-1 αν δεν έχει ανατεθεί)
    sem_t seated_sem;          // Semaphore για να περιμένει μέχρι να καθίσει
    int in_queue;              // Αν η ομάδα είναι στην ουρά αναμονής
} Group;

// Καθολικές μεταβλητές
extern Table *tables;
extern Group *groups;
extern int num_tables;
extern int num_groups;
extern int table_capacity;
extern int groups_finished;

extern pthread_mutex_t queue_mutex;      // Mutex για την ουρά αναμονής
extern pthread_mutex_t print_mutex;      // Mutex για thread-safe εκτύπωση
extern sem_t waiter_sem;                 // Semaphore για να ξυπνάει ο waiter

// Συναρτήσεις
void* group_thread(void* arg);
void* waiter_thread(void* arg);
void print_waiting_groups();

#endif