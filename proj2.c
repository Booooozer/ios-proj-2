#include <stdio.h>
#include <stdlib.h>


typedef struct {
    int numElves;
    int numRaind;
    int timeElf;
    int timeRaind;
}params_t;

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
                params->numRaind = tmp;
                break;
            case 3:
                params->timeElf = tmp;
                break;
            case 4:
                params->timeRaind = tmp;
                break;
            default:
                break;
        }
    }

    return 0;
}


int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Invalid input parameters\n");
        return 1;
    }

    params_t params;
    if (parseParams(argv, &params) != 0 ) {
        fprintf(stderr, "Input parameter is not a number\n");
        return 1;
    }


    return 0;
}
