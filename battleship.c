#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <float.h>

#define GRID_SIZE 10
#define NUM_SHIPS 5
#define NUM_SIMULATIONS 10000
#define NUM_RUNS 10

// Use bit fields to minimize memory usage
typedef struct {
    unsigned int size : 3;  // 3 bits for size (max 5)
    char symbol;
} Ship;

// Use a union to save memory by sharing space between grid and prob
typedef union {
    char grid[GRID_SIZE][GRID_SIZE];
    double prob[GRID_SIZE][GRID_SIZE];
} BoardData;

typedef struct {
    BoardData data;
    unsigned int shot[GRID_SIZE];  // Use bit field for shot tracking
    int ships_remaining;
} Board;

// Use const for read-only data
static const Ship ships[NUM_SHIPS] = {
    {5, 'C'}, {4, 'B'}, {3, 'D'}, {3, 'S'}, {2, 'P'}
};

// Use inline for small, frequently called functions
static inline void initialize_board(Board *board) {
    memset(&board->data, 0, sizeof(BoardData));
    memset(board->shot, 0, sizeof(board->shot));
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            board->data.prob[i][j] = 0.17;
        }
    }
    board->ships_remaining = NUM_SHIPS;
}

static inline bool is_shot(Board *board, int row, int col) {
    return (board->shot[row] & (1 << col)) != 0;
}

static inline void set_shot(Board *board, int row, int col) {
    board->shot[row] |= (1 << col);
}

bool place_ship(Board *board, int ship_index) {
    Ship ship = ships[ship_index];
    int row = rand() % GRID_SIZE;
    int col = rand() % GRID_SIZE;
    bool vertical = rand() % 2;
    
    for (int i = 0; i < ship.size; i++) {
        int r = vertical ? row + i : row;
        int c = vertical ? col : col + i;
        if (r >= GRID_SIZE || c >= GRID_SIZE || board->data.grid[r][c] != 0) {
            return false;
        }
    }
    
    for (int i = 0; i < ship.size; i++) {
        int r = vertical ? row + i : row;
        int c = vertical ? col : col + i;
        board->data.grid[r][c] = ship.symbol;
    }
    
    return true;
}

void place_ships(Board *board) {
    for (int i = 0; i < NUM_SHIPS; i++) {
        while (!place_ship(board, i));
    }
}

void update_probabilities(Board *board, int row, int col, bool hit) {
    board->data.prob[row][col] = 0;
    set_shot(board, row, col);
    
    if (hit) {
        int dr[] = {-1, 1, 0, 0};
        int dc[] = {0, 0, -1, 1};
        for (int i = 0; i < 4; i++) {
            int nr = row + dr[i];
            int nc = col + dc[i];
            if (nr >= 0 && nr < GRID_SIZE && nc >= 0 && nc < GRID_SIZE && !is_shot(board, nr, nc)) {
                board->data.prob[nr][nc] *= 1.5;
                if (board->data.prob[nr][nc] > 1.0) board->data.prob[nr][nc] = 1.0;
            }
        }
    }
}

bool make_shot(Board *board, int row, int col) {
    if (board->data.grid[row][col] != 0) {
        board->data.grid[row][col] = 'X';
        update_probabilities(board, row, col, true);
        return true;
    } else {
        board->data.grid[row][col] = 'O';
        update_probabilities(board, row, col, false);
        return false;
    }
}

int random_search(Board *board) {
    int row, col;
    do {
        row = rand() % GRID_SIZE;
        col = rand() % GRID_SIZE;
    } while (is_shot(board, row, col));
    return row * GRID_SIZE + col;
}

int pdf_search(Board *board) {
    int best_row = -1, best_col = -1;
    double max_prob = -1;
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (!is_shot(board, i, j) && board->data.prob[i][j] > max_prob) {
                max_prob = board->data.prob[i][j];
                best_row = i;
                best_col = j;
            }
        }
    }
    return best_row * GRID_SIZE + best_col;
}

int hunt_and_target(Board *board) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (board->data.grid[i][j] == 'X') {
                int dr[] = {-1, 1, 0, 0};
                int dc[] = {0, 0, -1, 1};
                for (int k = 0; k < 4; k++) {
                    int nr = i + dr[k];
                    int nc = j + dc[k];
                    if (nr >= 0 && nr < GRID_SIZE && nc >= 0 && nc < GRID_SIZE && !is_shot(board, nr, nc)) {
                        return nr * GRID_SIZE + nc;
                    }
                }
            }
        }
    }
    return random_search(board);
}

int simulate_game(Board *board, int (*strategy)(Board*)) {
    int shots = 0;
    while (board->ships_remaining > 0) {
        int move = strategy(board);
        int row = move / GRID_SIZE;
        int col = move % GRID_SIZE;
        bool hit = make_shot(board, row, col);
        shots++;
        if (hit) {
            bool sunk = true;
            for (int i = 0; i < GRID_SIZE; i++) {
                for (int j = 0; j < GRID_SIZE; j++) {
                    if (board->data.grid[i][j] == board->data.grid[row][col] && board->data.grid[i][j] != 'X') {
                        sunk = false;
                        break;
                    }
                }
                if (!sunk) break;
            }
            if (sunk) board->ships_remaining--;
        }
    }
    return shots;
}

// Use restrict keyword to optimize pointer usage
double run_simulation(int (* restrict strategy)(Board*)) {
    double total_shots = 0;
    Board board;  // Allocate on stack for better performance
    for (int i = 0; i < NUM_SIMULATIONS; i++) {
        initialize_board(&board);
        place_ships(&board);
        int shots = simulate_game(&board, strategy);
        total_shots += shots;
    }
    return total_shots / NUM_SIMULATIONS;
}

int main(void) {  // Use void for no parameters
    srand((unsigned int)time(NULL));
    
    printf("strategy,run,average_shots\n");  // CSV header
    
    for (int run = 0; run < NUM_RUNS; run++) {
        double random_avg = run_simulation(random_search);
        printf("Random,%d,%.2f\n", run + 1, random_avg);
        
        double pdf_avg = run_simulation(pdf_search);
        printf("PDF,%d,%.2f\n", run + 1, pdf_avg);
        
        double hunt_target_avg = run_simulation(hunt_and_target);
        printf("Hunt and Target,%d,%.2f\n", run + 1, hunt_target_avg);
    }
    
    return 0;
}