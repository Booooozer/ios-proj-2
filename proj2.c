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
    sem_t *santa;
    sem_t *actionSem;
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
    sems->santa = sem_open("santa", O_CREAT, 0644, 0);
    sems->actionSem = sem_open("actionCount", O_CREAT, 0644, 1);

    return 0;
}

void semDestruct(semaphores_t *sems) {
    sem_close(sems->santa);
    sem_close(sems->actionSem);

    sem_unlink("santa");
    sem_unlink("actionCount");
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

    unsigned int *actionCnt = mmap(NULL, sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    *actionCnt = 1;

    // TODO forks fail
    // Process Santa
    if (fork() == 0) {
        sem_wait(sems->actionSem);
        flushPrint("%d: Santa: going to sleep\n", (*actionCnt)++);
        sem_post(sems->actionSem);
        exit(0);
    }

    // Process Elves
    for (int i = 0; i < params.numElves; i++) {
        if (fork() == 0) {
            flushPrint("%d: Elf %d: started\n", (*actionCnt)++, i+1);
            srand(time(NULL) * getpid());

            usleep(1000 * (random() % params.timeElf));

            flushPrint("%d: Elf %d: need help\n", (*actionCnt)++, i+1);
            exit(0);
        }
    }

    // Process Reindeers
    for (int i = 0; i < params.numReind; i++) {
        if (fork() == 0) {
            flushPrint("%d: Raindeer %d\n", (*actionCnt)++, i);
            exit(0);
        }
    }

    sleep(10);

    semDestruct(sems);
    munmap(sems, sizeof(semaphores_t));
    munmap(actionCnt, sizeof(unsigned int));
    fclose(fp);

    return 0;
}
