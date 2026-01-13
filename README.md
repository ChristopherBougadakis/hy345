# Ask2 - Restaurant Simulation with Threads

## Περιγραφή Έργου

Η Ask2 είναι ένας **multi-threaded simulator** ενός εστιατορίου όπου:
- **Ομάδες πελατών** (threads) φτάνουν σε τυχαίες ώρες
- Ένας **σερβιτόρος** (waiter thread) ανακατανέμει ομάδες σε τραπέζια
- **Τραπέζια** έχουν περιορισμένη χωρητικότητα
- Συγχρονισμός με **mutexes** και **semaphores**

Αυτό είναι ένα κλασικό παράδειγμα **concurrent programming** με χρήση POSIX threads.

---

## Αρχιτεκτονική του Συστήματος

### 1. Δομές Δεδομένων

#### `struct Table`
```c
typedef struct {
    int id;                  // Αριθμός τραπεζιού (0, 1, 2, ...)
    int capacity;            // Μέγιστη χωρητικότητα (θέσεις)
    int occupied;            // Πόσες θέσεις είναι κατειλημμένες
    pthread_mutex_t mutex;   // Mutex για προστασία από race conditions
} Table;
```

**Σκοπός**: Αναπαριστά ένα φυσικό τραπέζι στο εστιατόριο
- `capacity` και `occupied` χρειάζονται mutex γιατί διαφορετικές ομάδες μπορεί να τα αλλάζουν ταυτόχρονα

#### `struct Group`
```c
typedef struct {
    int id;                  // Αριθμός ομάδας (0, 1, 2, ...)
    int size;                // Μέγεθος ομάδας (αριθμός ατόμων)
    int assigned_table;      // Σε ποιο τραπέζι ανατέθηκε (-1 = ακόμα όχι)
    sem_t seated_sem;        // Semaphore για αναμονή μέχρι τραπέζι
    int in_queue;            // Σημαία: 1 = περιμένει, 0 = ήδη κάθεται ή έφυγε
} Group;
```

**Σκοπός**: Αναπαριστά μια ομάδα πελατών
- Κάθε ομάδα έχει το δικό της semaphore για συγχρονισμό με το waiter

---

## Καθολικές Μεταβλητές

```c
Table *tables;               // Array των τραπεζιών
Group *groups;               // Array των ομάδων
int num_tables;              // Αριθμός τραπεζιών
int num_groups;              // Αριθμός ομάδων
int table_capacity;          // Χωρητικότητα κάθε τραπεζιού
int groups_finished;         // Πόσες ομάδες έχουν φύγει

pthread_mutex_t queue_mutex;     // Mutex για ουρά αναμονής
pthread_mutex_t print_mutex;     // Mutex για thread-safe printing
sem_t waiter_sem;                // Semaphore για waiter
```

---

## Κύριες Συναρτήσεις

### 1. `main(argc, argv)`

**Παράδειγμα Κλήσης**:
```bash
./restaurant 5 20 4
```
- 5 τραπέζια
- 20 ομάδες πελατών
- 4 άτομα χωρητικότητα ανά τραπέζι

**Λειτουργία**:

#### A. Έλεγχος ορισμάτων
```c
if (argc != 4) {
    fprintf(stderr, "Usage: %s <num_tables> <num_groups> <table_capacity>\n", argv[0]);
    return 1;
}
```

#### B. Αρχικοποίηση τραπεζιών
```c
tables = (Table*)malloc(num_tables * sizeof(Table));
for (int i = 0; i < num_tables; i++) {
    tables[i].id = i;
    tables[i].capacity = table_capacity;
    tables[i].occupied = 0;
    pthread_mutex_init(&tables[i].mutex, NULL);  // Κάθε τραπέζι έχει δικό του mutex
}
```

#### C. Αρχικοποίηση ομάδων
```c
groups = (Group*)malloc(num_groups * sizeof(Group));
for (int i = 0; i < num_groups; i++) {
    groups[i].id = i;
    groups[i].size = 1 + rand() % table_capacity;  // Τυχαίο μέγεθος
    groups[i].assigned_table = -1;
    groups[i].in_queue = 0;
    sem_init(&groups[i].seated_sem, 0, 0);  // Semaphore ξεκινά 0 (locked)
}
```

#### D. Δημιουργία threads
```c
// Group threads
for (int i = 0; i < num_groups; i++) {
    pthread_create(&group_threads[i], NULL, group_thread, &groups[i]);
}

// Waiter thread
pthread_create(&waiter_tid, NULL, waiter_thread, NULL);
```

#### E. Αναμονή για ολοκλήρωση
```c
// Wait για όλες τις ομάδες
for (int i = 0; i < num_groups; i++) {
    pthread_join(group_threads[i], NULL);
}

// Ενημέρωση waiter να τερματίσει
sem_post(&waiter_sem);

// Wait για waiter
pthread_join(waiter_tid, NULL);
```

#### F. Καθαρισμός πόρων
```c
for (int i = 0; i < num_tables; i++) {
    pthread_mutex_destroy(&tables[i].mutex);
}
// ... κλπ semaphore/mutex/memory cleanup
```

---

### 2. `group_thread(void *arg)`

**Αυτή είναι η thread function για κάθε ομάδα πελατών**

**Ροή**:

#### Βήμα 1: Τυχαία καθυστέρηση άφιξης
```c
int arrival_delay = rand() % 11;  // 0-10 δευτερόλεπτα
sleep(arrival_delay);

printf("[GROUP %d] Arrived with %d people (after %d sec)\n", 
       group->id, group->size, arrival_delay);
```

#### Βήμα 2: Προσθήκη στην ουρά αναμονής
```c
pthread_mutex_lock(&queue_mutex);
group->in_queue = 1;  // Σημαία: "περιμένω τραπέζι"
pthread_mutex_unlock(&queue_mutex);
```

**Γιατί χρειάζεται mutex;**
- Ο waiter thread διαβάζει `group->in_queue` ταυτόχρονα
- Χωρίς mutex, ίσως δούμε race condition

#### Βήμα 3: Ειδοποίηση waiter
```c
sem_post(&waiter_sem);
```
- Αυξάνει το semaphore
- Ξυπνάει το waiter thread που περιμένει στο `sem_wait()`

#### Βήμα 4: Αναμονή έως τραπέζι (BLOCKING)
```c
sem_wait(&group->seated_sem);
```
- **Κρίσιμο**: Η ομάδα **σταματάει εδώ** μέχρι ο waiter να ανακοινώσει τραπέζι
- Ο waiter θα κάνει `sem_post(&group->seated_sem)` για να την ξυπνήσει

#### Βήμα 5: Κάθισμα και φαγητό
```c
printf("[GROUP %d] Seated at Table %d with %d people\n",
       group->id, group->assigned_table, group->size);

int eating_time = 5 + rand() % 11;  // 5-15 δευτερόλεπτα
sleep(eating_time);
```

#### Βήμα 6: Αποχώρηση
```c
int table_id = group->assigned_table;
pthread_mutex_lock(&tables[table_id].mutex);
tables[table_id].occupied -= group->size;  // Απελευθέρωση θέσεων
pthread_mutex_unlock(&tables[table_id].mutex);

printf("[GROUP %d] Left Table %d (%d/%d occupied)\n",
       group->id, table_id, remaining, table_capacity);
```

#### Βήμα 7: Ενημέρωση ολοκλήρωσης
```c
pthread_mutex_lock(&queue_mutex);
groups_finished++;
pthread_mutex_unlock(&queue_mutex);

sem_post(&waiter_sem);  // Ξυπνάει waiter (μπορεί να υπάρχουν άλλες ομάδες)
```

---

### 3. `waiter_thread(void *arg)`

**Αυτή είναι η thread function του σερβιτόρου που ανακατανέμει ομάδες**

**Ροή**:

#### Κύριος βρόχος
```c
while (1) {
    sem_wait(&waiter_sem);  // Περιμένει ειδοποίηση (νέα ομάδα ή ελεύθερο τραπέζι)
    
    pthread_mutex_lock(&queue_mutex);
    
    // Έλεγχος αν όλες οι ομάδες έχουν φύγει
    if (groups_finished == num_groups) {
        pthread_mutex_unlock(&queue_mutex);
        break;  // Τερματισμός του waiter
    }
```

#### Ανακατανομή ομάδων
```c
int assigned_any = 0;

do {
    assigned_any = 0;
    
    for (int g = 0; g < num_groups; g++) {
        if (groups[g].in_queue == 1) {  // Ομάδα περιμένει
            // Αναζήτηση κατάλληλου τραπεζιού
            for (int t = 0; t < num_tables; t++) {
                pthread_mutex_lock(&tables[t].mutex);
                int available = tables[t].capacity - tables[t].occupied;
                
                if (available >= groups[g].size) {
                    // ✅ Τραπέζι βρέθηκε!
                    
                    // 1. Ενημέρωση τραπεζιού
                    tables[t].occupied += groups[g].size;
                    
                    // 2. Ενημέρωση ομάδας
                    groups[g].assigned_table = t;
                    groups[g].in_queue = 0;  // Δεν περιμένει πια
                    
                    pthread_mutex_unlock(&tables[t].mutex);
                    
                    // 3. Εκτύπωση
                    printf("[WAITER] Assigned group %d (size=%d) to Table %d\n",
                           groups[g].id, groups[g].size, t);
                    
                    // 4. Ξύπνημα της ομάδας
                    sem_post(&groups[g].seated_sem);
                    
                    assigned_any = 1;
                    break;
                }
                pthread_mutex_unlock(&tables[t].mutex);
            }
            
            if (assigned_any) break;  // Επανάληψη για άλλες ομάδες
        }
    }
} while (assigned_any);  // Συνέχιση όσο υπάρχουν ανακατανομές
```

---

### 4. `print_waiting_groups()`

**Εκτυπώνει τις ομάδες που περιμένουν στην ουρά** (Debug function)

```c
void print_waiting_groups() {
    printf("[WAITER] Groups waiting: ");
    
    for (int i = 0; i < num_groups; i++) {
        if (groups[i].in_queue == 1) {
            printf("%d", groups[i].id);
            if (/* δεν είναι η τελευταία */) printf(", ");
        }
    }
    printf("\n");
}
```

---

## Συγχρονισμός - Ανάλυση

### Πρόβλημα: Race Condition

**Χωρίς Synchronization**:
```
Thread A (Group 1)              Thread B (Waiter)
                                Διαβάζει: table[0].occupied = 2
                                Βλέπει available = 2 θέσεις
Αλλάζει: table[0].occupied = 3
                                Δημιουργεί θέσεις: occupied = 2 + 2 = 4 ⚠️
```
❌ Ο αριθμός θέσεων δεν συμφωνεί με την πραγματικότητα!

### Λύση: Mutex

**Με Mutex**:
```c
pthread_mutex_lock(&tables[t].mutex);     // 🔒 Κλείδωμα

int available = tables[t].capacity - tables[t].occupied;
if (available >= groups[g].size) {
    tables[t].occupied += groups[g].size;
}

pthread_mutex_unlock(&tables[t].mutex);   // 🔓 Ξεκλείδωμα
```

### Πρόβλημα: Deadlock

**Παράδειγμα Deadlock**:
```
Thread A: lock(mutex1) → περιμένει lock(mutex2)
Thread B: lock(mutex2) → περιμένει lock(mutex1)
❌ Και τα δύο threads είναι blocked για πάντα!
```

**Στο πρόγραμμά μας**: Δεν έχουμε nested mutexes, οπότε δεν υπάρχει κίνδυνος deadlock. ✅

### Semaphores - Λογική

**Semaphore**: Ένα κλειδί με μετρητή (counter)

```
sem_wait()  → counter--; αν counter < 0, περίμενε
sem_post()  → counter++; ξύπνησε έναν thread που περιμένει
```

**Στο εστιατόριό μας**:
- `waiter_sem` ξεκινά = 0
- Όταν ομάδα φτάνει: `sem_post()` → counter = 1 → waiter ξυπνά
- Αν 5 ομάδες φτάνουν γρήγορα: counter = 5 → waiter θα δει όλες

---

## Παράδειγμα Εκτέλεσης

```bash
./restaurant 2 5 2
```

**Αναμενόμενη Εξοδος**:
```
[GROUP 0] Arrived with 1 people (after 2 sec)
[GROUP 1] Arrived with 2 people (after 5 sec)
[WAITER] Assigned group 0 (size=1) to Table 0 (1/2 occupied)
[WAITER] Groups waiting: 1
[GROUP 0] Seated at Table 0 with 1 people
[GROUP 0] Eating (1 people) for 8 seconds...
[GROUP 1] Seated at Table 1 with 2 people
[WAITER] Groups waiting: None
[GROUP 1] Eating (2 people) for 10 seconds...
[GROUP 2] Arrived with 1 people (after 7 sec)
[GROUP 2] Seated at Table 1 with 1 people  ⚠️ Περιμένει πίνακες!
...
[GROUP 0] Left Table 0 (0/2 occupied)
[GROUP 2] Left Table 1 (1/2 occupied)
All groups have left. Closing restaurant.
```

---

## Synchronization Primitives

| Primitive | Σκοπός | Λειτουργία |
|-----------|--------|-----------|
| **pthread_mutex_t** | Αμοιβαίος αποκλεισμός | lock/unlock για κρίσιμα τμήματα |
| **sem_t** | Μετρητής με blocking | wait/post για signaling |
| **pthread_cond_t** | (Δεν χρησιμοποιείται εδώ) | Αναμονή σε συνθήκες |

---

## Πιθανά Προβλήματα

### 1. Livelock
Ο waiter συνεχώς ελέγχει αλλά δεν μπορεί να ανακατανείμει (όλα τα τραπέζια γεμάτα)
- **Λύση**: Στο πρόγραμμα αυτό δεν συμβαίνει λόγω του do-while loop

### 2. Priority Inversion
Ομάδα με 1 άτομο περιμένει ενώ μεγάλη ομάδα μπροστά
- **Στο πρόγραμμα**: Ο waiter δοκιμάζει όλες τις ομάδες και τα τραπέζια, οπότε δεν είναι αγκυρωμένη

### 3. Memory Leaks
Αν κάτι αποτύχει, τα malloc'd pointers δεν απελευθερώνονται
- **Λύση**: Προσθήκη try-catch ή ελέγχου σφαλμάτων

---

## Compilation & Execution

```bash
# Compilation
make

# Execution
./restaurant 3 10 2

# Ερμηνεία: 3 τραπέζια, 10 ομάδες, χωρητικότητα 2 άτομα ανά τραπέζι
```

---

## Κριτική Σκέψη - Ερωτήσεις Διδακτικής Αξίας

### Ερώτηση 1: Γιατί χρησιμοποιούμε `sem_wait()` στο `group_thread` αλλά όχι `pthread_cond_wait()`?
**Απάντηση**:
- `sem_wait()` έχει επιπλέον **counter** που επιτρέπει "buffering" σημάτων
- Αν 5 ομάδες φτάσουν τη σημαία πριν ο waiter την επεξεργαστεί, το counter = 5
- `pthread_cond_wait()` χάνει σήματα αν δεν υπάρχει thread που περιμένει
- Semaphores είναι καλύτερα για αυτό το pattern

### Ερώτηση 2: Τι θα συμβεί αν αφαιρέσουμε το `pthread_mutex_lock(&queue_mutex)` στο waiter?
**Απάντηση**:
```
Race condition:
- Group A: groups[0].in_queue = 1
- Waiter διαβάζει: groups[0].in_queue  (ίσως = 0 τώρα!)
- Group A: groups[0].in_queue = 0
- Waiter: Σημαία δεν τίθεται σωστά!
```
❌ Το πρόγραμμα θα ήταν **broken**

### Ερώτηση 3: Ποια είναι η διαφορά μεταξύ `occupied` και `capacity`;
**Απάντηση**:
- `capacity` = **Σταθερό**: π.χ. 4 άτομα (δεν αλλάζει)
- `occupied` = **Μεταβλητό**: Πόσα άτομα κάθονται τώρα (0-4)
- Για να επιτρέψουμε μια ομάδα: `capacity - occupied >= group.size`
- Παράδειγμα: capacity=4, occupied=2 → available=2 → όχι για ομάδα 3 ατόμων

### Ερώτηση 4: Γιατί ξεκινά το `seated_sem` με 0 και όχι 1;
**Απάντηση**:
```c
sem_init(&group->seated_sem, 0, 0);  // Ξεκινά 0
```
- Αν ξεκινούσε με 1, η ομάδα θα `sem_wait()` και θα συνεχίσει **αμέσως**
- Αλλά ο waiter δεν της έχει ανακοινώσει τραπέζι ακόμα!
- Ξεκινώντας με 0, η ομάδα **σταματάει** μέχρι ο waiter να κάνει `sem_post()`

### Ερώτηση 5: Τι συμβαίνει αν 10 ομάδες τυχαία φτάσουν ταυτόχρονα;
**Απάντηση**:
```
Initial: waiter_sem = 0

Group 0: sem_post() → waiter_sem = 1
Group 1: sem_post() → waiter_sem = 2
...
Group 9: sem_post() → waiter_sem = 10

Waiter: sem_wait() → waiter_sem = 9, ξυπνά και δουλεύει
```
✅ Ο κατάλογος semaphore κρατάει όλα τα σήματα!

### Ερώτηση 6: Γιατί χρησιμοποιούμε `int in_queue` αντί απλώς `assigned_table != -1`;
**Απάντηση**:
- `in_queue = 1` σημαίνει "περιμένει στη σειρά"
- `assigned_table != -1` σημαίνει "έχει τραπέζι"
- Η διαφορά είναι κρίσιμη: Μιας ομάδας που έχει τραπέζι δεν την ενδιαφέρει το waiter πια!
- Χωρίς αυτή τη σημαία, ο waiter θα δοκιμάζει συνεχώς την ίδια ομάδα

### Ερώτηση 7: Μπορούν δύο ομάδες να κάθονται στο ίδιο τραπέζι ταυτόχρονα;
**Απάντηση**:
```c
// Τραπέζι χωρητικότητα 4 ατόμων
Table 0: capacity=4, occupied=0

Group 1 (size=2): occupied += 2 → occupied = 2
Group 3 (size=2): occupied += 2 → occupied = 4

// Αμφότερες καθήνται στο ίδιο τραπέζι!
```
✅ **ΝΑΙ**! Το πρόγραμμα δεν απαγορεύει κοινόχρηστα τραπέζια. Αυτό είναι feature όχι bug!

### Ερώτηση 8: Πώς θα προσθέταμε ουρά προτεραιότητας (π.χ. VIP ομάδες);
**Απάντηση**:
- Προσθήκη `int priority` field στο Group struct
- Αντί να διατρέχουμε τις ομάδες με τη σειρά, να τις ταξινομούμε κατά priority
- Ή χρήση priority queue:
```c
typedef struct {
    Group *groups[MAX_GROUPS];
    int size;
} PriorityQueue;

// VIP ομάδες βάζονται πρώτες
```

### Ερώτηση 9: Τι συμβαίνει στο `print_waiting_groups()` αν 100 ομάδες περιμένουν;
**Απάντηση**:
```c
// Output: [WAITER] Groups waiting: 0, 1, 2, ..., 99
// Αυτό είναι πολλές πληροφορίες και δύσκολο να διαβαστεί!
```
- Καλύτερα: Εκτύπωση μόνο αριθμού ομάδων που περιμένουν
- Ή χρήση linked list για ουρά αναμονής που ενημερώνεται σωστά

### Ερώτηση 10: Ποια είναι η χρονική πολυπλοκότητα του waiter algorithm;
**Απάντηση**:
```
Per waiter activation:
  - Διατρέχει groups: O(num_groups)
  - Για κάθε ομάδα, διατρέχει τραπέζια: O(num_tables)
  - Total: O(num_groups * num_tables) ανά επιλογή

Χειρότερη περίπτωση: όλες οι ομάδες περιμένουν → O(num_groups^2 * num_tables)
```
⚠️ Δεν είναι βέλτιστο για μεγάλα δεδομένα. Λύση: indexed data structures (hash tables)

---

## Συμπέρασμα

Η Ask2 είναι ένα **κλασικό παράδειγμα concurrent programming** που δείχνει:
- ✅ Multi-threading με POSIX threads
- ✅ Mutual exclusion με mutexes
- ✅ Inter-thread signaling με semaphores
- ✅ Race condition prevention
- ✅ Realistic simulation pattern

Ωστόσο, έχει σημαντικά κενά για production:
- ❌ Δεν υπάρχει deadlock detection
- ❌ Δεν υπάρχει priority handling
- ❌ Το buffering είναι απλό (hardcoded sizes)
- ❌ Δεν υπάρχει error handling στα threads
