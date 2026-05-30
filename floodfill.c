#include "main.h"
#include "micromouse.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define COMP_SIZE 16
#define SIM_SIZE  5
#define MAX_COST  255

#define MAZE_SIZE COMP_SIZE

/* Tune these after watching raw ADC values in Live Expressions. */
#define WALL_R_THRESHOLD   1500
#define WALL_FR_THRESHOLD  1500
#define WALL_FL_THRESHOLD  1500
#define WALL_L_THRESHOLD   1500

#define MOVE_MAX 256

typedef enum {
  NORTH = 0,
  EAST  = 1,
  SOUTH = 2,
  WEST  = 3
} Direction;

typedef enum {
  PHASE_INIT,
  PHASE_EXPLORE_TO_GOAL,
  PHASE_RETURN_TO_START,
  PHASE_EXPLORE_FULL,
  PHASE_SPEEDRUN,
  PHASE_DONE
} RunPhase;

typedef enum {
  NORTH_WALL = 0b1000,
  EAST_WALL  = 0b0100,
  SOUTH_WALL = 0b0010,
  WEST_WALL  = 0b0001
} WallBit;

typedef struct {
  int x;
  int y;
} Coord;

typedef struct {
  Coord pos;
  Direction dir;
  bool blocked;
} Cell;

typedef struct {
  int size;
  Cell cells[4];
} CellList;

typedef struct {
  int distance[MAZE_SIZE][MAZE_SIZE];
  int cellWalls[MAZE_SIZE][MAZE_SIZE];

  Coord mouse_pos;
  Direction mouse_dir;
  Coord goalPos;
  bool mouse_blocked;
} Maze;

typedef struct {
  Coord cells[MAZE_SIZE * MAZE_SIZE];
  int head;
  int tail;
} CoordQueue;

static Maze maze;
static CoordQueue q;

static Coord startPos;
static Coord goalCells[4];
static int goalCount = 0;

static Coord activeTargets[4];
static int activeTargetCount = 0;

static RunPhase phase = PHASE_EXPLORE_TO_GOAL;
static int moveCount = 0;

volatile uint16_t ir_R  = 0;
volatile uint16_t ir_FR = 0;
volatile uint16_t ir_FL = 0;
volatile uint16_t ir_L  = 0;


__weak void MM_TurnRight90(void) {
  /* TODO: add right turn motor control */
}

__weak void MM_TurnLeft90(void) {
  /* TODO: add left turn motor control */
}

__weak void MM_MoveForwardOneCell(void) {
  /* TODO: add forward one-cell motor control */
	Encoders_Reset();

	  Motors_Forward(300);

	  while (Encoders_GetCenterDistanceMM() < 180.0f)
	  {
	    Encoders_Update();
	  }

	  Motors_Stop();
}

static int directionBitmask(Direction d) {
  switch (d) {
    case NORTH: return NORTH_WALL;
    case EAST:  return EAST_WALL;
    case SOUTH: return SOUTH_WALL;
    case WEST:  return WEST_WALL;
    default:    return 0;
  }
}

static bool coordEquals(Coord a, Coord b) {
  return (a.x == b.x) && (a.y == b.y);
}

static void queue_clear(CoordQueue* queue) {
  queue->head = 0;
  queue->tail = 0;
}

static bool queue_empty(const CoordQueue* queue) {
  return queue->head == queue->tail;
}

static bool queue_push(CoordQueue* queue, Coord c) {
  if (queue->tail >= MAZE_SIZE * MAZE_SIZE) {
    return false;
  }

  queue->cells[queue->tail++] = c;
  return true;
}

static Coord queue_pop(CoordQueue* queue) {
  return queue->cells[queue->head++];
}

static void setWallAt(Maze* m, int x, int y, Direction d) {
  if (x < 0 || x >= MAZE_SIZE || y < 0 || y >= MAZE_SIZE) {
    return;
  }

  m->cellWalls[y][x] |= directionBitmask(d);

  int nx = x;
  int ny = y;

  if (d == NORTH) {
    ny += 1;
  } else if (d == EAST) {
    nx += 1;
  } else if (d == SOUTH) {
    ny -= 1;
  } else if (d == WEST) {
    nx -= 1;
  }

  if (nx >= 0 && nx < MAZE_SIZE && ny >= 0 && ny < MAZE_SIZE) {
    Direction opposite = (Direction)((d + 2) % 4);
    m->cellWalls[ny][nx] |= directionBitmask(opposite);
  }
}

static void setGoalCells(void) {
#if MAZE_SIZE == SIM_SIZE
  goalCount = 1;
  goalCells[0] = (Coord){2, 2};
#else
  goalCount = 4;
  goalCells[0] = (Coord){7, 7};
  goalCells[1] = (Coord){7, 8};
  goalCells[2] = (Coord){8, 7};
  goalCells[3] = (Coord){8, 8};
#endif

  maze.goalPos = goalCells[0];
}

static bool isGoalCell(Coord pos) {
  for (int i = 0; i < goalCount; i++) {
    if (coordEquals(pos, goalCells[i])) {
      return true;
    }
  }

  return false;
}

static void setPhaseTargets(void) {
  activeTargetCount = 0;

  switch (phase) {
    case PHASE_EXPLORE_TO_GOAL:
      for (int i = 0; i < goalCount; i++) {
        activeTargets[activeTargetCount++] = goalCells[i];
      }
      break;

    case PHASE_RETURN_TO_START:
      activeTargets[activeTargetCount++] = startPos;
      break;

    case PHASE_EXPLORE_FULL:
      /* Placeholder: currently target center again. Later, replace with frontier cells. */
      for (int i = 0; i < goalCount; i++) {
        activeTargets[activeTargetCount++] = goalCells[i];
      }
      break;

    case PHASE_SPEEDRUN:
      for (int i = 0; i < goalCount; i++) {
        activeTargets[activeTargetCount++] = goalCells[i];
      }
      break;

    case PHASE_INIT:
    case PHASE_DONE:
    default:
      break;
  }
}

static bool reachedPhaseTarget(void) {
  switch (phase) {
    case PHASE_EXPLORE_TO_GOAL:
      return isGoalCell(maze.mouse_pos);

    case PHASE_RETURN_TO_START:
      return coordEquals(maze.mouse_pos, startPos);

    case PHASE_EXPLORE_FULL:
      return isGoalCell(maze.mouse_pos);

    case PHASE_SPEEDRUN:
      return isGoalCell(maze.mouse_pos);

    default:
      return false;
  }
}

static void updatePhase(void) {
  switch (phase) {
    case PHASE_EXPLORE_TO_GOAL:
      phase = PHASE_RETURN_TO_START;
      break;

    case PHASE_RETURN_TO_START:
      phase = PHASE_EXPLORE_FULL;
      break;

    case PHASE_EXPLORE_FULL:
      phase = PHASE_SPEEDRUN;
      break;

    case PHASE_SPEEDRUN:
      phase = PHASE_DONE;
      break;

    default:
      phase = PHASE_DONE;
      break;
  }
}

static void initPerimeterWalls(void) {
  for (int i = 0; i < MAZE_SIZE; i++) {
    setWallAt(&maze, i, MAZE_SIZE - 1, NORTH);
    setWallAt(&maze, i, 0, SOUTH);
    setWallAt(&maze, 0, i, WEST);
    setWallAt(&maze, MAZE_SIZE - 1, i, EAST);
  }
}

static void getNeighborCells(const Maze* m, Coord c, CellList* out) {
  out->size = 0;

  for (int d = 0; d < 4; d++) {
    Direction dir = (Direction)d;
    int wall_mask = directionBitmask(dir);

    if (c.x < 0 || c.x >= MAZE_SIZE || c.y < 0 || c.y >= MAZE_SIZE) {
      continue;
    }

    if (m->cellWalls[c.y][c.x] & wall_mask) {
      continue;
    }

    Cell neighbor;
    neighbor.pos = c;
    neighbor.dir = dir;
    neighbor.blocked = false;

    if (dir == NORTH) {
      neighbor.pos.y += 1;
    } else if (dir == EAST) {
      neighbor.pos.x += 1;
    } else if (dir == SOUTH) {
      neighbor.pos.y -= 1;
    } else if (dir == WEST) {
      neighbor.pos.x -= 1;
    }

    if (neighbor.pos.x < 0 || neighbor.pos.x >= MAZE_SIZE ||
        neighbor.pos.y < 0 || neighbor.pos.y >= MAZE_SIZE) {
      continue;
    }

    out->cells[out->size++] = neighbor;
  }
}

static void floodFill(void) {
  for (int y = 0; y < MAZE_SIZE; y++) {
    for (int x = 0; x < MAZE_SIZE; x++) {
      maze.distance[y][x] = -1;
    }
  }

  queue_clear(&q);

  for (int i = 0; i < activeTargetCount; i++) {
    Coord target = activeTargets[i];

    if (target.x < 0 || target.x >= MAZE_SIZE ||
        target.y < 0 || target.y >= MAZE_SIZE) {
      continue;
    }

    maze.distance[target.y][target.x] = 0;
    queue_push(&q, target);
  }

  while (!queue_empty(&q)) {
    Coord cur = queue_pop(&q);
    int curDist = maze.distance[cur.y][cur.x];

    CellList neighbors;
    getNeighborCells(&maze, cur, &neighbors);

    for (int i = 0; i < neighbors.size; i++) {
      Coord npos = neighbors.cells[i].pos;

      if (maze.distance[npos.y][npos.x] == -1) {
        maze.distance[npos.y][npos.x] = curDist + 1;
        queue_push(&q, npos);
      }
    }
  }
}

static void scanWalls(void) {
  Coord p = maze.mouse_pos;
  Direction d = maze.mouse_dir;

  ir_R  = measure_dist(DIST_R);
  ir_FR = measure_dist(DIST_FR);
  ir_FL = measure_dist(DIST_FL);
  ir_L  = measure_dist(DIST_L);

  bool wallRight = (ir_R > WALL_R_THRESHOLD);
  bool wallFront = (ir_FR > WALL_FR_THRESHOLD) || (ir_FL > WALL_FL_THRESHOLD);
  bool wallLeft  = (ir_L > WALL_L_THRESHOLD);

  if (wallFront) {
    setWallAt(&maze, p.x, p.y, d);
  }

  if (wallRight) {
    Direction rightDir = (Direction)((d + 1) % 4);
    setWallAt(&maze, p.x, p.y, rightDir);
  }

  if (wallLeft) {
    Direction leftDir = (Direction)((d + 3) % 4);
    setWallAt(&maze, p.x, p.y, leftDir);
  }
}

static void rotate(Direction desired) {
  int cur = (int)maze.mouse_dir;
  int want = (int)desired;
  int diff = (want - cur + 4) % 4;

  if (diff == 0) {
    return;
  } else if (diff == 1) {
    MM_TurnRight90();
  } else if (diff == 2) {
    MM_TurnRight90();
    MM_TurnRight90();
  } else if (diff == 3) {
    MM_TurnLeft90();
  }

  maze.mouse_dir = desired;
}

static bool canMoveForward(void) {
  Coord cur = maze.mouse_pos;
  Direction d = maze.mouse_dir;

  if (cur.x < 0 || cur.x >= MAZE_SIZE || cur.y < 0 || cur.y >= MAZE_SIZE) {
    return false;
  }

  if (maze.cellWalls[cur.y][cur.x] & directionBitmask(d)) {
    return false;
  }

  int tx = cur.x;
  int ty = cur.y;

  if (d == NORTH) {
    ty += 1;
  } else if (d == SOUTH) {
    ty -= 1;
  } else if (d == EAST) {
    tx += 1;
  } else if (d == WEST) {
    tx -= 1;
  }

  if (tx < 0 || tx >= MAZE_SIZE || ty < 0 || ty >= MAZE_SIZE) {
    return false;
  }

  return true;
}

static void updateMousePos(void) {
  if (maze.mouse_dir == NORTH) {
    maze.mouse_pos.y += 1;
  } else if (maze.mouse_dir == SOUTH) {
    maze.mouse_pos.y -= 1;
  } else if (maze.mouse_dir == EAST) {
    maze.mouse_pos.x += 1;
  } else if (maze.mouse_dir == WEST) {
    maze.mouse_pos.x -= 1;
  }
}

static bool safeForwardMove(void) {
  if (!canMoveForward()) {
    return false;
  }

  MM_MoveForwardOneCell();
  updateMousePos();
  return true;
}

void micromouse_init(void) {
  memset(&maze, 0, sizeof(maze));

  maze.mouse_pos = (Coord){0, 0};
  maze.mouse_dir = NORTH;
  maze.mouse_blocked = false;

  startPos = maze.mouse_pos;
  phase = PHASE_EXPLORE_TO_GOAL;
  moveCount = 0;

  for (int y = 0; y < MAZE_SIZE; y++) {
    for (int x = 0; x < MAZE_SIZE; x++) {
      maze.distance[y][x] = MAX_COST;
      maze.cellWalls[y][x] = 0;
    }
  }

  initPerimeterWalls();
  setGoalCells();
  setPhaseTargets();
  floodFill();
}

void micromouse_step(void) {
  if (phase == PHASE_DONE) {
    return;
  }

  if (moveCount++ >= MOVE_MAX) {
    phase = PHASE_DONE;
    return;
  }

  setPhaseTargets();

  if (phase != PHASE_SPEEDRUN) {
    scanWalls();
  }

  floodFill();

  if (reachedPhaseTarget()) {
    updatePhase();
    return;
  }

  CellList neighbors;
  getNeighborCells(&maze, maze.mouse_pos, &neighbors);

  int bestIdx = -1;
  int bestDistance = maze.distance[maze.mouse_pos.y][maze.mouse_pos.x];

  for (int i = 0; i < neighbors.size; i++) {
    Coord npos = neighbors.cells[i].pos;
    int neighborDistance = maze.distance[npos.y][npos.x];

    if (neighborDistance != -1 && neighborDistance < bestDistance) {
      bestDistance = neighborDistance;
      bestIdx = i;
    }
  }

  if (bestIdx == -1) {
    maze.mouse_blocked = true;
    phase = PHASE_DONE;
    return;
  }

  rotate(neighbors.cells[bestIdx].dir);

  if (!safeForwardMove()) {
    maze.mouse_blocked = true;
    phase = PHASE_DONE;
    return;
  }
}



