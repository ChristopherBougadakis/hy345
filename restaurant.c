#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "restaurant.h"

// Καθολικές μεταβλητές
Table *tables = NULL;
Group *groups = NULL;
int num_tables = 0;
int num_groups = 0;
int table_capacity = 0;
int groups_finished = 0;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t waiter_sem;

// Thread function για κάθε ομάδα πελατών
void* group_thread(void* arg) {
    Group *group = (Group*)arg;
    
    // Τυχαία καθυστέρηση πριν την άφιξη (0-10 δευτερόλεπτα)
    int arrival_delay = rand() % 11;
    sleep(arrival_delay);
    
    // Άφιξη στο εστιατόριο
    pthread_mutex_lock(&print_mutex);
    printf("[GROUP %d] Arrived with %d people (after %d sec)\n", 
           group->id, group->size, arrival_delay);
    pthread_mutex_unlock(&print_mutex);
    
    // Προσθήκη στην ουρά αναμονής
    pthread_mutex_lock(&queue_mutex);
    group->in_queue = 1;
    pthread_mutex_unlock(&queue_mutex);
    
    // Ειδοποίηση του waiter ότι υπάρχει νέα ομάδα
    sem_post(&waiter_sem);
    
    // Αναμονή μέχρι ο waiter να ανακοινώσει τραπέζι
    sem_wait(&group->seated_sem);
    
    // Κάθισμα στο τραπέζι
    pthread_mutex_lock(&print_mutex);
    printf("[GROUP %d] Seated at Table %d with %d people\n",
           group->id, group->assigned_table, group->size);
    pthread_mutex_unlock(&print_mutex);
    
    // Φαγητό για τυχαίο χρονικό διάστημα (5-15 δευτερόλεπτα)
    int eating_time = 5 + rand() % 11;
    
    pthread_mutex_lock(&print_mutex);
    printf("[GROUP %d] Eating (%d people) for %d seconds...\n",
           group->id, group->size, eating_time);
    pthread_mutex_unlock(&print_mutex);
    
    sleep(eating_time);
    
    // Αποχώρηση από το τραπέζι
    int table_id = group->assigned_table;
    pthread_mutex_lock(&tables[table_id].mutex);
    tables[table_id].occupied -= group->size;
    int remaining = tables[table_id].occupied;
    pthread_mutex_unlock(&tables[table_id].mutex);
    
    pthread_mutex_lock(&print_mutex);
    printf("[GROUP %d] Left Table %d (%d/%d occupied)\n",
           group->id, table_id, remaining, table_capacity);
    pthread_mutex_unlock(&print_mutex);
    
    // Ενημέρωση ότι ολοκληρώθηκε
    pthread_mutex_lock(&queue_mutex);
    groups_finished++;
    int all_done = (groups_finished == num_groups);
    pthread_mutex_unlock(&queue_mutex);
    
    // Ειδοποίηση του waiter για το ελεύθερο τραπέζι ή τερματισμό
    sem_post(&waiter_sem);
    
    return NULL;
}

// Εκτύπωση ομάδων που περιμένουν (Πρέπει να καλείται με queue_mutex locked)
void print_waiting_groups() {
    int first = 1;
    
    pthread_mutex_lock(&print_mutex);
    printf("[WAITER] Groups waiting: ");
    
    int found = 0;
    for (int i = 0; i < num_groups; i++) {
        if (groups[i].in_queue == 1) {
            if (!first) printf(", ");
            printf("%d", groups[i].id);
            first = 0;
            found = 1;
        }
    }
    
    if (!found) {
        printf("None");
    }
    printf("\n");
    pthread_mutex_unlock(&print_mutex);
}

// Thread function για τον σερβιτόρο
void* waiter_thread(void* arg) {
    while (1) {
        // Αναμονή για ειδοποίηση (νέα ομάδα ή ελεύθερο τραπέζι)
        sem_wait(&waiter_sem);
        
        pthread_mutex_lock(&queue_mutex);
        
        // Έλεγχος αν όλες οι ομάδες έχουν τελειώσει
        if (groups_finished == num_groups) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }
        
        // Προσπάθεια να ανακατανείμει ομάδες σε τραπέζια
        int assigned_any = 0;
        
        do {
            assigned_any = 0;
            
            for (int g = 0; g < num_groups; g++) {
                if (groups[g].in_queue == 1) {
                    // Αναζήτηση κατάλληλου τραπεζιού
                    for (int t = 0; t < num_tables; t++) {
                        pthread_mutex_lock(&tables[t].mutex);
                        int available = tables[t].capacity - tables[t].occupied;
                        
                        if (available >= groups[g].size) {
                            // Ανάθεση τραπεζιού στην ομάδα
                            tables[t].occupied += groups[g].size;
                            groups[g].assigned_table = t;
                            groups[g].in_queue = 0;
                            
                            pthread_mutex_unlock(&tables[t].mutex);
                            
                            pthread_mutex_lock(&print_mutex);
                            printf("[WAITER] Assigned group %d (size=%d) to Table %d (%d/%d occupied)\n",
                                   groups[g].id, groups[g].size, t, 
                                   tables[t].occupied, tables[t].capacity);
                            pthread_mutex_unlock(&print_mutex);
                            
                            // Εκτύπωση ομάδων που περιμένουν
                            print_waiting_groups();
                            
                            // "Ξύπνημα" της ομάδας
                            sem_post(&groups[g].seated_sem);
                            
                            assigned_any = 1;
                            break;
                        }
                        pthread_mutex_unlock(&tables[t].mutex);
                    }
                    
                    if (assigned_any) break;
                }
            }
        } while (assigned_any); // Συνέχεια αν βρέθηκε κάποια ανάθεση
        
        pthread_mutex_unlock(&queue_mutex);
    }
    
    return NULL;
}