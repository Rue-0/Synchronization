#include <stdio.h>
#include <pthread.h>

typedef struct account_t {
    pthread_mutex_t lock;
    float balance;
    long uuid;
    int priority;
} account_t;

typedef struct {
    account_t *donor;
    account_t *recipient;
    float amount;
    int priority;
} transfer_args;

void transfer(
    account_t *donor,
    account_t *recipient,
    float amount,
    int thread_priority
) {
    account_t *first =
        (donor->uuid < recipient->uuid)
        ? donor
        : recipient;

    account_t *second =
        (donor->uuid < recipient->uuid)
        ? recipient
        : donor;

    pthread_mutex_lock(&first->lock);

    if (first->priority < thread_priority) {
        printf(
            "Priority donated to account %ld: %d\n",
            first->uuid,
            thread_priority
        );

        first->priority = thread_priority;
    }

    pthread_mutex_lock(&second->lock);

    if (second->priority < thread_priority) {
        printf(
            "Priority donated to account %ld: %d\n",
            second->uuid,
            thread_priority
        );

        second->priority = thread_priority;
    }

    if (donor->balance < amount) {
        printf("Insufficient funds.\n");
    } else {
        donor->balance -= amount;
        recipient->balance += amount;

        printf(
            "Priority %d transferred %.2f from account %ld to %ld\n",
            thread_priority,
            amount,
            donor->uuid,
            recipient->uuid
        );
    }

    second->priority = 0;
    first->priority = 0;

    pthread_mutex_unlock(&second->lock);
    pthread_mutex_unlock(&first->lock);
}

void* transfer_thread(void* arg) {
    transfer_args *params =
        (transfer_args*)arg;

    transfer(
        params->donor,
        params->recipient,
        params->amount,
        params->priority
    );

    return NULL;
}

int main() {
    account_t acc1 = {
        PTHREAD_MUTEX_INITIALIZER,
        1000,
        1,
        0
    };

    account_t acc2 = {
        PTHREAD_MUTEX_INITIALIZER,
        500,
        2,
        0
    };

    transfer_args high = {
        &acc1,
        &acc2,
        200,
        2
    };

    transfer_args low = {
        &acc2,
        &acc1,
        100,
        1
    };

    pthread_t t1, t2;

    pthread_create(
        &t1,
        NULL,
        transfer_thread,
        &high
    );

    pthread_create(
        &t2,
        NULL,
        transfer_thread,
        &low
    );

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf(
        "Final balances: %.2f and %.2f\n",
        acc1.balance,
        acc2.balance
    );

    return 0;
}
