#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/wait.h>


#define sharedMemDestruct munmap(sems, sizeof(semaphores_t));   \
            munmap(actionCnt, sizeof(int));                     \
            munmap(elves, sizeof(int));                         \
            munmap(reindeer, sizeof(int));                      \
            munmap(closed, sizeof(int));                        \
            munmap(sems, sizeof(semaphores_t));

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
    sem_t *startXmas;
    sem_t *waitHelp;
    sem_t *santaHelping;
}semaphores_t;

int parseParams(char *argv[], params_t *params) {
    char *ptr = NULL;
    int tmp;
    for (int i = 1; i < 5; i++) {
        tmp = (int)strtol(argv[i], &ptr, 10);
        if (tmp < 0 || ptr[0] != '\0') {
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
                params->timeElf = tmp + 1;
                break;
            case 4:
                params->timeReind = tmp + 1;
                break;
            default:
                break;
        }
    }

    return 0;
}

void flushPrint(FILE *fp, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);
    fflush(fp);
}


// Initializes semaphores
int semInit(semaphores_t *sems) {
    // TODO check if sem_open fails
    sems->santaSem = sem_open("santaSem", O_CREAT, 0644, 0);    // keeps santa sleeping
    sems->actionSem = sem_open("actionCount", O_CREAT, 0644, 1);    // used to access shared memory
    sems->elfTex = sem_open("elfTex", O_CREAT, 0644, 1);    // block elves, only one at a time can enter help tree
    sems->reindTex = sem_open("reindTex", O_CREAT, 0644, 0);    // block reindeer until last one comes back
    sems->startXmas = sem_open("startXmas", O_CREAT, 0644, 0);   // tell santa when last reindeer is hitched
    sems->waitHelp = sem_open("waitHelp", O_CREAT, 0644, 0);   // elves are waiting for help
    sems->santaHelping = sem_open("santaHelping", O_CREAT, 0644, 0);   // elves are waiting for help
    return 0;
}

void semDestruct(semaphores_t *sems) {
    sem_close(sems->santaSem);
    sem_close(sems->actionSem);
    sem_close(sems->elfTex);
    sem_close(sems->reindTex);
    sem_close(sems->startXmas);
    sem_close(sems->waitHelp);
    sem_close(sems->santaHelping);

    sem_unlink("santaSem");
    sem_unlink("actionCount");
    sem_unlink("elfTex");
    sem_unlink("reindTex");
    sem_unlink("startXmas");
    sem_unlink("waitHelp");
    sem_unlink("santaHelping");
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
    int *actionCnt = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    *actionCnt = 1;
    // shared counter of elves waiting for santa's help
    int *elves = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    // shared counter of reindeer who came back from holidays
    int *reindeer = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    int *closed = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    *elves = 0;
    *reindeer = 0;
    *closed = 0;

    srand(time(NULL) * getpid());

    // TODO forks fail
    // Process Santa
    pid_t santa = fork();
    if (santa == 0) {
        while (1) {
            // santa is sleeping
            sem_wait(sems->actionSem);
            flushPrint(fp, "%d: Santa: going to sleep\n", (*actionCnt)++);
            sem_post(sems->actionSem);

            sem_wait(sems->santaSem);
            sem_wait(sems->actionSem);
            if ((*reindeer) >= params.numReind) {
                // starting christmas
                flushPrint(fp, "%d: Santa: closing workshop\n", (*actionCnt)++);
                *closed = 1;
                sem_post(sems->actionSem);
                for (int i = 0; i < params.numReind; i++) {
                    sem_post(sems->reindTex);
                }
                // wait for all reindeer to get hitched
                sem_wait(sems->startXmas);
                sem_wait(sems->actionSem);
                flushPrint(fp, "%d: Santa: Christmas started\n", (*actionCnt)++);
                sem_post(sems->actionSem);

                for (int i = 0; i < params.numElves; i++) {
                    sem_post(sems->elfTex);
                    sem_post(sems->waitHelp);
                }

                exit(0);
            } else if ((*elves) >= 3) {
                // help elves
                flushPrint(fp, "%d: Santa: helping elves\n", (*actionCnt)++);
                for (int i = 0; i < 3; i++) {
                    sem_post(sems->waitHelp);
                }
                sem_post(sems->actionSem);
                sem_wait(sems->santaHelping);
            }
        } // Santa while loop
    } else if (santa < 0) {
        fprintf(stderr, "Failed to create Santa process\n");
        semDestruct(sems);
        sharedMemDestruct
        fclose(fp);
        return 1;
    }

    // Process Elves
    for (int i = 0; i < params.numElves; i++) {
        pid_t elf = fork();
        if (elf == 0) {
            sem_wait(sems->actionSem);
            flushPrint(fp, "%d: Elf %d: started\n", (*actionCnt)++, i+1);
            sem_post(sems->actionSem);

            while (1) {
                // elf is working
                usleep(1000 * (random() % params.timeElf));
                // need help
                sem_wait(sems->elfTex);
                sem_wait(sems->actionSem);
                flushPrint(fp, "%d: Elf %d: need help\n", (*actionCnt)++, i+1);
                *elves += 1;
                if ((*elves) == 3) {
                    // wake up santa
                    sem_post(sems->santaSem);
                } else {
                    sem_post(sems->elfTex);
                }

                sem_post(sems->actionSem);
                // wait for santa's help
                sem_wait(sems->waitHelp);

                sem_wait(sems->actionSem);
                // received santa's help
                if ((*closed) == 1) {
                    flushPrint(fp, "%d: Elf %d: taking holidays\n", (*actionCnt)++, i+1);
                    sem_post(sems->actionSem);
                    exit(0);
                }
                flushPrint(fp, "%d: Elf %d: get help\n", (*actionCnt)++, i+1);
                (*elves)--;
                if ((*elves) == 0) {
                    sem_post(sems->santaHelping);
                    sem_post(sems->elfTex);
                }
                sem_post(sems->actionSem);


            } // Elf while loop
        } else if (elf < 0) {
            fprintf(stderr, "Failed to create Elf process\n");
            semDestruct(sems);
            sharedMemDestruct
            fclose(fp);
            return 1;
        }
    }

    // Process Reindeer
    for (int i = 0; i < params.numReind; i++) {
        pid_t reind = fork();
        if (reind == 0) {
            sem_wait(sems->actionSem);
            flushPrint(fp, "%d: RD %d: rstarted\n", (*actionCnt)++, i+1);
            sem_post(sems->actionSem);

            // Reindeer on holidays
            //usleep(1000 * ((params.timeReind / 2) + (random() % (params.timeReind / 2))));
            usleep(1000 * (random() % params.timeReind));

            // Reindeer came back
            sem_wait(sems->actionSem);
            flushPrint(fp, "%d: RD %d: return home\n", (*actionCnt)++, i+1);
            *reindeer += 1;
            if ((*reindeer) == params.numReind) {
                // all are back, wake up santa to start christmas
                sem_post(sems->santaSem);
            }
            sem_post(sems->actionSem);

            // waiting for last reindeer
            sem_wait(sems->reindTex);

            // all reindeer are here, get hitched
            sem_wait(sems->actionSem);
            flushPrint(fp, "%d: RD %d: get hitched\n", (*actionCnt)++, i+1);
            (*reindeer)--;
            if ((*reindeer) == 0) {
                // last reindeer hitched
                sem_post(sems->startXmas);
            }
            sem_post(sems->actionSem);

            exit(0);
        } else if (reind < 0) {
            fprintf(stderr, "Failed to create Reindeer process\n");
            semDestruct(sems);
            sharedMemDestruct
            fclose(fp);
            return 1;
        }
    }

    // wait for children to die
    while(wait(NULL) > 0);

    // destroy semaphores and free shared memory
    semDestruct(sems);
    sharedMemDestruct
    fclose(fp);

    return 0;
}
