#include <cstdio>
#include <iostream>
#include <climits>
#include <queue>
#include <vector>
#include <thread>
#include <chrono>
#include "API.h"

#define COMP_SIZE 16
#define SIM_SIZE  5
#define MAX_COST  255

using namespace std;

// !!! __CHANGE THIS VARIABLE__ to switch between 16x16 maze (COMP_SIZE) and 5x5 maze (SIM_SIZE)
const int mazeSize = COMP_SIZE; 

// Logging function for simulation print outputs
void log (const string& text){
  cerr << text;
}

// -------------------------------  Data Structures  -------------------------------
// Structure (Coordinates and Cell Values) and Enum (Direction) for mouse
struct Coord{
  int x;
  int y;
};

// Direction Enum for mouse orientation
enum Direction {
  NORTH,   EAST, 
  SOUTH,   WEST
};

// Function to get the name of a direction, or the initial of the direction (for API)
const char* getDirectionName(Direction d) {
  switch (d) {
    case NORTH: return "NORTH"; case EAST:  return "EAST";
    case SOUTH: return "SOUTH"; case WEST:  return "WEST";
    default:    return "UNKNOWN";
  }
}

const char getDirInitial(int d) {
  switch (d) {
    case NORTH: return 'n'; case EAST:  return 'e';
    case SOUTH: return 's'; case WEST:  return 'w';
    default: return '?';
  }
}

enum DirectionBitmask {
  NORTH_WALL = 0b1000,     EAST_WALL  = 0b0100,
  SOUTH_WALL = 0b0010,     WEST_WALL  = 0b0001
};

// Cell and Maze Structures, and Queue for floodfill
struct Cell{
  Coord pos;
  Direction dir;
  bool blocked;
};

struct CellList {
  int size;
  Cell cells[4];
};

struct Maze{
  // mouse position and direction
  int distance[mazeSize][mazeSize];
  int cellWalls[mazeSize][mazeSize];
  Coord mouse_pos;
  Direction mouse_dir;
  Coord goalPos;
  bool mouse_blocked; // flag to indicate if the mouse is blocked by a wall in front of it
};

struct Queue {
  // Cell object array with max size
  // Two int objects for head and tail
  Cell cells[mazeSize * mazeSize];
  int head;
  int tail;
};


// ---------------------  Functions for Floodfill operations   --------------------
void setGoalCell(int size, int tail, Maze& maze, vector<Coord>& goalCells, queue<Coord>& q);
void getNeighborCells(const Maze* maze, Coord c, CellList& out);

void rotate(Maze* maze, Direction desired);
bool canMoveForward(const Maze* maze);
void updateMousePos(Coord* pos, Direction dir);
bool safeForwardMove(Maze* maze);
void scanWalls(Maze* maze);
void updateSimulator(Maze maze);
void floodFill(Maze* maze, queue<Coord>& q, bool to_start);


// ------------------------------- Main Function -------------------------------

int main(){

  // Creating Maze Structure as Maze, as well as mouse and it's parameters
  Maze maze;
  vector<Coord> goalCells;            // Vector to store goal cell coordinates
  maze.mouse_pos = {0, 0};            // Starting Position, bottom-left corner of the maze
  maze.mouse_dir = NORTH;             // Starting Direction, facing North 
  Coord startPos = maze.mouse_pos;     // Store the starting position to return to later

  maze.mouse_blocked = false;         // No blocks at start
  bool returning = false;             // Flag to indicate if mouse is returning to start after reaching goal
  bool speedrun = false;              // Flag to indicate if mouse is in speedrun phase after returning to start

  int moveCount = 0; // Counter for number of moves taken by the mouse 
  int moveMax = 300; // Max limit to exit FloodFill loop 

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
  std::this_thread::sleep_for(std::chrono::milliseconds(1500)); 

  //Adding boundary walls so mouse cannot go out of bounds
  for(int i = 0; i < mazeSize; i++){               // Using mazeSize to set walls
    //maze.cellWalls[i][0]  |= NORTH_WALL;          // North wall for top row
    API::setWall(i, mazeSize - 1, getDirInitial(NORTH));
    //maze.cellWalls[i][mazeSize-1] |= SOUTH_WALL;  // South wall for bottom row
    API::setWall(i, 0, getDirInitial(SOUTH));
    //maze.cellWalls[0][i]  |= WEST_WALL;           // West wall for left column
    API::setWall(0, i, getDirInitial(WEST));
    //maze.cellWalls[mazeSize-1][i] |= EAST_WALL;   // East wall for right column
    API::setWall(mazeSize - 1, i, getDirInitial(EAST));
  };

  // Sleep for a moment to allow the simulator to update with the boundary walls before starting the algorithm
  std::this_thread::sleep_for(std::chrono::milliseconds(1500)); 

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

  //Initialize Goal Cell, Set Goal Cell values, add them to queue
  setGoalCell(mazeSize, tail, maze, goalCells, q);
  // Store center position for later speedrun phase
  Coord centerPos = maze.goalPos; 

  updateSimulator(maze); // Update the simulator with the initial state of the maze before starting the floodfill algorithm

  // // ----------------------- FloodFill Algo main loop -----------------------
  while(moveCount++ < moveMax) {
    // Check if queue is empty, if so, break out of loop
    if (q.empty()) {
      log("Queue is empty. No path to goal cell found.\n");
      break;
    }

    floodFill(&maze, q, false); // Perform floodfill to explore the maze and find path to goal cell
    updateSimulator(maze); // Update the simulator with the current state of the maze after floodfill
    scanWalls(&maze); // Scan for walls and update the maze's internal representation

    // Check if mouse has reached the goal cell
    if (maze.mouse_pos.x == maze.goalPos.x && maze.mouse_pos.y == maze.goalPos.y) {
      if(!returning && !speedrun){
        API::setText(maze.mouse_pos.x, maze.mouse_pos.y, "GOAL");
        returning = true; // Set flag to indicate mouse is now returning to start
        maze.goalPos = startPos; // Update goal position to starting position for return trip
        continue; // Continue to next iteration to start returning to start
      }
      else if(returning && !speedrun){
        API::setText(maze.mouse_pos.x, maze.mouse_pos.y, "START");
        returning = false;        // Clear returning flag
        speedrun = true;          // Set speedrun flag to indicate mouse is now in speedrun phase
        maze.goalPos = centerPos; // Update goal position to center for speedrun phase
        continue; // Continue to next iteration to start speedrun to center
      }
      else if(speedrun && !returning){
        API::setText(maze.mouse_pos.x, maze.mouse_pos.y, "SPEEDRUN INITIALIZED");
        break;
      }
    }
    
    // Choose neihbor cell with lowest distance to goal and move mouse towards it, if not blocked by a wall
    CellList neighbors;
    getNeighborCells(&maze, maze.mouse_pos, neighbors); // Get neighbors of the current cell using the getNeighborCells function

    // Find the best neighbor cell with the lowest distance to the goal
    int bestIdx = -1;
    int bestDistance = maze.distance[maze.mouse_pos.y][maze.mouse_pos.x]; // Distance of current cell to goal
    
    for(int i = 0; i < neighbors.size; i++){
      // Get coordinates of the neighbor cell
      Coord npos = neighbors.cells[i].pos; 

      // Skip out-of-bounds neighbors
      if(npos.x < 0 || npos.x >= mazeSize || npos.y < 0 || npos.y >= mazeSize) continue;
      // Get distance of neighbor cell to goal
      int d = maze.distance[npos.y][npos.x]; 
      // Check if the neighbor cell is blocked by a wall in the direction of the neighbor
      if(!neighbors.cells[i].blocked){ // Only consider accessible cells
        int neighborDistance = maze.distance[npos.y][npos.x]; // Get distance of neighbor cell to goal
        if(d != -1 && neighborDistance < bestDistance){ // Update best cell if neighbor is closer to goal
          bestDistance = neighborDistance;
          bestIdx = i;
        }
      }
    }

    if(bestIdx == -1){
      log("No accessible neighbors found. Mouse is blocked.\n");
      break; // Exit loop if no accessible neighbors are found
    }

    // Rotate mouse to face the best neighbor cell and move forward if not blocked
    // Get the direction of the best neighbor cell
    Direction desired = neighbors.cells[bestIdx].dir; 
    rotate(&maze, desired); // Rotate mouse to face the best neighbor cell
    if(!safeForwardMove(&maze)){ // Move forward if not blocked, otherwise log that mouse is blocked and exit loop
      log("Mouse is blocked by a wall. Cannot move forward.\n");
      API::setText(maze.mouse_pos.x, maze.mouse_pos.y, "ERR");
      break;
    }

    // Update the simulator
    updateSimulator(maze);

  }

  return 0;

}

// ------------------------------- ------------------------------- -------------------------------
// Maze Function Definitions
void setGoalCell(int size, int tail, Maze& maze, vector<Coord>& goalCells, queue<Coord>& q) {
  switch(size){
    case SIM_SIZE:
      goalCells = {{2,2}};                         // Single center square for 5x5 maze
      for (Coord goal : goalCells) {               // Set distance of goal cell to 0, print and add to queue
        maze.distance[goal.x][goal.y] = 0;
        //printf("Goal Cell: (%d, %d) = %d\n", goal.x, goal.y, maze.distances[goal.x][goal.y]);
        API::setText(goal.x, goal.y, "0");
        q.push(goal);
        tail++;
      }
      // log("Tail Size: "); cerr << tail  << "\n";;
    break;
    
    case COMP_SIZE:
      goalCells = {{7,7}, {7,8}, {8,7}, {8,8}};;  // Center 2x2 square for 16x16 maze
      for (Coord goal : goalCells) {               // Set distance of goal cell to 0, print and add to queue
        maze.distance[goal.x][goal.y] = 0;
        //printf("Goal Cells: (%d, %d) = %d\n", goal.x, goal.y, maze.distances[goal.x][goal.y]);
        API::setText(goal.x, goal.y, "0");
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
void getNeighborCells(const Maze* maze, Coord c, CellList& out) {
  out.size = 0;

  //  check each direction for walls. If no wall, add to neighbor list with appropriate coordinates and direction.
  for (int d = 0; d < 4; d++) {

    int wall_mask = DirectionBitmask(d);

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

  Cell bestCell = mouse;                                  // Initialize best cell as current cell
  int bestDistance = maze.distance[mouse.pos.x][mouse.pos.y];  // Distance of current cell to goal

  // Check each neighbor cell and update best cell if it's closer to the goal
  for(int dir = 0; dir < 4; dir++){
    
    // Check if the neighbor cell is accessible (not blocked by a wall)
    Cell neighbor = neighbors.cells[dir];

    if(!neighbor.blocked){ // Only consider accessible cells
      
      int neighborDistance = maze.distance[neighbor.pos.x][neighbor.pos.y]; // Get distance of neighbor cell to goal
    
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
  if (maze->cellWalls[cur.y][cur.x] & DirectionBitmask(d)) return false;
  
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
    maze->cellWalls[y][x] |= DirectionBitmask(d);
    // set opposite in neighbor
    int nx = x, ny = y;
    if (d == NORTH) ny += 1;
    else if (d == EAST) nx += 1;
    else if (d == SOUTH) ny -= 1;
    else if (d == WEST) nx -= 1;
    if (nx >= 0 && nx < mazeSize && ny >= 0 && ny < mazeSize) {
        int opp = (d + 2) % 4;
        maze->cellWalls[ny][nx] |= DirectionBitmask(opp);
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
  log("\n");

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

void floodFill(Maze* maze, queue<Coord> q, bool to_start) { // BFS-based flood-fill
  
  // initialize distances to -1 (unvisited)
  for (int y = 0; y < mazeSize; ++y)
    for (int x = 0; x < mazeSize; ++x)
      maze -> distance[y][x] = -1;

  // determine starting point for flood-fill based on whether we're filling towards the start or the goal
  Coord start = to_start ? maze -> mouse_pos : maze -> goalPos;
  
  //clear and receed the queue with the starting cell and set its distance to 0
  while(!q.empty()) q.pop(); // Clear the queue

  // seed the queue with the starting cell and set its distance to 0
  if (start.x < 0 || start.x >= mazeSize || start.y < 0 || start.y >= mazeSize) return;
  
  maze -> distance[start.y][start.x] = 0;
  q.push(start);

  while (!q.empty()) {
    // get the front cell from the queue
    Coord cur = q.front();
    q.pop();

    // extract coordinates and current distance of mouse
    int curDist = maze -> distance[cur.y][cur.x];

    // get neighbors of the current cell using the getNeighborCells function
    CellList neighbors;
    getNeighborCells(maze, cur, neighbors);
    
    // for each neighbor, if it's unvisited and accessible, set distance and add to queue
    for (int i = 0; i < neighbors.size; ++i) {
      Coord npos = neighbors.cells[i].pos;
      // bounds are already checked by getNeighborCells, but double-check
      if (npos.x < 0 || npos.x >= mazeSize || npos.y < 0 || npos.y >= mazeSize)
        continue;

      if (maze->distance[npos.y][npos.x] == -1) {
        maze->distance[npos.y][npos.x] = curDist + 1;
        q.push(npos);
      }
    }
  }
}
