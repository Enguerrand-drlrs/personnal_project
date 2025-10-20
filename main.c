#include <stdio.h>
#include <stdlib.h>
#include <time.h>
/*
Rules : 
Any celles with fewer than 2 neighbourgs die (underpopulation)
Any celles with 2 or 3 neighbours live on to the next generation
Any celles with more than 3 neighbours die (overpopulation)
Any dead celles with exactly 3 neighbours become a live celle (reproduction)
*/

unsigned char status = 0; // 0 = dead, 1 = alive
typedef struct GridCell {
    unsigned char status;
} GridCell;


// make the grid
#define GRID_SIZE 10
GridCell grid[GRID_SIZE][GRID_SIZE];
void init_grid() {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j].status = 0; // all cells dead initially
        }
    }
}
    
void get_cell_state(x,y){}

void num_neighbors(x,y){}

void get_next_state(x,y){}

void next_generation(){}


int main() {
    init_grid(); 
    for ( int i=0; i<1000; i++){
        next_generation();
        clock_gettime();
    }
    return 0;
}