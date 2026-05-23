#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <climits>
#include <queue>
#include <vector>
#include <thread>
#include <chrono>
#include "API.h"
#include "maze_types.h"

#define COMP_SIZE 16
#define SIM_SIZE  5
#define MAX_COST  255

using namespace std;

// ---------------------  Functions for Floodfill operations   --------------------
void setGoalCell(int size, int tail, Maze& maze, vector<Coord>& goalCells, queue<Coord>& q);
void getNeighborCells(const Maze* maze, Coord c, CellList& out);

void rotate(Maze* maze, Direction desired);
bool canMoveForward(const Maze* maze);
void updateMousePos(Coord* pos, Direction dir);
bool safeForwardMove(Maze* maze);
void scanWalls(Maze* maze);
void updateSimulator(Maze maze);
bool isGoalCell(const Coord& pos, const vector<Coord>& goalCells);
void setPhaseTargets(RunPhase phase, const Coord& startPos, const vector<Coord>& centerGoals, vector<Coord>& activeTargets);
bool reachedPhaseTarget(RunPhase phase, const Coord& mousePos, const Coord& startPos, const vector<Coord>& centerGoals);
void updatePhase(RunPhase& phase);
void floodFill(Maze* maze, queue<Coord>& q, const vector<Coord>& targets);

// ------------------------------- Main Function -------------------------------
int main(){

  // Creating Maze Structure as Maze, as well as mouse and it's parameters
  Maze maze;
  vector<Coord> goalCells;            // Vector to store goal cell coordinates
  maze.mouse_pos = {0, 0};            // Starting Position, bottom-left corner of the maze
  maze.mouse_dir = NORTH;             // Starting Direction, facing North 
  Coord startPos = maze.mouse_pos;     // Store the starting position to return to later

  maze.mouse_blocked = false;         // No blocks at start

  // Phase tracking variable
  RunPhase phase = PHASE_EXPLORE_TO_GOAL;
  vector<Coord> activeTargets; // Vector to store current targets based on phase (initially center goal(s))

  int moveCount = 0; // Counter for number of moves taken by the mouse 
  int moveMax = 300; // Max limit to exit FloodFill loop 

  // After declaring Maze maze;
  memset(maze.cellWalls, 0, sizeof(maze.cellWalls));
  for(int i = 0; i < mazeSize; i++)
    for(int j = 0; j < mazeSize; j++)
      maze.distance[i][j] = MAX_COST;

  API::clearAllText(); // Clear any existing text in the simulator

  // Maze Initialization [Distances and Walls]
  // Initiallize Max Cost value for all cells, and Goal Cell values
  for(int i = 0; i < mazeSize; i++){
    for(int j = 0; j < mazeSize; j++){
      API::setText(i, j, to_string(255));         // Initialize all cells with Max Cost value (255)
      for(int d = 0; d < 4; d++){
        API::clearWall(i, j, getDirInitial(d));   // No walls in any direction (N, E, S, W)
      }
    };
  };

  // Sleep for a moment to allow the simulator to update with the initial cell values before setting boundary walls
  //std::this_thread::sleep_for(std::chrono::milliseconds(1500)); 

  //Adding boundary walls so mouse cannot go out of bounds
  for(int i = 0; i < mazeSize; i++){              // Using mazeSize to set walls
    maze.cellWalls[mazeSize - 1][i]  |= NORTH_WALL;          // North wall for top row
    API::setWall(i, mazeSize - 1, getDirInitial(NORTH));
    
    maze.cellWalls[0][i] |= SOUTH_WALL;  // South wall for bottom row
    API::setWall(i, 0, getDirInitial(SOUTH));
    
    maze.cellWalls[i][0]  |= WEST_WALL;           // West wall for left column
    API::setWall(0, i, getDirInitial(WEST));
    
    maze.cellWalls[i][mazeSize-1] |= EAST_WALL;   // East wall for right column
    API::setWall(mazeSize - 1, i, getDirInitial(EAST));
  };

  // Sleep for a moment to allow the simulator to update with the boundary walls before starting the algorithm
  //std::this_thread::sleep_for(std::chrono::milliseconds(1500)); 

  // Print initial state of the mouse
  log("Starting Micromouse Floodfill Simulation\n");
  log("Start Position: ("); cerr << maze.mouse_pos.x << ", " << maze.mouse_pos.y << ")\n";
  log("Start Direction: "); cerr << getDirectionName(maze.mouse_dir) << "\n\n";


  // -------------------------------------------------------------------------------
  // Queue, head,and tail initialization
  queue<Coord> q; // Queue for floodfill algorithm
  Queue queue; // Custom queue structure for floodfill algorithm

  int head = 0; // Initialize head of the queue
  int tail = 0; // Initialize tail of the queue

  std::this_thread::sleep_for(std::chrono::milliseconds(1500)); 
  //Initialize Goal Cell, Set Goal Cell values, add them to queue
  setGoalCell(mazeSize, tail, maze, goalCells, q);
  // Store center position for later speedrun phase
  Coord centerPos = maze.goalPos; 
  updateSimulator(maze); // Update the simulator with the initial state of the maze before starting the floodfill algorithm

  // ----------------------- FloodFill Algo main loop -----------------------
  // Logic for floodfill algorithin is as follows:
  // 1. scanWalls()        ← scan FIRST at current position
  // 2. floodFill()        ← recompute with new wall data
  // 3. find best neighbor ← now uses correct distances
  // 4. rotate()           ← turn to face best neighbor
  // 5. safeForwardMove()  ← move
  // 6. updateSimulator()  ← redraw last

  while(moveCount++ < moveMax && phase != PHASE_DONE){

    // PRERUN: Log current state at the beginning of each loop iteration
    log("Current State: "); cerr << getPhaseName(phase) << "\n";

    activeTargets.clear();
    setPhaseTargets(phase, startPos, goalCells, activeTargets);

    // STEP 1: Scan walls at current position FIRST
    if(phase != PHASE_SPEEDRUN){ // During speedrun, we assume the map is fully known and skip scanning to save time
      scanWalls(&maze);
    } else {
      log("Speedrun phase.\n");
    }

    // STEP 2: Recompute distances with updated wall data
    floodFill(&maze, q, activeTargets);

    // STEP 3: Update simulator display
    if(phase != PHASE_SPEEDRUN) {
      updateSimulator(maze);
    }

    // STEP 4: Check if mouse has reached ANY goal cell for the current phase, and if so, update to next phase and set new targets
    if(reachedPhaseTarget(phase, maze.mouse_pos, startPos, goalCells)) {
      updatePhase(phase);

      API::setText(maze.mouse_pos.x, maze.mouse_pos.y, getPhaseName(phase));

      if(phase == PHASE_DONE) {
        log("Run complete!\n");
        break;
      }
      continue;
    }



    // STEP 5: Find best neighbor to move to
    CellList neighbors;
    getNeighborCells(&maze, maze.mouse_pos, neighbors);

    int bestIdx = -1;
    int bestDistance = maze.distance[maze.mouse_pos.y][maze.mouse_pos.x];
    
    for(int i = 0; i < neighbors.size; i++){
      Coord npos = neighbors.cells[i].pos;

      if(npos.x < 0 || npos.x >= mazeSize || npos.y < 0 || npos.y >= mazeSize) continue;

      int neighborDistance = maze.distance[npos.y][npos.x];

      if(neighborDistance != -1 && neighborDistance < bestDistance){
        bestDistance = neighborDistance;
        bestIdx = i;
      }
    }
    
    // Check if we're actually at a goal cell before declaring blocked
    if(bestIdx == -1){
      if(isGoalCell(maze.mouse_pos, goalCells)){
        log("Goal cell(s) reached!\n");
      } else {
        log("No accessible neighbors found. Mouse is blocked.\n");
        API::setText(maze.mouse_pos.x, maze.mouse_pos.y, "ERR");
      }
      break;
    }

    // STEP 6: Rotate to face best neighbor
    Direction desired = neighbors.cells[bestIdx].dir;
    rotate(&maze, desired);

    // STEP 7: Move forward
    if(!safeForwardMove(&maze)){
      log("Mouse is blocked by a wall. Cannot move forward.\n");
      API::setText(maze.mouse_pos.x, maze.mouse_pos.y, "ERR");
      break;
    }

  }

  return 0;

}



// ------------------------------- ------------------------------- -------------------------------
// Maze Function Definitions
// ------------------------------- ------------------------------- -------------------------------
void setGoalCell(int size, int tail, Maze& maze, vector<Coord>& goalCells, queue<Coord>& q) {
  switch(size){
    case SIM_SIZE:
      goalCells = {{2,2}};                         // Single center square for 5x5 maze
      maze.goalPos = goalCells[0];                 // Set initial goal position to the center cell (2,2)
      for (Coord goal : goalCells) {               // Set distance of goal cell to 0, print and add to queue
        maze.distance[goal.y][goal.x] = 0;
        API::setText(goal.y, goal.x, "0");
        q.push(goal);
        tail++;
      }
      // log("Tail Size: "); cerr << tail  << "\n";;
    break;
    
    case COMP_SIZE:
      goalCells = {{7,7}, {7,8}, {8,7}, {8,8}};;  // Center 2x2 square for 16x16 maze
      maze.goalPos = goalCells[0];                // Set initial goal position to one of the center cells (can be any of the 4)
      for (Coord goal : goalCells) {              // Set distance of goal cell to 0, print and add to queue
        maze.distance[goal.y][goal.x] = 0;
        API::setText(goal.y, goal.x, "0");
        q.push(goal);
        tail++;
      }
      // log("Tail Size: "); cerr << tail  << "\n";;
    break;

    default:
      log("Invalid maze size. Please use SIM_SIZE (5) or COMP_SIZE (16).\n");
    break;
  }

};

// Non-allocating neighbor enumerator: fills out CellList (caller-provided)
void getNeighborCells(const Maze* maze, Coord c, CellList& out){
  out.size = 0;

  //  check each direction for walls. If no wall, add to neighbor list with appropriate coordinates and direction.
  for (int d = 0; d < 4; d++) {

    int wall_mask = directionBitmask(d);

    // guard bounds on source cell
    if (c.x < 0 || c.x >= mazeSize || c.y < 0 || c.y >= mazeSize) continue;
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
      if (neighbor.pos.x < 0 || neighbor.pos.x >= mazeSize || neighbor.pos.y < 0 || neighbor.pos.y >= mazeSize)
        continue;

      out.cells[out.size++] = neighbor;

    }
  }
}

// Returns best accessible cell for the mouse to move to
Cell getBestCell(Maze& maze, Cell& mouse, int mazeSize = mazeSize) {
  
  CellList neighbors;

  getNeighborCells(&maze, mouse.pos, neighbors);

  Cell bestCell = mouse;                                       // Initialize best cell as current cell
  int bestDistance = maze.distance[mouse.pos.y][mouse.pos.x];  // Distance of current cell to goal

  // Check each neighbor cell and update best cell if it's closer to the goal
  for(int dir = 0; dir < 4; dir++){
    
    // Check if the neighbor cell is accessible (not blocked by a wall)
    Cell neighbor = neighbors.cells[dir];

    if(!neighbor.blocked){ // Only consider accessible cells
      
      int neighborDistance = maze.distance[neighbor.pos.y][neighbor.pos.x]; // Get distance of neighbor cell to goal
    
      if(neighborDistance < bestDistance){ // Update best cell if it's closer to the goal
      bestCell = neighbor;
      bestDistance = neighborDistance;
      }
    
    }
  }

  return bestCell; // Return the best accessible cell

};

// Simulator Specific Functions
void rotate(Maze* maze, Direction desired) {
  
  int cur  = static_cast<int>(maze->mouse_dir);
  int want = static_cast<int>(desired);
  int diff = (want - cur + 4) % 4;

  if (diff == 0) return; // No rotation needed
  if (diff == 1) {
    API::turnRight();
    maze->mouse_dir = static_cast<Direction>(want);
  } 
  else if (diff == 2) {
    // turn around: two rights
    API::turnRight();
    API::turnRight();
    maze->mouse_dir = static_cast<Direction>(want);
  } 
  else if (diff == 3) {
    API::turnLeft();
    maze->mouse_dir = static_cast<Direction>(want);
  }

};

bool canMoveForward(const Maze* maze) {
  Coord cur = maze->mouse_pos;
  Direction d = maze->mouse_dir;
  
  // if there's a wall in front according to the map OR out of bounds, don't try
  if (cur.x < 0 || cur.x >= mazeSize || cur.y < 0 || cur.y >= mazeSize) return false;
  if (maze->cellWalls[cur.y][cur.x] & directionBitmask(d)) return false;
  
  // compute target cell and check bounds
  int tx = cur.x, ty = cur.y;
  if (d == NORTH) ty += 1;
  else if (d == SOUTH) ty -= 1;
  else if (d == EAST) tx += 1;
  else if (d == WEST) tx -= 1;

  if (tx < 0 || tx >= mazeSize || ty < 0 || ty >= mazeSize) return false;
  return true;

}

void updateMousePos(Coord* pos, Direction dir) {
  // depending on the mouse direction, increment position by one
  if (dir == NORTH)
    pos->y += 1;
  else if (dir == SOUTH)
    pos->y -= 1;
  else if (dir == EAST)
    pos->x += 1;
  else if (dir == WEST)
    pos->x -= 1;
}

bool safeForwardMove(Maze* maze) {
  if (!canMoveForward(maze)) return false;
  API::moveForward(1);
  updateMousePos(&maze->mouse_pos, maze->mouse_dir);
  return true;
}

void scanWalls(Maze* maze) { // fill in code for changing value of the cell walls
  Coord p = maze -> mouse_pos;
  Direction md = maze -> mouse_dir;
  
  // helper to set wall at (x,y) in direction d and also set opposite wall in neighbor if inside bounds
  auto setWallAt = [&](int x, int y, int d) {
    if (x < 0 || x >= mazeSize || y < 0 || y >= mazeSize) return;
    maze->cellWalls[y][x] |= directionBitmask(d);
    // set opposite in neighbor
    int nx = x, ny = y;
    if (d == NORTH) ny += 1;
    else if (d == EAST) nx += 1;
    else if (d == SOUTH) ny -= 1;
    else if (d == WEST) nx -= 1;
    if (nx >= 0 && nx < mazeSize && ny >= 0 && ny < mazeSize) {
        int opp = (d + 2) % 4;
        maze->cellWalls[ny][nx] |= directionBitmask(opp);
    }
  };

  // check each direction for walls. If no wall, add to neighbor list with appropriate coordinates and direction.
  if (API::wallFront()) {
    int d = md; // front is current orientation
    setWallAt(p.x, p.y, d);
    log("Wall in Front. ");
  }
  if (API::wallRight()) {
    int d = (md + 1) % 4;
    setWallAt(p.x, p.y, d);
    log("Wall on Right. ");
  }
  if (API::wallLeft()) {
    int d = (md + 3) % 4;
    setWallAt(p.x, p.y, d);
    log("Wall on Left. ");
  }

}

void updateSimulator(Maze maze) { // redraws the maze in simulator after each loop in main
  // clear previous text so numbers don't pile up
  API::clearAllText();

  // Iterate through each cell in the maze and update the simulator display based on the current state of the maze
  for (int x = 0; x < mazeSize; x++){
    for (int y = 0; y < mazeSize; y++){
      // if there's a wall in a direction according to the map, set it in the simulator
      if (maze.cellWalls[y][x] & NORTH_WALL)
        API::setWall(x, y, 'n');
      if (maze.cellWalls[y][x] & EAST_WALL)
        API::setWall(x, y, 'e');
      if (maze.cellWalls[y][x] & SOUTH_WALL)
        API::setWall(x, y, 's');
      if (maze.cellWalls[y][x] & WEST_WALL)
        API::setWall(x, y, 'w');

      // show distance number (blank for unknown / -1)
      if (maze.distance[y][x] >= 0) {
        API::setText(x, y, std::to_string(maze.distance[y][x]));

      }
    }
  }
}

// Bool helper to check if a cell is a goal cell based on the list of goal cells
bool isGoalCell(const Coord& pos, const vector<Coord>& goalCells) {
  for (const Coord& g : goalCells)
    if (g.x == pos.x && g.y == pos.y) return true;
  return false;
}

// Phase management function to set active targets based on current phase of the run
void setPhaseTargets(RunPhase phase, const Coord& startPos, const vector<Coord>& centerGoals, vector<Coord>& activeTargets) {
  activeTargets.clear();

  switch(phase) {
    case PHASE_EXPLORE_TO_GOAL:
      activeTargets = centerGoals;
      break;

    case PHASE_RETURN_TO_START:
      activeTargets.push_back(startPos);
      break;

    case PHASE_EXPLORE_FULL:
      // Temporary behavior: explore back toward center again.
      // Later, this can be replaced with unexplored/frontier cells.
      activeTargets = centerGoals;
      break;

    case PHASE_SPEEDRUN:
      activeTargets = centerGoals;
      break;

    case PHASE_INIT:
    case PHASE_DONE:
    default:
      activeTargets = centerGoals;
      break;
  }
}

bool reachedPhaseTarget(RunPhase phase, const Coord& mousePos, const Coord& startPos, const vector<Coord>& centerGoals) {
  switch(phase) {
    case PHASE_EXPLORE_TO_GOAL:
      return isGoalCell(mousePos, centerGoals);

    case PHASE_RETURN_TO_START:
      return mousePos == startPos;

    case PHASE_EXPLORE_FULL:
      return isGoalCell(mousePos, centerGoals);

    case PHASE_SPEEDRUN:
      return isGoalCell(mousePos, centerGoals);

    default:
      return false;
  }
}

void updatePhase(RunPhase& phase) {
  switch(phase) {
    case PHASE_EXPLORE_TO_GOAL:
      log("Initial exploration complete. Returning to start.\n");
      phase = PHASE_RETURN_TO_START;
      break;

    case PHASE_RETURN_TO_START:
      log("Returned to start. Beginning full exploration phase.\n");
      phase = PHASE_EXPLORE_FULL;
      break;

    case PHASE_EXPLORE_FULL:
      log("Explore maze phase complete. Beginning speedrun.\n");
      phase = PHASE_SPEEDRUN;
      break;

    case PHASE_SPEEDRUN:
      log("Speedrun complete.\n");
      phase = PHASE_DONE;
      break;

    default:
      phase = PHASE_DONE;
      break;
  }
}

// BFS-based flood-fill algorithm to compute distances from all cells to the goal (or start) based on current maze map
void floodFill(Maze* maze, queue<Coord>& q, const vector<Coord>& targets){
  
  // Initialize all distances to -1 (unvisited)
  for (int y = 0; y < mazeSize; ++y){
    for (int x = 0; x < mazeSize; ++x){
      bool isGoalCell = false;
      for (const Coord& target : targets) {
        if (target.x == x && target.y == y) {
          isGoalCell = true;
          break;
        }
      }
      maze->distance[y][x] = isGoalCell ? 0 : -1;
    }
  }
      
  // Clear and reseed the queue
  while(!q.empty()) q.pop();

  // Seed the queue with the target cells (goals or start depending on phase)
  for(const Coord& target : targets) {
    
    if(target.x < 0 || target.x >= mazeSize || target.y < 0 || target.y >= mazeSize) {
      continue;
    }

    maze -> distance[target.y][target.x] = 0;
    q.push(target);

  }

  while (!q.empty()) {
    Coord cur = q.front();
    q.pop();

    int curDist = maze -> distance[cur.y][cur.x];

    CellList neighbors;
    getNeighborCells(maze, cur, neighbors);
    
    for (int i = 0; i < neighbors.size; i++) {
      Coord npos = neighbors.cells[i].pos;
      if (npos.x < 0 || npos.x >= mazeSize || npos.y < 0 || npos.y >= mazeSize) continue;

      if (maze->distance[npos.y][npos.x] == -1) {
        maze->distance[npos.y][npos.x] = curDist + 1;
        q.push(npos);
      }
    }
  }
}
