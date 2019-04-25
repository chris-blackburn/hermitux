// example inspired by Ulrich Drepper's "Futexes Are Tricky"
#include <stdio.h>
#include <stdatomic.h>
#include <unistd.h>
#include "futex.h"
#include <sys/syscall.h>
#include <sys/time.h>
#include <pthread.h>

#define NTHREADS 4

// Wrapper for the futex system call
static inline int futex(int* uaddr, int futex_op, int val,
	const struct timespec* timeout, int* uaddr2, int val3) {
	syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}


#define UNLOCKED 0
#define LOCKED 1
#define CONTENDED 2

typedef int mutex_t;
static mutex_t mutex = UNLOCKED;

// Here, we want to acquire a lock.
static void lock(mutex_t* m) {

	
	// Attempt to acquire the lock with an atomic instruction
	int exp = UNLOCKED;
	int worked = atomic_compare_exchange_strong(m, &exp, LOCKED);

	// If we did not acquire the lock, then call futex to wait on it
	if (!worked) {
		
		// Update the status to contended if it was only locked
		if (exp == LOCKED) {
			exp = atomic_exchange(m, CONTENDED);
		}

		// while we don't have the lock, wait. If we wake up, exchange the
		// value to be contended (also make sure we got the lock).
		while (exp != UNLOCKED) {
			futex(m, FUTEX_WAIT | FUTEX_PRIVATE, LOCKED, NULL, NULL, 0);
			exp = atomic_exchange(m, CONTENDED);
		}
	}
}

// We will release the lock here
static void unlock(mutex_t* m) {

	// If the lock was contended in, then we need to atomically
	// exchange the value in the mutex. If it turns out that the
	// lock became contended, we need to wake the waiters that
	// tried to grab it. Otherwise, we can just return.
	if (atomic_exchange(m, UNLOCKED) != UNLOCKED) {

		// The call below tells the kernel to wake up at most 1 waiter.
		futex(m, FUTEX_WAKE | FUTEX_PRIVATE, 1, NULL, NULL, 0);
	}
}

// ******** Beginning of multithreaded counter program ********
static unsigned counter = 0;
static void* inc(void* arg) {
	printf("Thread started\n");

	int i;
	for (i = 0; i < 1000000; i++) {
		lock(&mutex);
		counter++;
		unlock(&mutex);
	}
}

int main(int argc, char** argv) {
	pthread_t threads[NTHREADS];

	// spin off threads
	int i;
	for (i = 0; i < NTHREADS; i++) {
		if (pthread_create(&threads[i], NULL, inc, NULL)) {
			return 1;
		}
	}

	// Join all threads
	for (i = 0; i < NTHREADS; i++) {
		pthread_join(threads[i], NULL);
	}

	// Print the result
	printf("Result: %d\n", counter);

	return 0;
}
