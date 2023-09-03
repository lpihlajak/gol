#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define CELL_SIZE 10
#define GRID_WIDTH (SCREEN_WIDTH / CELL_SIZE)
#define GRID_HEIGHT (SCREEN_HEIGHT / CELL_SIZE)

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

enum CellState { EMPTY, FUEL, FIRE };

enum CellState grid[GRID_WIDTH][GRID_HEIGHT];
enum CellState nextGrid[GRID_WIDTH][GRID_HEIGHT];

bool running = true;
bool addingFuel = false;

pthread_t mouseThread;

// Mutex for controlling access to the addingFuel flag
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void initializeGrid() {
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            grid[x][y] = (rand() % 2 == 0) ? FUEL : EMPTY;
        }
    }

    int centerX = GRID_WIDTH / 2;
    int centerY = GRID_HEIGHT / 2;
    grid[centerX][centerY] = FIRE;
}

void addFuel(int x, int y) {
    if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
        grid[x][y] = FUEL;
    }
}

void handleInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
        } else if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                pthread_mutex_lock(&mutex);
                addingFuel = true;
                pthread_mutex_unlock(&mutex);
            }
        } else if (event.type == SDL_MOUSEBUTTONUP) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                pthread_mutex_lock(&mutex);
                addingFuel = false;
                pthread_mutex_unlock(&mutex);
            }
        }
    }
}

void* mouseInputThread(void* arg) {
    while (running) {
        pthread_mutex_lock(&mutex);
        if (addingFuel) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            int gridX = mouseX / CELL_SIZE;
            int gridY = mouseY / CELL_SIZE;
            addFuel(gridX, gridY);
        }
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void updateGrid() {
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            if (grid[x][y] == EMPTY) {
                // Empty cells remain empty
                nextGrid[x][y] = EMPTY;
            } else if (grid[x][y] == FIRE) {
                // Fire cells turn into empty cells after a certain time step
                nextGrid[x][y] = EMPTY;
            } else if (grid[x][y] == FUEL) {
                // Fuel cells can catch fire if adjacent to a fire cell
                nextGrid[x][y] = FUEL;

                // Check neighboring cells for fire
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        int nx = x + dx;
                        int ny = y + dy;

                        if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT &&
                            grid[nx][ny] == FIRE) {
                            // Fuel cell catches fire
                            nextGrid[x][y] = FIRE;
                        }
                    }
                }
            }
        }
    }

    // Swap the grids
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            grid[x][y] = nextGrid[x][y];
        }
    }
}

void drawGrid() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            if (grid[x][y] == FUEL) {
                // Draw fuel cells as green
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE);
            } else if (grid[x][y] == FIRE) {
                // Draw fire cells as red
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
            } else {
                // Draw empty cells as black
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            }

            // Draw cell as a rectangle
            SDL_Rect cellRect = {x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
            SDL_RenderFillRect(renderer, &cellRect);
        }
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    srand(time(NULL));

    window = SDL_CreateWindow("Fire Simulation", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    initializeGrid();

    // Create a thread for mouse input
    if (pthread_create(&mouseThread, NULL, mouseInputThread, NULL) != 0) {
        fprintf(stderr, "Error creating mouse input thread\n");
        return 1;
    }

    while (running) {
        handleInput();
        updateGrid();
        drawGrid();
        SDL_Delay(100);
    }

    // Wait for the mouse input thread to finish
    pthread_join(mouseThread, NULL);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

