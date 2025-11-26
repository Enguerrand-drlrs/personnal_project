#include "gameoflife.h"
#include <string.h>

// ---- Allocation (contiguous byte-per-cell buffers) ----
LifeWorld *life_create(uint16_t width, uint16_t height) {
    LifeWorld *w = malloc(sizeof(LifeWorld));
    if (w == NULL) return NULL;
    w->width = width;
    w->height = height;

    uint32_t total = (uint32_t)width * height;
    w->grid = calloc(total, sizeof(uint8_t));  // All zeros (dead)
    w->next = calloc(total, sizeof(uint8_t));  // All zeros (dead)
    
    if (w->grid == NULL || w->next == NULL) {
        free(w->grid);
        free(w->next);
        free(w);
        return NULL;
    }
    return w;
}

void life_destroy(LifeWorld *w) {
    if (w == NULL) return;
    free(w->grid);
    free(w->next);
    free(w);
}

void life_randomize(LifeWorld *w) {
    uint32_t total = (uint32_t)w->width * w->height;
    for (uint32_t i = 0; i < total; i++)
        w->grid[i] = (rand() & 1) ? 1 : 0;
}

// Load a pattern from a file and place it centered in the grid.
int life_load_pattern(LifeWorld *w, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        perror("Error opening pattern file");
        return -1;
    }

    char line[512];
    uint16_t pattern_height = 0;
    uint16_t pattern_width = 0;
    char pattern_data[256][256];

    while (fgets(line, sizeof(line), f) != NULL) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';

        if (strlen(line) == 0)
            continue;

        if ((uint16_t)strlen(line) > pattern_width)
            pattern_width = strlen(line);

        if (pattern_height < 256) {
            strcpy(pattern_data[pattern_height], line);
            pattern_height++;
        }
    }
    fclose(f);

    if (pattern_height == 0 || pattern_width == 0) {
        fprintf(stderr, "Pattern file is empty\n");
        return -1;
    }

    int offset_x = (int)(w->width - pattern_width) / 2;
    int offset_y = (int)(w->height - pattern_height) / 2;

    for (uint16_t py = 0; py < pattern_height; py++) {
        for (uint16_t px = 0; px < pattern_width; px++) {
            int gx = offset_x + px;
            int gy = offset_y + py;

            if (gx >= 0 && gx < (int)w->width && gy >= 0 && gy < (int)w->height) {
                if (pattern_data[py][px] == '*') {
                    w->grid[(uint32_t)gy * w->width + gx] = 1;
                }
            }
        }
    }

    return 0;
}

// ---- Neighbor counting honoring boundary mode ----
static inline uint8_t neighbor_count(const LifeWorld *w, uint16_t x, uint16_t y) {
    const uint8_t *grid = w->grid;
    uint16_t width = w->width;
    uint16_t height = w->height;
    uint32_t stride = width;
    uint8_t n = 0;

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int xx = (int)x + dx;
            int yy = (int)y + dy;

            switch (w->boundary) {
                case BOUNDARY_EDGE:
                    if (xx < 0 || xx >= (int)width || yy < 0 || yy >= (int)height)
                        continue; // outside is dead
                    break;
                case BOUNDARY_TORUS:
                    if (width && height) {
                        xx = (xx % (int)width + (int)width) % (int)width;
                        yy = (yy % (int)height + (int)height) % (int)height;
                    }
                    break;
                case BOUNDARY_MIRROR:
                    if (xx < 0) xx = -xx;
                    if (yy < 0) yy = -yy;
                    if (xx >= (int)width) xx = (int)width - 1 - (xx - (int)width);
                    if (yy >= (int)height) yy = (int)height - 1 - (yy - (int)height);
                    if (xx < 0) xx = 0;
                    if (yy < 0) yy = 0;
                    break;
                case BOUNDARY_ALIVE:
                    if (xx < 0 || xx >= (int)width || yy < 0 || yy >= (int)height) {
                        if (++n > 3) return n;
                        continue;
                    }
                    break;
            }

            n += grid[(uint32_t)yy * stride + (uint32_t)xx];
            if (n > 3) return n;
        }
    }
    return n;
}

// ---- Evolution (optimized) ----
void life_step(LifeWorld *w) {
    uint32_t total = (uint32_t)w->width * w->height;
    uint16_t width = w->width;
    uint16_t height = w->height;
    const uint8_t *grid = w->grid;
    uint8_t *next = w->next;
    
    for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = 0; x < width; x++) {
            uint8_t alive = grid[(uint32_t)y * width + x];
            uint8_t n = neighbor_count(w, x, y);
            uint8_t cell_next = (alive && (n == 2 || n == 3)) || (!alive && n == 3);
            next[(uint32_t)y * width + x] = cell_next;
        }
    }
    
    // Swap buffers
    uint8_t *tmp = w->grid;
    w->grid = w->next;
    w->next = tmp;
}

// ---- Display ----
void life_print(LifeWorld *w, uint16_t gen) {
    printf("\x1b[H");  // Move cursor to home
    printf("Generation: %u\n\n", gen);
    for (uint16_t y = 0; y < w->height; y++) {
        for (uint16_t x = 0; x < w->width; x++)
            putchar(w->grid[(uint32_t)y * w->width + x] ? '*' : '.');
        putchar('\n');
    }
    fflush(stdout);
}

void life_clear_screen(void) {
    printf("\x1b[2J\x1b[H");
    fflush(stdout);
}

void life_save_final(FILE *f, LifeWorld *w) {
    for (uint16_t y = 0; y < w->height; y++) {
        for (uint16_t x = 0; x < w->width; x++)
            fputc(w->grid[(uint32_t)y * w->width + x] ? '*' : '.', f);
        fputc('\n', f);
    }
}
