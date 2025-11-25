#ifndef GAMEOFLIFE_H
#define GAMEOFLIFE_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t **grid;       // current generation
    uint8_t **next;       // next generation
} LifeWorld;

// ---- API ----
LifeWorld *life_create(uint16_t width, uint16_t height);
void life_destroy(LifeWorld *w);
void life_randomize(LifeWorld *w);
int life_load_pattern(LifeWorld *w, const char *filename);
void life_step(LifeWorld *w);
void life_print(LifeWorld *w);
void life_save(FILE *f, LifeWorld *w, uint16_t gen);


#endif
