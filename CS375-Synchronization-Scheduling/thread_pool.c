#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#define NUM_THREADS 4
#define NUM_TASKS 10

typedef struct {
    void (*task)(int);
    int arg;
} Task;

typedef struct {
    Task *queue;
    int head;
    int tail;
    int count;
    int size;
    bool shutdown;

    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} ThreadSafeQueue;

void queue_init(ThreadSafeQueue *q, int size) {
    q->queue = malloc(sizeof(Task) * size);
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->size = size;
    q->shutdown = false;

    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
}

void queue_push(ThreadSafeQueue *q, Task task) {
    pthread_mutex_lock(&q->lock);

    while (q->count == q->size) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }

    q->queue[q->tail] = task;
    q->tail = (q->tail + 1) % q->size;
    q->count++;

    pthread_cond_signal(&q->not_empty);

    pthread_mutex_unlock(&q->lock);
}

int queue_pop(ThreadSafeQueue *q, Task *task) {
    pthread_mutex_lock(&q->lock);

    while (q->count == 0 && !q->shutdown) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }

    if (q->count == 0 && q->shutdown) {
        pthread_mutex_unlock(&q->lock);
        return 0;
    }

    *task = q->queue[q->head];

    q->head = (q->head + 1) % q->size;
    q->count--;

    pthread_cond_signal(&q->not_full);

    pthread_mutex_unlock(&q->lock);

    return 1;
}

void* worker(void* arg) {
    ThreadSafeQueue *q = (ThreadSafeQueue*)arg;

    while (1) {
        Task task;

        if (!queue_pop(q, &task)) {
            break;
        }

        task.task(task.arg);
    }

    return NULL;
}

void sample_task(int arg) {
    printf(
        "Task executed with arg: %d by thread %lu\n",
        arg,
        (unsigned long)pthread_self()
    );
}

int main() {
    ThreadSafeQueue q;

    queue_init(&q, NUM_TASKS);

    pthread_t threads[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(
            &threads[i],
            NULL,
            worker,
            &q
        );
    }

    for (int i = 0; i < NUM_TASKS; i++) {
        Task task = {
            sample_task,
            i
        };

        queue_push(&q, task);
    }

    pthread_mutex_lock(&q.lock);

    while (q.count > 0) {
        pthread_mutex_unlock(&q.lock);
        sched_yield();
        pthread_mutex_lock(&q.lock);
    }

    q.shutdown = true;

    pthread_cond_broadcast(&q.not_empty);

    pthread_mutex_unlock(&q.lock);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&q.lock);
    pthread_cond_destroy(&q.not_empty);
    pthread_cond_destroy(&q.not_full);

    free(q.queue);

    return 0;
}
