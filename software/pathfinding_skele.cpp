#include "API.h"
#include <iostream>
#include <string>
#include <climits>
#define MAZE_SIZE 16

// direction
char dir_chars[4] = { 'n', 'e', 's', 'w' };

// allows you to use bitwise OR when updating which walls are present in a cell
// also allows you to bitwise AND
int dir_mask[4] = { 0b1000, 0b0100, 0b0010, 0b0001 };

enum Direction {
    NORTH = 0,
    EAST = 1,
    SOUTH = 2,
    WEST = 3
};

enum DirectionBitmask {
    NORTH_MASK = 0b1000,
    EAST_MASK = 0b0100,
    SOUTH_MASK = 0b0010,
    WEST_MASK = 0b0001
};

struct Coord {
    int x;
    int y;
};

struct Cell {
    Coord pos;
    Direction dir;
};

// now fixed-size, no dynamic allocation
struct CellList {
    int size;
    Cell cells[4];
};

struct Queue {
    // Cell object array with max size
    // Two int objects for head and tail
    Cell cells[MAZE_SIZE * MAZE_SIZE];
    int head;
    int tail;
};

struct Maze {
    Coord mouse_pos;
    Direction mouse_dir;

    // 2D arrays for distances of each cell, and values for cell walls
    int distances[MAZE_SIZE][MAZE_SIZE];
    int cellWalls[MAZE_SIZE][MAZE_SIZE];

    // goal as value (no dynamic memory)
    Coord goalPos;
};

// Queue functions
void initQueue(Queue* q) { // initialize empty queue
    q->head = 0;
    q->tail = 0;
}

bool isQEmpty(const Queue& q) {
    return (q.head == q.tail);
}

// Maze functions
void scanWalls(Maze* maze) { // read sensors and set walls in the map
    Coord p = maze->mouse_pos;
    Direction md = maze->mouse_dir;

    // helper to set wall at (x,y) in direction d and also set opposite wall in neighbor if inside bounds
    auto setWallAt = [&](int x, int y, int d) {
        if (x < 0 || x >= MAZE_SIZE || y < 0 || y >= MAZE_SIZE) return;
        maze->cellWalls[y][x] |= dir_mask[d];
        // set opposite in neighbor
        int nx = x, ny = y;
        if (d == NORTH) ny += 1;
        else if (d == EAST) nx += 1;
        else if (d == SOUTH) ny -= 1;
        else if (d == WEST) nx -= 1;
        if (nx >= 0 && nx < MAZE_SIZE && ny >= 0 && ny < MAZE_SIZE) {
            int opp = (d + 2) % 4;
            maze->cellWalls[ny][nx] |= dir_mask[opp];
        }
    };

    if (API::wallFront()) {
        int d = md; // front is current orientation
        setWallAt(p.x, p.y, d);
    }
    if (API::wallRight()) {
        int d = (md + 1) % 4;
        setWallAt(p.x, p.y, d);
    }
    if (API::wallLeft()) {
        int d = (md + 3) % 4;
        setWallAt(p.x, p.y, d);
    }
}

// Non-allocating neighbor enumerator: fills out CellList (caller-provided)
void getNeighborCells(const Maze* maze, Coord c, CellList& out) {
    out.size = 0;
    // check each direction for walls
    for (int d = 0; d < 4; d++) {
        int wall_mask = dir_mask[d];
        // guard bounds on source cell
        if (c.x < 0 || c.x >= MAZE_SIZE || c.y < 0 || c.y >= MAZE_SIZE) continue;
        if (!(maze->cellWalls[c.y][c.x] & wall_mask)) { // no wall in this direction
            Cell neighbor;
            neighbor.pos = c;
            neighbor.dir = static_cast<Direction>(d);
            // update neighbor position based on direction
            if (d == NORTH)
                neighbor.pos.y += 1;
            else if (d == EAST)
                neighbor.pos.x += 1;
            else if (d == SOUTH)
                neighbor.pos.y -= 1;
            else if (d == WEST)
                neighbor.pos.x -= 1;

            // optionally skip out-of-bounds neighbor
            if (neighbor.pos.x < 0 || neighbor.pos.x >= MAZE_SIZE || neighbor.pos.y < 0 || neighbor.pos.y >= MAZE_SIZE)
                continue;

            out.cells[out.size++] = neighbor;
        }
    }
}

void updateSimulator(Maze maze) { // redraws the maze in simulator after each loop in main
    // clear previous text so numbers don't pile up
    API::clearAllText();

    for (int x = 0; x < MAZE_SIZE; x++)
    {
        for (int y = 0; y < MAZE_SIZE; y++)
        {
            if (maze.cellWalls[y][x] & NORTH_MASK)
                API::setWall(x, y, 'n');

            if (maze.cellWalls[y][x] & EAST_MASK)
                API::setWall(x, y, 'e');

            if (maze.cellWalls[y][x] & SOUTH_MASK)
                API::setWall(x, y, 's');

            if (maze.cellWalls[y][x] & WEST_MASK)
                API::setWall(x, y, 'w');

            // show distance number (blank for unknown / -1)
            if (maze.distances[y][x] >= 0) {
                API::setText(x, y, std::to_string(maze.distances[y][x]));
            }
        }
    }
}

void updateMousePos(Coord* pos, Direction dir) {
    // depending on the mouse direction, increment position by one
    if (dir == NORTH)
        pos->y += 1;
    else if (dir == SOUTH)
        pos->y -= 1;
    else if (dir == WEST)
        pos->x -= 1;
    else if (dir == EAST)
        pos->x += 1;
}

// helper: check if forward move is allowed by known map and bounds
bool canMoveForward(const Maze* maze) {
    Coord cur = maze->mouse_pos;
    Direction d = maze->mouse_dir;
    if (cur.x < 0 || cur.x >= MAZE_SIZE || cur.y < 0 || cur.y >= MAZE_SIZE) return false;
    // if there's a wall in front according to the map, don't try
    if (maze->cellWalls[cur.y][cur.x] & dir_mask[d]) return false;
    // compute target cell and check bounds
    int tx = cur.x, ty = cur.y;
    if (d == NORTH) ty += 1;
    else if (d == SOUTH) ty -= 1;
    else if (d == EAST) tx += 1;
    else if (d == WEST) tx -= 1;
    if (tx < 0 || tx >= MAZE_SIZE || ty < 0 || ty >= MAZE_SIZE) return false;
    return true;
}

// safe wrapper: check, call API, update internal state
bool safeMoveForward(Maze* maze) {
    if (!canMoveForward(maze)) return false;
    API::moveForward(1);
    updateMousePos(&maze->mouse_pos, maze->mouse_dir);
    return true;
}

void floodFill(Maze* maze, bool to_start) { // BFS-based flood-fill
    // initialize distances to -1 (unvisited)
    for (int y = 0; y < MAZE_SIZE; ++y)
        for (int x = 0; x < MAZE_SIZE; ++x)
            maze->distances[y][x] = -1;

    Coord start;
    if (to_start) {
        start = maze->mouse_pos;
    } else {
        start = maze->goalPos;
    }

    Queue q;
    initQueue(&q);

    // seed
    if (start.x < 0 || start.x >= MAZE_SIZE || start.y < 0 || start.y >= MAZE_SIZE) return;
    maze->distances[start.y][start.x] = 0;
    q.cells[q.tail++] = Cell{ start, NORTH };

    while (!isQEmpty(q)) {
        Cell cur = q.cells[q.head++];
        int cx = cur.pos.x;
        int cy = cur.pos.y;
        int curDist = maze->distances[cy][cx];

        CellList neighbors;
        getNeighborCells(maze, cur.pos, neighbors);
        for (int i = 0; i < neighbors.size; ++i) {
            Coord npos = neighbors.cells[i].pos;
            // bounds are already checked by getNeighborCells, but double-check
            if (npos.x < 0 || npos.x >= MAZE_SIZE || npos.y < 0 || npos.y >= MAZE_SIZE)
                continue;

            if (maze->distances[npos.y][npos.x] == -1) {
                maze->distances[npos.y][npos.x] = curDist + 1;
                // guard queue push to avoid overflow on embedded target
                const int QUEUE_CAP = MAZE_SIZE * MAZE_SIZE;
                if (q.tail < QUEUE_CAP) {
                    q.cells[q.tail++] = Cell{ npos, neighbors.cells[i].dir };
                }
            }
        }
    }
}

// helper: rotate mouse to desired absolute direction (sends API turns and updates maze->mouse_dir)
void rotateTo(Maze* maze, Direction desired) {
    int cur = static_cast<int>(maze->mouse_dir);
    int want = static_cast<int>(desired);
    int diff = (want - cur + 4) % 4;
    if (diff == 0) return;
    if (diff == 1) {
        API::turnRight();
        maze->mouse_dir = static_cast<Direction>(want);
    } else if (diff == 2) {
        // turn around: two rights
        API::turnRight();
        API::turnRight();
        maze->mouse_dir = static_cast<Direction>(want);
    } else if (diff == 3) {
        API::turnLeft();
        maze->mouse_dir = static_cast<Direction>(want);
    }
}

// simple main loop to run floodfill + move in simulator (no dynamic allocation)
int main() {
    Maze maze;
    // init maze arrays
    for (int y = 0; y < MAZE_SIZE; ++y) {
        for (int x = 0; x < MAZE_SIZE; ++x) {
            maze.cellWalls[y][x] = 0;
            maze.distances[y][x] = -1;
        }
    }

    // set perimeter walls so algorithm never tries to leave bounds
    for (int x = 0; x < MAZE_SIZE; ++x) {
        maze.cellWalls[MAZE_SIZE-1][x] |= NORTH_MASK; // top row north wall
        maze.cellWalls[0][x] |= SOUTH_MASK;           // bottom row south wall
    }
    for (int y = 0; y < MAZE_SIZE; ++y) {
        maze.cellWalls[y][0] |= WEST_MASK;            // left col west wall 
        maze.cellWalls[y][MAZE_SIZE-1] |= EAST_MASK;  // right col east wall
    }

    // start at bottom-left (0,0) facing NORTH
    maze.mouse_pos = Coord{0, 0};
    maze.mouse_dir = NORTH;

    // remember start position so we can return later
    Coord startPos = maze.mouse_pos;

    // goal in center (as value)
    maze.goalPos = Coord{ MAZE_SIZE/2 - 1, MAZE_SIZE/2 - 1 };

    // save center for later speedrun
    Coord centerPos = maze.goalPos;

    const int MAX_STEPS = 500;
    int steps = 0;
    bool returning = false;
    bool speedrun = false;
    
    // initial simulator draw
    updateSimulator(maze);

    while (steps++ < MAX_STEPS) {
        // compute distances to goal
        floodFill(&maze, false);

        // update simulator immediately so numbers appear after floodFill
        updateSimulator(maze);

        // make sure we read sensors before deciding next move (also during speedrun)
        // this prevents trying to move into a real wall that isn't yet in the map
        scanWalls(&maze);

        // if at goal/start, handle state transitions
        if (maze.mouse_pos.x == maze.goalPos.x && maze.mouse_pos.y == maze.goalPos.y) {
            if (!returning && !speedrun) {
                // reached the center goal during exploration -> mark and set return-to-start
                API::setText(maze.mouse_pos.x, maze.mouse_pos.y, "GOAL");
                returning = true;
                maze.goalPos = startPos; // now head back to start
                // recompute next loop
                continue;
            } else if (returning && !speedrun) {
                // returned to start -> start speedrun (shortest path to center)
                API::setText(maze.mouse_pos.x, maze.mouse_pos.y, "START");
                returning = false;
                speedrun = true;
                maze.goalPos = centerPos; // head to center for speedrun
                // recompute next loop
                continue;
            } else if (speedrun) {
                // finished speedrun
                API::setText(maze.mouse_pos.x, maze.mouse_pos.y, "SPEEDRUN");
                break;
            }
        }

        // choose neighbor with lowest distance (using fixed CellList)
        CellList neighbors;
        getNeighborCells(&maze, maze.mouse_pos, neighbors);

        int bestIdx = -1;
        int bestDist = INT_MAX;
        for (int i = 0; i < neighbors.size; ++i) {
            Coord n = neighbors.cells[i].pos;
            if (n.x < 0 || n.x >= MAZE_SIZE || n.y < 0 || n.y >= MAZE_SIZE) continue;
            int d = maze.distances[n.y][n.x];
            if (d != -1 && d < bestDist) {
                bestDist = d;
                bestIdx = i;
            }
        }

        if (bestIdx == -1) {
            // no reachable neighbor -> stop
            break;
        }

        // rotate to desired direction and move
        Direction desired = neighbors.cells[bestIdx].dir;
        rotateTo(&maze, desired);
        // use safe move; if it fails, stop to avoid crashing
        if (!safeMoveForward(&maze)) {
            API::setText(maze.mouse_pos.x, maze.mouse_pos.y, "ERR");
            break;
        }

        // redraw simulator
        updateSimulator(maze);
    }

    return 0;
}
