#include "gameoflife.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
/*
Rules : 
Any cell with fewer than 2 neighbourgs die (underpopulation)
Any cell with 2 or 3 neighbours live on to the next generation
Any cell with more than 3 neighbours die (overpopulation)
Any dead cell with exactly 3 neighbours become a live cell (reproduction)
*/
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

int main(int argc, char **argv) {
    // Default values matching the example
    uint16_t width  = 320;
    uint16_t height = 240;
    uint32_t gens   = 500;
    BoundaryMode boundary = BOUNDARY_TORUS;
    const char *infile = "glider.txt";
    const char *outfile = "result.txt";
    double target_hz = 60.0;
    int render_scale = 1;

    bool no_render = false;
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
        } else if (strcmp(argv[i], "--no-render") == 0) {
            no_render = true;
        } else if (strcmp(argv[i], "--render-scale") == 0 && i + 1 < argc) {
            render_scale = atoi(argv[++i]);
            if (render_scale < 1) render_scale = 1;
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

    // Apply render scale to the renderer
    extern int life_render_scale; // from gameoflife.c
    life_render_scale = render_scale;

    // Print runtime configuration for debugging (unless --no-render)
    if (!no_render) {
        const char *bname = "unknown";
        switch (boundary) {
            case BOUNDARY_EDGE: bname = "edge"; break;
            case BOUNDARY_TORUS: bname = "torus"; break;
            case BOUNDARY_MIRROR: bname = "mirror"; break;
            case BOUNDARY_ALIVE: bname = "alive"; break;
        }
        printf("Configuration: %ux%u, gens=%u, boundary=%s, in=%s, out=%s, target_hz=%.2f\n",
               width, height, gens, bname, infile, outfile, target_hz);
    }

    // Load pattern (or randomize on failure)
    if (life_load_pattern(w, infile) != 0) {
        fprintf(stderr, "Failed to load pattern '%s' — using random init\n", infile);
        life_randomize(w);
    }

    // Display loop: update at the requested target_hz and show in-place
    double target_ms = 1000.0 / target_hz;
    struct timespec t_frame_start, t_step_start, t_step_end, t_sleep;

    if (!no_render) life_clear_screen(); // clear once so subsequent `\x1b[H` draws are clean

    // Timing accumulators (milliseconds)
    double step_total = 0.0, step_min = 1e9, step_max = 0.0;
    double frame_total = 0.0, frame_min = 1e9, frame_max = 0.0;

    for (uint32_t g = 0; g < gens; g++) {
        // Frame work start (render + compute)
        clock_gettime(CLOCK_MONOTONIC, &t_frame_start);

        if (!no_render) life_print(w, (uint16_t)g);

        // Measure compute time (life_step)
        clock_gettime(CLOCK_MONOTONIC, &t_step_start);
        life_step(w);
        clock_gettime(CLOCK_MONOTONIC, &t_step_end);

        double step_ms = timespec_diff_ms(t_step_start, t_step_end);
        step_total += step_ms;
        if (step_ms < step_min) step_min = step_ms;
        if (step_ms > step_max) step_max = step_ms;

        // Frame work time = time from frame start to end of compute/render
        double frame_work_ms = timespec_diff_ms(t_frame_start, t_step_end);
        frame_total += frame_work_ms;
        if (frame_work_ms < frame_min) frame_min = frame_work_ms;
        if (frame_work_ms > frame_max) frame_max = frame_work_ms;

        // Sleep to respect target refresh (only when rendering)
        if (!no_render) {
            double to_sleep_ms = target_ms - frame_work_ms;
            if (to_sleep_ms > 0) {
                t_sleep.tv_sec = (time_t)(to_sleep_ms / 1000.0);
                t_sleep.tv_nsec = (long)((to_sleep_ms - t_sleep.tv_sec * 1000.0) * 1000000.0);
                nanosleep(&t_sleep, NULL);
            }
        }
    }

    // Compute averages and jitter
    double step_avg = step_total / (double)gens;
    double step_worst = step_max;
    double step_jitter = step_worst - step_avg;

    double frame_avg = frame_total / (double)gens;
    double frame_worst = frame_max;
    double frame_jitter = frame_worst - frame_avg;

    // Print timing summary
    printf("\nTiming summary (ms) over %u generations:\n", gens);
    printf("- Compute (life_step): avg = %.6f, worst = %.6f, jitter = %.6f\n", step_avg, step_worst, step_jitter);
    if (!no_render) {
        printf("- Frame work (render+compute): avg = %.6f, worst = %.6f, jitter = %.6f\n", frame_avg, frame_worst, frame_jitter);
        printf("- Target frame time: %.3f ms (%.2f Hz)\n", target_ms, target_hz);
        if (frame_avg <= target_ms) printf("- Verdict: PASS (avg frame work <= target)\n");
        else printf("- Verdict: FAIL (avg frame work > target)\n");
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

    if (!no_render) printf("Simulation completed — final grid saved to %s\n", outfile);
    return 0;
}