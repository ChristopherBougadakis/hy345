

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "restaurant.h"

int main(int argc, char *argv[]) {
    // Έλεγχος ορισμάτων γραμμής εντολών
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <num_tables> <num_groups> <table_capacity>\n", argv[0]);
        return 1;
    }
    
    // Ανάγνωση παραμέτρων
    num_tables = atoi(argv[1]);
    num_groups = atoi(argv[2]);
    table_capacity = atoi(argv[3]);
    
    // Έλεγχος εγκυρότητας παραμέτρων
    if (num_tables <= 0 || num_groups <= 0 || table_capacity <= 0) {
        fprintf(stderr, "Error: All parameters must be positive integers\n");
        return 1;
    }
    
    // Αρχικοποίηση γεννήτριας τυχαίων αριθμών
    srand(time(NULL));
    
    // Δημιουργία πινάκων για τραπέζια και ομάδες
    tables = (Table*)malloc(num_tables * sizeof(Table));
    groups = (Group*)malloc(num_groups * sizeof(Group));
    
    if (tables == NULL || groups == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }
    
    // Αρχικοποίηση semaphore για τον waiter
    sem_init(&waiter_sem, 0, 0);
    
    // Αρχικοποίηση τραπεζιών
    for (int i = 0; i < num_tables; i++) {
        tables[i].id = i;
        tables[i].capacity = table_capacity;
        tables[i].occupied = 0;
        pthread_mutex_init(&tables[i].mutex, NULL);
    }
    
    // Αρχικοποίηση ομάδων
    for (int i = 0; i < num_groups; i++) {
        groups[i].id = i;
        groups[i].size = 1 + rand() % table_capacity;  // Τυχαίο από 1 έως table_capacity
        groups[i].assigned_table = -1;
        groups[i].in_queue = 0;
        sem_init(&groups[i].seated_sem, 0, 0);
    }
    
    // Δημιουργία threads για τις ομάδες
    pthread_t *group_threads = (pthread_t*)malloc(num_groups * sizeof(pthread_t));
    if (group_threads == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }
    
    for (int i = 0; i < num_groups; i++) {
        if (pthread_create(&group_threads[i], NULL, group_thread, &groups[i]) != 0) {
            fprintf(stderr, "Error: Failed to create group thread %d\n", i);
            return 1;
        }
    }
    
    // Δημιουργία thread για τον waiter
    pthread_t waiter_tid;
    if (pthread_create(&waiter_tid, NULL, waiter_thread, NULL) != 0) {
        fprintf(stderr, "Error: Failed to create waiter thread\n");
        return 1;
    }
    
    // Αναμονή για όλα τα group threads να ολοκληρωθούν
    for (int i = 0; i < num_groups; i++) {
        pthread_join(group_threads[i], NULL);
    }
    
    // Ειδοποίηση του waiter να τερματίσει
    sem_post(&waiter_sem);
    
    // Αναμονή για τον waiter
    pthread_join(waiter_tid, NULL);
    
    printf("All groups have left. Closing restaurant.\n");
    
    // Καθαρισμός πόρων
    for (int i = 0; i < num_tables; i++) {
        pthread_mutex_destroy(&tables[i].mutex);
    }
    
    for (int i = 0; i < num_groups; i++) {
        sem_destroy(&groups[i].seated_sem);
    }
    
    sem_destroy(&waiter_sem);
    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&print_mutex);
    
    free(tables);
    free(groups);
    free(group_threads);
    
    return 0;
}