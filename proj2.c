#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <stdarg.h>

// mmap, munmap
#include <sys/mman.h>

// struct for input parameters
typedef struct {
    int numElves;
    int numReind;
    int timeElf;
    int timeReind;
}params_t;

// struct for semaphores
typedef struct {
    sem_t *santaSem;
    sem_t *actionSem;
    sem_t *elfTex;
    sem_t *reindTex;
}semaphores_t;

int parseParams(char *argv[], params_t *params) {
    char *ptr = NULL;
    int tmp;
    for (int i = 1; i < 5; i++) {
        tmp = (int)strtol(argv[i], &ptr, 10);
        if (tmp <= 0 || ptr[0] != '\0') {
            // couldn't convert parameter
            return -1;
        }
        switch (i) {
            case 1:
                params->numElves = tmp;
                break;
            case 2:
                params->numReind = tmp;
                break;
            case 3:
                params->timeElf = tmp;
                break;
            case 4:
                params->timeReind = tmp;
                break;
            default:
                break;
        }
    }

    return 0;
}

void flushPrint(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    fflush(stdout);
}


// Initializes semaphores
int semInit(semaphores_t *sems) {
    // TODO check if sem_open fails
    sems->santaSem = sem_open("santaSem", O_CREAT, 0644, 0);    // keeps santa sleeping
    sems->actionSem = sem_open("actionCount", O_CREAT, 0644, 1);    // used to access shared memory
    sems->elfTex = sem_open("elfTex", O_CREAT, 0644, 1);    // block elves, only one at a time can enter help tree
    sems->reindTex = sem_open("reindTex", O_CREAT, 0644, 0);    // block reindeer until last one comes back
    return 0;
}

void semDestruct(semaphores_t *sems) {
    sem_close(sems->santaSem);
    sem_close(sems->actionSem);
    sem_close(sems->elfTex);
    sem_close(sems->reindTex);

    sem_unlink("santaSem");
    sem_unlink("actionCount");
    sem_unlink("elfTex");
    sem_unlink("reindTex");
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Invalid input parameters\n");
        return 1;
    }

    params_t params;
    if (parseParams(argv, &params) != 0) {
        fprintf(stderr, "Input parameter is not a number\n");
        return 1;
    }

    FILE *fp = fopen("./proj2.out", "w");

    // make shared memory for sem struct and initialize
    semaphores_t *sems = mmap(NULL, sizeof(semaphores_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    semInit(sems);

    // shared counter to enumerate actions in output
    unsigned int *actionCnt = mmap(NULL, sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    *actionCnt = 1;
    // shared counter of elves waiting for santa's help
    unsigned int *elves = mmap(NULL, sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    // shared counter of reindeer who came back from holidays
    unsigned int *reindeer = mmap(NULL, sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    *elves = 0;
    *reindeer = 0;

    srand(time(NULL) * getpid());

    // TODO forks fail
    // Process Santa
    if (fork() == 0) {
        while (1) {
            // santa is sleeping
            sem_wait(sems->actionSem);
            flushPrint("%d: Santa: going to sleep\n", (*actionCnt)++);
            sem_post(sems->actionSem);

            sem_wait(sems->santaSem);
            sem_wait(sems->actionSem);
            if ((*reindeer) >= 9) {
                // start christmas
                for (int i = 0; i < 9; i++) {
                    sem_post(sems->reindTex);

                    // ...

                }
            } else if ((*elves) == 3) {
                // help elves
                exit(0);
            }
            sem_post(sems->actionSem);
        }
    }

    // Process Elves
    for (int i = 0; i < params.numElves; i++) {
        if (fork() == 0) {
            sem_wait(sems->actionSem);
            flushPrint("%d: Elf %d: started\n", (*actionCnt)++, i+1);
            sem_post(sems->actionSem);

            // elf is working
            usleep(1000 * (random() % params.timeElf));

            // need help
            sem_wait(sems->elfTex);
            sem_wait(sems->actionSem);
            flushPrint("%d: Elf %d: need help\n", (*actionCnt)++, i+1);
            *elves += 1;
            if ((*elves) == 3) {
                // wake up santa
                sem_post(sems->santaSem);
            } else {
                // wait in a queue for more elves
                sem_post(sems->elfTex);
            }
            sem_post(sems->actionSem);
            exit(0);
        }
    }

    // Process Reindeer
    for (int i = 0; i < params.numReind; i++) {
        if (fork() == 0) {
            sem_wait(sems->actionSem);
            flushPrint("%d: RD %d: rstarted\n", (*actionCnt)++, i);
            sem_post(sems->actionSem);

            // Reindeer on holidays
            usleep(1000 * ((params.numReind / 2) + (random() % (params.timeReind / 2))));

            // Reindeer came back
            sem_wait(sems->actionSem);
            flushPrint("%d: RD %d return home", (*actionCnt)++, i);
            *reindeer += 1;
            if ((*reindeer) == 9) {
                // all are back, wake up santa to start christmas
                sem_post(sems->santaSem);
            }
            sem_post(sems->actionSem);

            // waiting for last reindeer
            sem_wait(sems->reindTex);

            // all reindeer are here, get hitched
            sem_wait(sems->actionSem);
            flushPrint("%d: RD %d: get hitched", (*actionCnt)++, i);
            sem_post(sems->actionSem);

            exit(0);
        }
    }

    sleep(10);

    // destroy semaphores and free shared memory
    semDestruct(sems);
    munmap(sems, sizeof(semaphores_t));
    munmap(actionCnt, sizeof(unsigned int));
    munmap(elves, sizeof(unsigned int));
    munmap(reindeer, sizeof(unsigned int));
    fclose(fp);

    return 0;
}
