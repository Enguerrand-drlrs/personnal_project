#include "gameoflife.h"
#include <time.h>
/*
Rules : 
Any celles with fewer than 2 neighbourgs die (underpopulation)
Any celles with 2 or 3 neighbours live on to the next generation
Any celles with more than 3 neighbours die (overpopulation)
Any dead celles with exactly 3 neighbours become a live celle (reproduction)
*/

int main(int argc, char **argv) {
    uint16_t width  = 50;
    uint16_t height = 20;
    uint16_t gens   = 100;

    srand(time(NULL));

    LifeWorld *w = life_create(width, height);
    if (!w) {
        fprintf(stderr, "Memory allocation failed!\n");
        return 1;
    }

    life_randomize(w);

    FILE *out = fopen("life_output.txt", "w");
    if (!out) {
        perror("Error creating output file");
        life_destroy(w);
        return 1;
    }

    for (uint16_t g = 0; g < gens; g++) {
        life_save(out, w, g);
        life_step(w);
    }

    fclose(out);
    life_destroy(w);

    printf("Simulation completed — generations saved to life_output.txt\n");
    return 0;
}
