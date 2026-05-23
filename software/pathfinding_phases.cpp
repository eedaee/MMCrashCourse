#include "API.h"
#include <cstdint>

// reuse existing Maze, Coord, Cell structs but avoid dynamic memory in MCU build
// ...existing code...

enum RunPhase {
    PHASE_INIT,
    PHASE_EXPLORE_TO_GOAL,
    PHASE_RETURN_EXPLORE,
    PHASE_SPEEDRUN,
    PHASE_DONE
};

// small stack-allocated neighbor container (no new/delete)
struct NeighborBuf {
    int size;
    Cell cells[4];
};

static void getNeighborsStatic(Maze* maze, Coord c, NeighborBuf* out) {
    out->size = 0;
    // identical logic to getNeighborCells but writes into out->cells
    for (int d = 0; d < 4; ++d) {
        int mask = dir_mask[d];
        if (!(maze->cellWalls[c.y][c.x] & mask)) {
            Cell nb;
            nb.pos = c;
            nb.dir = static_cast<Direction>(d);
            if (d == NORTH) nb.pos.y += 1;
            else if (d == EAST) nb.pos.x += 1;
            else if (d == SOUTH) nb.pos.y -= 1;
            else if (d == WEST) nb.pos.x -= 1;
            out->cells[out->size++] = nb;
        }
    }
}

// high level helpers (declarations)
static void phaseInit(Maze* maze);
static void phaseExploreToGoal(Maze* maze);
static void phaseReturnExplore(Maze* maze);
static void phaseSpeedrun(Maze* maze);

// main loop as FSM
int main() {
    Maze maze;
    // initialize arrays (no dynamic alloc)
    for (int y = 0; y < MAZE_SIZE; ++y)
        for (int x = 0; x < MAZE_SIZE; ++x) {
            maze.cellWalls[y][x] = 0;
            maze.distances[y][x] = -1;
        }

    maze.mouse_pos = {0,0};
    maze.mouse_dir = NORTH;

    RunPhase phase = PHASE_INIT;

    // main scheduler loop (single-threaded, deterministic)
    while (phase != PHASE_DONE) {
        switch (phase) {
            case PHASE_INIT:
                phaseInit(&maze);
                phase = PHASE_EXPLORE_TO_GOAL;
                break;
            case PHASE_EXPLORE_TO_GOAL:
                phaseExploreToGoal(&maze);
                // inside this function set a flag / return when goal reached to move to next phase
                if (/* goal reached */ maze.mouse_pos.x == maze.goalPos->x && maze.mouse_pos.y == maze.goalPos->y)
                    phase = PHASE_RETURN_EXPLORE;
                break;
            case PHASE_RETURN_EXPLORE:
                phaseReturnExplore(&maze);
                if (/* back at start */ false) phase = PHASE_SPEEDRUN;
                break;
            case PHASE_SPEEDRUN:
                phaseSpeedrun(&maze);
                phase = PHASE_DONE;
                break;
            default:
                phase = PHASE_DONE;
        }
        // lightweight delay or scheduler yield; watchdog kick
        // HAL_delay_ms(1); HAL_kick_watchdog();
    }

    return 0;
}

// ...existing code...