#include "gameoflife.h"

#define BYTE_INDEX(x) ((x) >> 3)
#define BIT_MASK(x)   (1 << ((x) & 7))
#define GET(w,x,y)    ((w->grid[y][BYTE_INDEX(x)] & BIT_MASK(x)) ? 1 : 0)
#define SET(w,x,y,v)  do { \
    if (v) w->next[y][BYTE_INDEX(x)] |=  BIT_MASK(x); \
    else   w->next[y][BYTE_INDEX(x)] &= ~BIT_MASK(x); \
} while(0)

static inline uint8_t neighbor_count(LifeWorld *w, int x, int y) {
    uint8_t n = 0;
    for (int dy = -1; dy <= 1; dy++) {
        int yy = y + dy;
        if (yy < 0 || yy >= w->height) continue;
        for (int dx = -1; dx <= 1; dx++) {
            int xx = x + dx;
            if (dx == 0 && dy == 0) continue;
            if (xx < 0 || xx >= w->width) continue;
            if (GET(w, xx, yy)) {
                if (++n > 3) return n; // early exit
            }
        }
    }
    return n;
}

// ---- Allocation ----
LifeWorld *life_create(uint16_t width, uint16_t height) {
    LifeWorld *w = malloc(sizeof(LifeWorld));
    if (!w) return NULL;
    w->width = width;
    w->height = height;

    uint16_t row_bytes = (width + 7) / 8;
    w->grid = malloc(height * sizeof(uint8_t *));
    w->next = malloc(height * sizeof(uint8_t *));
    for (uint16_t y = 0; y < height; y++) {
        w->grid[y] = calloc(row_bytes, 1);
        w->next[y] = calloc(row_bytes, 1);
    }
    return w;
}

void life_destroy(LifeWorld *w) {
    if (!w) return;
    uint16_t row_bytes = (w->width + 7) / 8;
    for (uint16_t y = 0; y < w->height; y++) {
        free(w->grid[y]);
        free(w->next[y]);
    }
    free(w->grid);
    free(w->next);
    free(w);
}

void life_randomize(LifeWorld *w) {
    uint16_t row_bytes = (w->width + 7) / 8;
    for (uint16_t y = 0; y < w->height; y++)
        for (uint16_t i = 0; i < row_bytes; i++)
            w->grid[y][i] = rand() & 0xFF;
}

// ---- Evolution ----
void life_step(LifeWorld *w) {
    for (uint16_t y = 0; y < w->height; y++) {
        for (uint16_t x = 0; x < w->width; x++) {
            uint8_t alive = GET(w, x, y);
            uint8_t n = neighbor_count(w, x, y);
            uint8_t next = (alive && (n == 2 || n == 3)) || (!alive && n == 3);
            SET(w, x, y, next);
        }
    }
    // swap buffers
    uint8_t **tmp = w->grid;
    w->grid = w->next;
    w->next = tmp;
}

// ---- Display ----
void life_print(LifeWorld *w) {
    for (uint16_t y = 0; y < w->height; y++) {
        for (uint16_t x = 0; x < w->width; x++)
            putchar(GET(w, x, y) ? '*' : '.');
        putchar('\n');
    }
    putchar('\n');
}