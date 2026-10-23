#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>

int numBatterInBowl = 0;
int numEggInBowl = 0;
bool readyToEat = false;

pthread_mutex_t lock;
pthread_cond_t needIngredients;
pthread_cond_t readyToBake;
pthread_cond_t startEating;

void addBatter() {
    numBatterInBowl++;
    printf("Batter added to bowl.\n");
}

void addEgg() {
    numEggInBowl++;
    printf("Egg added to bowl. Eggs: %d\n", numEggInBowl);
}

void heatBowl() {
    printf("Cake is baking...\n");

    readyToEat = true;
    numBatterInBowl = 0;
    numEggInBowl = 0;
}

void eatCake() {
    printf("Cake is ready and being eaten!\n");
    readyToEat = false;
}

void* batterAdder(void* arg) {
    pthread_mutex_lock(&lock);

    while (numBatterInBowl >= 1 || readyToEat) {
        pthread_cond_wait(
            &needIngredients,
            &lock
        );
    }

    addBatter();

    pthread_cond_broadcast(&readyToBake);

    pthread_mutex_unlock(&lock);

    return NULL;
}

void* eggBreaker(void* arg) {
    pthread_mutex_lock(&lock);

    while (readyToEat) {
        pthread_cond_wait(
            &needIngredients,
            &lock
        );
    }

    while (numEggInBowl < 2) {
        addEgg();
    }

    pthread_cond_broadcast(&readyToBake);

    pthread_mutex_unlock(&lock);

    return NULL;
}

void* bowlHeater(void* arg) {
    pthread_mutex_lock(&lock);

    while (
        numBatterInBowl < 1 ||
        numEggInBowl < 2 ||
        readyToEat
    ) {
        pthread_cond_wait(
            &readyToBake,
            &lock
        );
    }

    heatBowl();

    pthread_cond_signal(&startEating);

    pthread_mutex_unlock(&lock);

    return NULL;
}

void* cakeEater(void* arg) {
    pthread_mutex_lock(&lock);

    while (!readyToEat) {
        pthread_cond_wait(
            &startEating,
            &lock
        );
    }

    eatCake();

    pthread_cond_broadcast(&needIngredients);

    pthread_mutex_unlock(&lock);

    return NULL;
}

int main() {
    pthread_t batter;
    pthread_t egg;
    pthread_t heater;
    pthread_t eater;

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&needIngredients, NULL);
    pthread_cond_init(&readyToBake, NULL);
    pthread_cond_init(&startEating, NULL);

    pthread_create(
        &batter,
        NULL,
        batterAdder,
        NULL
    );

    pthread_create(
        &egg,
        NULL,
        eggBreaker,
        NULL
    );

    pthread_create(
        &heater,
        NULL,
        bowlHeater,
        NULL
    );

    pthread_create(
        &eater,
        NULL,
        cakeEater,
        NULL
    );

    pthread_join(batter, NULL);
    pthread_join(egg, NULL);
    pthread_join(heater, NULL);
    pthread_join(eater, NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&needIngredients);
    pthread_cond_destroy(&readyToBake);
    pthread_cond_destroy(&startEating);

    return 0;
}
