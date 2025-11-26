#include "gameoflife.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Helper to convert timespec to milliseconds
static double timespec_to_ms(struct timespec ts) {
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

// Helper to subtract two timespecs and return result in milliseconds
static double timespec_diff_ms(struct timespec start, struct timespec end) {
    struct timespec result;
    if (end.tv_nsec < start.tv_nsec) {
        result.tv_sec = end.tv_sec - start.tv_sec - 1;
        result.tv_nsec = end.tv_nsec + 1000000000L - start.tv_nsec;
    } else {
        result.tv_sec = end.tv_sec - start.tv_sec;
        result.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return timespec_to_ms(result);
}
/*
Rules : 
Any celles with fewer than 2 neighbourgs die (underpopulation)
Any celles with 2 or 3 neighbours live on to the next generation
Any celles with more than 3 neighbours die (overpopulation)
Any dead celles with exactly 3 neighbours become a live celle (reproduction)
*/

int main(int argc, char **argv) {
    // Default values matching the example
    uint16_t width  = 320;
    uint16_t height = 240;
    uint32_t gens   = 500;
    BoundaryMode boundary = BOUNDARY_TORUS;
    const char *infile = "glider.txt";
    const char *outfile = "result.txt";
    double target_hz = 60.0;

    // Simple argument parsing (long options)
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            width = (uint16_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
            height = (uint16_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "--gens") == 0 && i + 1 < argc) {
            gens = (uint32_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "--boundary") == 0 && i + 1 < argc) {
            const char *b = argv[++i];
            if (strcmp(b, "edge") == 0) boundary = BOUNDARY_EDGE;
            else if (strcmp(b, "torus") == 0) boundary = BOUNDARY_TORUS;
            else if (strcmp(b, "mirror") == 0) boundary = BOUNDARY_MIRROR;
            else if (strcmp(b, "alive") == 0) boundary = BOUNDARY_ALIVE;
            else {
                fprintf(stderr, "Unknown boundary: %s\n", b);
                return 1;
            }
        } else if (strcmp(argv[i], "--in") == 0 && i + 1 < argc) {
            infile = argv[++i];
        } else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            outfile = argv[++i];
        } else if (strcmp(argv[i], "--target-hz") == 0 && i + 1 < argc) {
            target_hz = atof(argv[++i]);
            if (target_hz <= 0) target_hz = 60.0;
        } else {
            fprintf(stderr, "Unknown or malformed argument: %s\n", argv[i]);
            return 1;
        }
    }

    srand((unsigned)time(NULL));

    LifeWorld *w = life_create(width, height);
    if (w == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        return 1;
    }
    w->boundary = boundary;

    // Print runtime configuration for debugging
    const char *bname = "unknown";
    switch (boundary) {
        case BOUNDARY_EDGE: bname = "edge"; break;
        case BOUNDARY_TORUS: bname = "torus"; break;
        case BOUNDARY_MIRROR: bname = "mirror"; break;
        case BOUNDARY_ALIVE: bname = "alive"; break;
    }
    printf("Configuration: %ux%u, gens=%u, boundary=%s, in=%s, out=%s, target_hz=%.2f\n",
           width, height, gens, bname, infile, outfile, target_hz);

    // Load pattern (or randomize on failure)
    if (life_load_pattern(w, infile) != 0) {
        fprintf(stderr, "Failed to load pattern '%s' — using random init\n", infile);
        life_randomize(w);
    }

    // Display loop: update at the requested target_hz and show in-place
    double target_ms = 1000.0 / target_hz;
    struct timespec t_start, t_end, t_frame;

    life_clear_screen(); // clear once so subsequent `\x1b[H` draws are clean
    
    for (uint32_t g = 0; g < gens; g++) {        
        clock_gettime(CLOCK_MONOTONIC, &t_start);

        life_print(w, (uint16_t)g);
        life_step(w);

        clock_gettime(CLOCK_MONOTONIC, &t_end);
        double elapsed_ms = timespec_diff_ms(t_start, t_end);
        double to_sleep_ms = target_ms - elapsed_ms;
        if (to_sleep_ms > 0) {
            t_frame.tv_sec = (time_t)(to_sleep_ms / 1000.0);
            t_frame.tv_nsec = (long)((to_sleep_ms - t_frame.tv_sec * 1000.0) * 1000000.0);
            nanosleep(&t_frame, NULL);
        }
    }

    // Save final grid to output file
    FILE *out = fopen(outfile, "w");
    if (out == NULL) {
        perror("Error creating output file");
        life_destroy(w);
        return 1;
    }
    life_save_final(out, w);
    fclose(out);
    life_destroy(w);

    printf("Simulation completed — final grid saved to %s\n", outfile);
    return 0;
}