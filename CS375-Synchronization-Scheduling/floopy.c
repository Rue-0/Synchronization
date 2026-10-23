#include <stdio.h>
#include <pthread.h>

typedef struct account_t {
    pthread_mutex_t lock;
    float balance;
    long uuid;
} account_t;

typedef struct {
    account_t *donor;
    account_t *recipient;
    float amount;
} transfer_args;

void transfer(account_t *donor, account_t *recipient, float amount) {
    account_t *first =
        (donor->uuid < recipient->uuid) ? donor : recipient;
    account_t *second =
        (donor->uuid < recipient->uuid) ? recipient : donor;

    pthread_mutex_lock(&first->lock);
    pthread_mutex_lock(&second->lock);

    if (donor->balance < amount) {
        printf("Insufficient funds.\n");
    } else {
        donor->balance -= amount;
        recipient->balance += amount;

        printf(
            "Transferred %.2f from account %ld to %ld\n",
            amount,
            donor->uuid,
            recipient->uuid
        );
    }

    pthread_mutex_unlock(&second->lock);
    pthread_mutex_unlock(&first->lock);
}

void* transfer_thread(void* arg) {
    transfer_args *args = (transfer_args*)arg;

    transfer(
        args->donor,
        args->recipient,
        args->amount
    );

    return NULL;
}

int main() {
    account_t acc1 = {
        PTHREAD_MUTEX_INITIALIZER,
        1000,
        1
    };

    account_t acc2 = {
        PTHREAD_MUTEX_INITIALIZER,
        500,
        2
    };

    transfer_args args1 = {
        &acc1,
        &acc2,
        200
    };

    transfer_args args2 = {
        &acc2,
        &acc1,
        100
    };

    pthread_t t1, t2;

    pthread_create(
        &t1,
        NULL,
        transfer_thread,
        &args1
    );

    pthread_create(
        &t2,
        NULL,
        transfer_thread,
        &args2
    );

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Final balance account 1: %.2f\n", acc1.balance);
    printf("Final balance account 2: %.2f\n", acc2.balance);

    pthread_mutex_destroy(&acc1.lock);
    pthread_mutex_destroy(&acc2.lock);

    return 0;
}
