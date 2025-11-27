#include "gameoflife.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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
    // For performance, use a specialized torus implementation when possible
    if (w->boundary == BOUNDARY_TORUS) {
        // optimized torus stepping
        uint16_t width = w->width;
        uint16_t height = w->height;
        const uint8_t *grid = w->grid;
        uint8_t *next = w->next;

        for (uint16_t y = 0; y < height; y++) {
            uint16_t y_up = (y == 0) ? (height - 1) : (y - 1);
            uint16_t y_down = (y + 1 == height) ? 0 : (y + 1);
            const uint8_t *row_up = grid + (uint32_t)y_up * width;
            const uint8_t *row = grid + (uint32_t)y * width;
            const uint8_t *row_down = grid + (uint32_t)y_down * width;

            for (uint16_t x = 0; x < width; x++) {
                uint16_t x_left = (x == 0) ? (width - 1) : (x - 1);
                uint16_t x_right = (x + 1 == width) ? 0 : (x + 1);

                uint8_t n = 0;
                n += row_up[x_left];
                n += row_up[x];
                n += row_up[x_right];
                n += row[x_left];
                n += row[x_right];
                n += row_down[x_left];
                n += row_down[x];
                n += row_down[x_right];

                uint8_t alive = row[x];
                uint8_t cell_next = (alive && (n == 2 || n == 3)) || (!alive && n == 3);
                next[(uint32_t)y * width + x] = cell_next;
            }
        }

        // Swap buffers
        uint8_t *tmp = w->grid;
        w->grid = w->next;
        w->next = tmp;
        return;
    }

    // Fallback to generic implementation using neighbor_count for other boundaries
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
// render scale global (default 1)
int life_render_scale = 1;

void life_print(LifeWorld *w, uint16_t gen) {
    // Build the whole frame in a buffer and write once to stdout
    int scale = life_render_scale > 1 ? life_render_scale : 1;
    uint16_t out_w = (w->width + scale - 1) / scale;
    uint16_t out_h = (w->height + scale - 1) / scale;
    size_t row_len = (size_t)out_w + 1; // +1 for newline
    size_t need = 64 + row_len * (size_t)out_h; // header + scaled grid
    static char *buf = NULL;
    static size_t bufcap = 0;
    if (need > bufcap) {
        free(buf);
        buf = (char *)malloc(need);
        if (buf == NULL) return; // allocation failure silently skip
        bufcap = need;
    }

    char *p = buf;
    // Home escape + header
    memcpy(p, "\x1b[H", 3); p += 3;
    int n = sprintf(p, "Generation: %u\n\n", gen);
    p += n;

    // Scaled output: group (scale x scale) cells into one char.
    for (uint16_t oy = 0; oy < out_h; oy++) {
        uint16_t base_y = oy * scale;
        for (uint16_t ox = 0; ox < out_w; ox++) {
            uint16_t base_x = ox * scale;
            // If any cell in the block is alive -> '*', else '.'
            int any = 0;
            for (uint16_t dy = 0; dy < scale && base_y + dy < w->height && !any; dy++) {
                const uint8_t *row = w->grid + (uint32_t)(base_y + dy) * w->width;
                for (uint16_t dx = 0; dx < scale && base_x + dx < w->width; dx++) {
                    if (row[base_x + dx]) { any = 1; break; }
                }
            }
            *p++ = any ? '*' : '.';
        }
        *p++ = '\n';
    }

    size_t written = p - buf;
    fwrite(buf, 1, written, stdout);
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
