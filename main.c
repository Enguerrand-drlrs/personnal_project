#include "gameoflife.h"
#include "gameoflife.c"
/*
Rules : 
Any celles with fewer than 2 neighbourgs die (underpopulation)
Any celles with 2 or 3 neighbours live on to the next generation
Any celles with more than 3 neighbours die (overpopulation)
Any dead celles with exactly 3 neighbours become a live celle (reproduction)
*/

unsigned char status = 0; // 0 = dead, 1 = alive





void init_grid() {
    for (int i = 0; i < GRID_X; i++) {
        for (int j = 0; j < GRID_Y; j++) {
            grid[i][j].status = 0; // all cells dead initially
        }
    }
}
void first_grid(){
    printf("\n\n");
    for (int i = 0; i < GRID_X; i++) {
        for (int j = 0; j < GRID_Y; j++) {
            grid[i][j].status = rand() % 2; // random alive or dead
            if (grid[i][j].status == 1){
                printf("*");
            } else {
            printf(".");
            }
        }
        printf("\n");
    }
}

void get_cell_state(int x, int y){

}

int num_neighbors(int x, int y){
    int num = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0)
                continue; // skip the cell itself
            int nx = x + i;
            int ny = y + j;
            if (nx >= 0 && nx < GRID_X && ny >= 0 && ny < GRID_Y) {
                if (grid[nx][ny].status == 1)
                    num++;
            }
        }
    }
    return num;
}


void next_generation() {
    GridCell new_grid[GRID_X][GRID_Y];

    

    // Copy new grid to main grid
    for (int i = 0; i < GRID_X; i++) {
        for (int j = 0; j < GRID_Y; j++) {
            grid[i][j].status = new_grid[i][j].status;
        }
    }
}


void print_grid() {
    for (int i = 0; i < GRID_X; i++) {
        for (int j = 0; j < GRID_Y; j++) {
            printf("%c", grid[i][j].status ? '*' : '.');
        }
        printf("\n");
    }
}

int main() {
    init_grid(); 
    first_grid();
    for ( int i=0; i<1000; i++){
        //printf("\nGeneration %d:\n", i+1);
        next_generation();
        
        
        //clock_gettime();
    }
    printf("\nFinal Generation:\n");
    print_grid();
    return 0;
}