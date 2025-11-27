#ifndef GAMEOFLIFE_H
#define GAMEOFLIFE_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Use byte-per-cell for speed (not bit-packed)
typedef enum { BOUNDARY_EDGE = 0, BOUNDARY_TORUS, BOUNDARY_MIRROR, BOUNDARY_ALIVE } BoundaryMode;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t *grid;       // current generation (contiguous, 1 byte per cell)
    uint8_t *next;       // next generation (contiguous, 1 byte per cell)
    BoundaryMode boundary;
} LifeWorld;

// ---- API ----
LifeWorld *life_create(uint16_t width, uint16_t height);
void life_destroy(LifeWorld *w);
void life_randomize(LifeWorld *w);
int life_load_pattern(LifeWorld *w, const char *filename);
void life_step(LifeWorld *w);
void life_print(LifeWorld *w, uint16_t gen);
void life_clear_screen(void);
void life_save_final(FILE *f, LifeWorld *w);

// Render scale: group `life_render_scale`×`life_render_scale` cells into one output character
extern int life_render_scale;


#endif
