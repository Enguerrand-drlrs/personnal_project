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
    uint16_t height = 50;
    uint16_t gens   = 10;

    srand(time(NULL));

    LifeWorld *w = life_create(width, height);
    if (w == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        return 1;
    }

    // Grid is initialized to all dead cells (no random initialization)
    // Only the pattern from glider.txt will be alive
    
    // Load a pattern from glider.txt and place it at the center:
    if (life_load_pattern(w, "glider.txt") != 0) {
        fprintf(stderr, "Failed to load pattern (continuing with random init)\n");
        life_randomize(w);
    }

    FILE *out = fopen("life_output.txt", "w");
    if (out == NULL) {
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
