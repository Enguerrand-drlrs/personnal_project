#include "gameoflife.h"
#include <string.h>

#define BYTE_INDEX(x) ((x) >> 3)
#define BIT_MASK(x)   (1 << ((x) & 7))
#define GET(w,x,y)    ((w->grid[y][BYTE_INDEX(x)] & BIT_MASK(x)) ? 1 : 0)
#define SET(w,x,y,v)  do { \
    if (v) w->next[y][BYTE_INDEX(x)] |=  BIT_MASK(x); \
    else   w->next[y][BYTE_INDEX(x)] &= ~BIT_MASK(x); \
} while(0)

// ---- Allocation ----
LifeWorld *life_create(uint16_t width, uint16_t height) {
    LifeWorld *w = malloc(sizeof(LifeWorld));
    if (w == NULL) return NULL;
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
/*
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
}*/

/*
 * ALTERNATIVE BOUNDARY HANDLERS (uncomment one to try it):
 *
 * 1) Edge  - default above (outside cells are dead)
 * 2) Toroidal - wrap around edges (left<->right, top<->bottom)
 * 3) Mirror - reflect coordinates at the borders
 * 4) Alive rim - anything outside the grid is treated as alive
 *
 * To use an alternative, replace the active `neighbor_count` function
 * body above with one of the implementations below (or rename and
 * call the alternative from the active function).
 */


// 2) TOROIDAL (wrap around)
static inline uint8_t neighbor_count(LifeWorld *w, int x, int y) {
    uint8_t n = 0;
    int width = w->width, height = w->height;
    for (int dy = -1; dy <= 1; dy++) {
        int yy = (y + dy + height) % height;
        for (int dx = -1; dx <= 1; dx++) {
            int xx = (x + dx + width) % width;
            if (dx == 0 && dy == 0) continue;
            if (GET(w, xx, yy)) {
                if (++n > 3) return n;
            }
        }
    }
    return n;
}


/*
// 3) MIRROR (reflect coordinates)
static inline uint8_t neighbor_count(LifeWorld *w, int x, int y) {
    uint8_t n = 0;
    int width = w->width, height = w->height;
    for (int dy = -1; dy <= 1; dy++) {
        int yy = y + dy;
        if (yy < 0) yy = -yy;                     // reflect at top
        if (yy >= height) yy = height - 1 - (yy - height); // reflect at bottom
        if (yy < 0) yy = 0;                       // guard for tiny grids
        for (int dx = -1; dx <= 1; dx++) {
            int xx = x + dx;
            if (xx < 0) xx = -xx;                 // reflect at left
            if (xx >= width) xx = width - 1 - (xx - width); // reflect at right
            if (xx < 0) xx = 0;
            if (dx == 0 && dy == 0) continue;
            if (GET(w, xx, yy)) {
                if (++n > 3) return n;
            }
        }
    }
    return n;
}
*/

/*
// 4) ALIVE RIM (outside cells are alive)
static inline uint8_t neighbor_count(LifeWorld *w, int x, int y) {
    uint8_t n = 0;
    int width = w->width, height = w->height;
    for (int dy = -1; dy <= 1; dy++) {
        int yy = y + dy;
        for (int dx = -1; dx <= 1; dx++) {
            int xx = x + dx;
            if (dx == 0 && dy == 0) continue;
            if (xx < 0 || xx >= width || yy < 0 || yy >= height) {
                if (++n > 3) return n; // outside counts as alive
                continue;
            }
            if (GET(w, xx, yy)) {
                if (++n > 3) return n;
            }
        }
    }
    return n;
}
*/



void life_destroy(LifeWorld *w) {
    if (w == NULL) return;
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

// Load a pattern from a file and place it centered in the grid.
// Format: each line contains '*' (alive) or '.' (dead)
// Returns 0 on success, -1 on error.
int life_load_pattern(LifeWorld *w, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        perror("Error opening pattern file");
        return -1;
    }

    // Read pattern into a temporary buffer
    char line[512];
    uint16_t pattern_height = 0;
    uint16_t pattern_width = 0;
    char pattern_data[256][256]; // Max 256x256 pattern

    while (fgets(line, sizeof(line), f) != NULL) {
        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';

        // Skip empty lines
        if (strlen(line) == 0)
            continue;

        // Track width (max width of any line)
        if ((uint16_t)strlen(line) > pattern_width)
            pattern_width = strlen(line);

        // Store the line
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

    // Calculate offset to center the pattern
    int offset_x = (int)(w->width - pattern_width) / 2;
    int offset_y = (int)(w->height - pattern_height) / 2;

    // Place the pattern into the grid at the center
    for (uint16_t py = 0; py < pattern_height; py++) {
        for (uint16_t px = 0; px < pattern_width; px++) {
            int gx = offset_x + px;
            int gy = offset_y + py;

            // Only place if within grid bounds
            if (gx >= 0 && gx < (int)w->width && gy >= 0 && gy < (int)w->height) {
                uint8_t alive = (pattern_data[py][px] == '*') ? 1 : 0;
                if (alive) {
                    uint16_t byte_idx = BYTE_INDEX(gx);
                    uint8_t bit = BIT_MASK(gx);
                    w->grid[gy][byte_idx] |= bit;
                }
            }
        }
    }

    return 0;
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

void life_save(FILE *f, LifeWorld *w, uint16_t gen) {
    fprintf(f, "Generation %u\n", gen);
    for (uint16_t y = 0; y < w->height; y++) {
        for (uint16_t x = 0; x < w->width; x++)
            fputc(GET(w, x, y) ? '*' : '.', f);
        fputc('\n', f);
    }
    fputc('\n', f);
}