// Ensure include only once
#ifndef __MY_LCM_H__
#define __MY_LCM_H__
// Include Dependencies
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
// Function prototype
void initgrid();
void firstgrid();
void get_cell_state(int x, int y);  
int num_neighbors(int x, int y);
void next_generation();
void print_grid();
// Define constants
#define GRID_X 20
#define GRID_Y 50
// Define data structures
typedef struct {
    uint8_t status; // 0 for dead, 1 for alive
} GridCell;
// Global grid variable
GridCell grid[GRID_X][GRID_Y];

#endif // __MY_LCM_H__