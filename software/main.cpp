#include <cstdio>
#include <iostream>
#include <iomanip>
#include <queue>
#include <vector>
#include "API.h"
using namespace std;

void log (const string& text){
  cerr << text;
}

// Structure (Coordinates and Cell Values) and Enum (Direction) for mouse
struct Coord{
  int x;
  int y;
};

// Direction Enum for mouse orientation
enum Direction {
  NORTH, 
  EAST, 
  SOUTH, 
  WEST
};

// Function to get the name of a direction (for debugging purposes)
const char* getDirectionName(Direction d) {
  switch (d) {
    case NORTH: return "NORTH";
    case EAST:  return "EAST";
    case SOUTH: return "SOUTH";
    case WEST:  return "WEST";
    default:    return "UNKNOWN";
  }
}

const int NORTH_WALL = 0b1000;
const int EAST_WALL  = 0b0100;
const int SOUTH_WALL = 0b0010;
const int WEST_WALL  = 0b0001;

// Cell and Maze Structures
struct Cell{
  Coord pos;
  Direction dir;
  bool blocked;
};

struct CellList {
    int size;
    Cell* cells;
};

struct Maze{
  int distances[16][16];
  int cellWalls[16][16];
  Coord* goalPos;
};

// Global Values
const int MAX_COST  = 255;
const int COMP_SIZE = 16;
const int SIM_SIZE  = 5;

void setGoalCell(int size, int &tail, Maze& maze, vector<Coord>& goalCells, queue<Coord>& q);

// Functions for Simulator / Real Mouse
// Differentiate between accessible cells and cells blocked by walls
Cell* getNeighborCell(Maze& maze, Cell& currentCell, Direction dir, int mazeSize);

// Returns best accessible cell for the mouse to move to
Cell getBestCell(Maze& maze, Cell& mouse, int mazeSize);

// Direction functions to return direction after step rotation
Direction cwStep(Direction currentDir);
Direction ccwStep(Direction currentDir);

//Sets a Certain cell position as target cell

// Simulator Specific Functions
void rotate(Cell& mouse, Direction targetDir);
void move(Cell& mouse, Cell bestCell);
//void updateSim(Maze& maze, Cell mouse, int mazeSize);

int main(){
  // ------------------------------- Maze & Mouse Initialization -------------------------------
  // Creating Maze Structure as Maze
  Maze maze1;
  int mazeSize = COMP_SIZE; // !!!Using COMP_SIZE (16) for the maze size, can change to SIM_SIZE (5) for testing
  vector<Coord> goalCells;  // Vector to store goal cell coordinates

  // Creating mouse and it's parameters
  Cell mouse;
  mouse.pos = {mazeSize - 1, 0};  // Starting Position, bottom-left corner of the maze
  mouse.dir = NORTH;              // Starting Direction, facing North
  mouse.blocked = false;          // No blocks at start 

  int moveCount = 0; // Counter for number of moves taken by the mouse 
  int moveMax = 300; // Max limit to exit FloodFill loop 

  // Maze Initialization [Distances and Walls]
  // Initiallize Max Cost value for all cells, and Goal Cell values
  for(int i = 0; i < mazeSize; i++){
    for(int j = 0; j < mazeSize; j++){
      maze1.distances[i][j] = MAX_COST; // Initialize all cells with Max Cost value (255)
      maze1.cellWalls[i][j] = 0b0000; // No walls in any direction (N, E, S, W)
    };
  };

  //Adding boundary walls so mouse cannot go out of bounds
  for(int i = 0; i < mazeSize; i++){               // Using mazeSize to set walls
    maze1.cellWalls[i][0]  |= NORTH_WALL;          // South wall for bottom row
    maze1.cellWalls[i][mazeSize-1] |= SOUTH_WALL;  // North wall for top row
    maze1.cellWalls[0][i]  |= WEST_WALL;           // West wall for left column
    maze1.cellWalls[mazeSize-1][i] |= EAST_WALL;   // East wall for right column
  };

  // Test code to check if walls are set correctly
  if(maze1.cellWalls[0][0] == 0b1001 && maze1.cellWalls[0][15] == 0b0011 && maze1.cellWalls[15][0] == 0b1100 && maze1.cellWalls[15][15] == 0b0110){
    log("Walls initialized correctly\n\n");
  } else {
    log("Error setting walls.\n\n");
  }

  // Print initial state of the mouse
  log("Starting Micromouse Floodfill Simulation\n");
  log("Start Position: ("); cerr << mouse.pos.x << ", " << mouse.pos.y << ")\n";
  log("Start Direction: "); cerr << getDirectionName(mouse.dir) << "\n\n";

  // -------------------------------------------------------------------------------
  // Queue, head,and tail initialization
  queue<Coord> floodQueue; // Queue for floodfill algorithm
  int head = 0; int tail = 0;

  //Initialize Goal Cell, Set Goal Cell values, add them to queue
  setGoalCell(mazeSize, tail, maze1, goalCells, floodQueue);

  //FloodFill Algo main loop
  // STILL WORK IN PROGRESS, NEED TO ADD MOUSE MOVEMENT AND WALL SENSING, ALSO NEED TO CHECK FOR WALLS IN EACH DIRECTION
  while((tail - head > 0) || moveCount < moveMax){ // Loop until queue is empty
    
    // Check if mouse has reached the goal cell
    bool reached_goal = false;
    if(reached_goal){
      log("Mouse has reached the goal cell!\n");
      log("Final Position: ("); cerr <<  mouse.pos.x << ", " << mouse.pos.y << ")\n";
      log("Total Moves: : ("); cerr <<  moveCount << ")\n";
      break; // Exit the loop if the goal is reached
    }

    // Check if the mouse's current position matches any of the goal cell coordinates
    for(Coord goal : goalCells){
      if(mouse.pos.x == goal.x && mouse.pos.y == goal.y){
        reached_goal = true;
        break;
      }
    }

    // Get the front cell from the queue
    Coord currentCell = floodQueue.front();
    
    int newCost = maze1.distances[mouse.pos.x][mouse.pos.y] + 1; // Calculate new cost for neighboring cells
    
    // Get neighboring cells (assuming direction doesn't matter for floodfill)
    Cell* neighborCell = getNeighborCell(maze1, mouse, mouse.dir , mazeSize); 

    // Check each neighbor and update distances if a shorter path is found
    for(int dir = 0; dir < 4; dir++){
      if(!neighborCell[dir].blocked){ // Only consider accessible cells

        if(newCost < maze1.distances[neighborCell[dir].pos.x][neighborCell[dir].pos.y]){ // Update distance if a shorter path is found
          maze1.distances[neighborCell[dir].pos.x][neighborCell[dir].pos.y] = newCost; // Update distance to goal for the neighbor cell
          floodQueue.push(neighborCell[dir].pos);  // Enqueue the neighbor cell for further exploration
          tail++;                         // Increment tail for each new cell added to the queue
        }

      }
    }

    // Pick the best cell for the mouse to move to based on the updated distances
    Cell bestCell = getBestCell(maze1, mouse, mazeSize); // Get the best accessible cell for the mouse to move to

    // Rotate the mouse to face the best cell's direction
    rotate(mouse, bestCell.dir);

    // Move the mouse to the best cell
    move(mouse, bestCell);

    // Increment move count after each move
    moveCount++;

    // Update the simulator's display (if applicable)
    //updateSim(maze1, mouse, mazeSize);
    

  }

  return 1;

}

// Function Definitions
void setGoalCell(int size, int &tail, Maze& maze, vector<Coord>& goalCells, queue<Coord>& q){
  switch(size){
   
    case SIM_SIZE:
      goalCells = {{2,2}};                         // Single center square for 5x5 maze
      for (Coord goal : goalCells) {               // Set distance of goal cell to 0, print and add to queue
        maze.distances[goal.x][goal.y] = 0;
        //printf("Goal Cell: (%d, %d) = %d\n", goal.x, goal.y, maze.distances[goal.x][goal.y]);
        q.push(goal);
        tail++;
      }
      // log("Tail Size: "); cerr << tail  << "\n";;
    break;
    
    case COMP_SIZE:
      goalCells = {{7,7}, {7,8}, {8,7}, {8,8}};;  // Center 2x2 square for 16x16 maze
      for (Coord goal : goalCells) {               // Set distance of goal cell to 0, print and add to queue
        maze.distances[goal.x][goal.y] = 0;
        //printf("Goal Cells: (%d, %d) = %d\n", goal.x, goal.y, maze.distances[goal.x][goal.y]);
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

// Differentiate between accessible cells and cells blocked by walls
CellList* getNeighborCells(Maze* maze, Coord c) { //to be called in a while loop within Floodfill when setting each cell
    
};
Cell* getNeighborCell(Maze& maze, Cell& currentCell, Direction dir, int mazeSize){
  static Cell neighborCells[4]; // Static to return pointer to local variable
  
  // Directon vectors for N, E, S, W
  // Assumes pos.x = row index and pos.y = column index, x increases downwards and y increases rightwards
  int dx[4] = {0, 1, 0, -1};
  int dy[4] = {1, 0, -1, 0};

  int wallMask[4] = {NORTH_WALL, EAST_WALL, SOUTH_WALL, WEST_WALL};     // Wall masks for current cell
  int oppWallMask[4] = {SOUTH_WALL, WEST_WALL, NORTH_WALL, EAST_WALL};  // Opposite walls for neighbor cell

  // Check each of the 4 directions (N, E, S, W) and determine if the neighbor cell is accessible or blocked by a wall
  for(int dir = 0; dir < 4; dir++){
    int newX = currentCell.pos.x + dx[dir]; // Calculate new x coordinate
    int newY = currentCell.pos.y + dy[dir]; // Calculate new y coordinate

    neighborCells[dir].pos = {currentCell.pos.x, currentCell.pos.y}; // Initialize neighbor cell position as current cell position
    neighborCells[dir].dir = static_cast<Direction>(dir);            // Set neighbor cell direction
    neighborCells[dir].blocked = false;                              // Default to accessible, will update if accessible

    // Check if the new cell is within bounds and not blocked by a wall
    if(newX < 0 || newX >= mazeSize || newY < 0 || newY >= mazeSize) { // Out of bounds
      neighborCells[dir].blocked = true;                               // Mark as blocked
    }
    else if (maze.cellWalls[currentCell.pos.x][currentCell.pos.y] & wallMask[dir]) { // Blocked by a wall
      neighborCells[dir].blocked = true;                                             // Mark as blocked
    }
    else if (maze.cellWalls[newX][newY] & oppWallMask[dir]) { // Blocked by a wall from the neighbor cell's perspective
      neighborCells[dir].blocked = true;                      // Mark as blocked
    }
    else { // Accessible cell
      neighborCells[dir].pos = {newX, newY}; // Set neighbor cell position
      neighborCells[dir].blocked = false;     // Mark as accessible
    }
  }

  return neighborCells; // Default return value if no valid neighbor is found

};

// Returns best accessible cell for the mouse to move to
Cell getBestCell(Maze& maze, Cell& mouse, int mazeSize){
  Cell* neighbors = getNeighborCell(maze, mouse, mouse.dir, mazeSize); // Get neighbor cells

  Cell bestCell = mouse;                                       // Initialize best cell as current cell
  int bestDistance = maze.distances[mouse.pos.x][mouse.pos.y]; // Distance of current cell to goal

  // Check each neighbor cell and update best cell if it's closer to the goal
  for(int dir = 0; dir < 4; dir++){
    Cell neighbor = neighbors[dir];
    if(!neighbor.blocked){ // Only consider accessible cells
      int neighborDistance = maze.distances[neighbor.pos.x][neighbor.pos.y]; // Get distance of neighbor cell to goal
    
      if(neighborDistance < bestDistance){ // Update best cell if it's closer to the goal
      bestCell = neighbor;
      bestDistance = neighborDistance;
      }
    
    }
  }

  return bestCell; // Return the best accessible cell

};

// Direction functions to return direction after step rotation
Direction cwStep(Direction currentDir){
  return static_cast<Direction>((currentDir + 1) % 4); // Rotate clockwise (N->E->S->W->N)
};

Direction ccwStep(Direction currentDir){
  return static_cast<Direction>((currentDir + 3) % 4); // Rotate counterclockwise (N->W->S->E->N)
};

// Simulator Specific Functions
void rotate(Cell& mouse, Direction targetDir) {
  while(mouse.dir != targetDir) {
    int turnAmount = (targetDir - mouse.dir + 4) % 4; // Calculate the number of steps to turn

    switch(turnAmount) {
      case 1:
        mouse.dir = cwStep(mouse.dir);
        log("Rotate Clockwise -> Facing "); cerr << getDirectionName(mouse.dir) << "\n";
        break;
      case 2:
        mouse.dir = cwStep(mouse.dir);
        mouse.dir = cwStep(mouse.dir);
        log("Rotate Clockwise 2x -> Facing "); cerr << getDirectionName(mouse.dir) << "\n";
        break;
      case 3:
        mouse.dir = ccwStep(mouse.dir);
        log("Rotate CounterClockwise -> Facing "); cerr << getDirectionName(mouse.dir) << "\n";
        break;
      default:
        break;
    }
  }
};

void move(Cell& mouse, Cell bestCell) {
  mouse.pos = bestCell.pos;
  API::moveForward();
  log("To Cell: ("); cerr << mouse.pos.x << ", " << mouse.pos.y << "). ";
  log("Dir: "); cerr << getDirectionName(mouse.dir) << ". \n";
}

// Maze functions
void scanWalls(Maze* maze) { // fill in code for changing value of the cell walls
  if (API::wallFront()) {
    
  }
  if (API::wallRight()) {
    
  }
  if (API::wallLeft()) {
    
  }
}

void updateSimulator(Maze maze) { // redraws the maze in simulator after each loop in main
  for(int x = 0; x < MAZE_SIZE; x++){
    for(int y = 0; y < MAZE_SIZE; y++){
      if (maze.cellWalls[y][x] & NORTH_WALL)
        // API set walls for some direction

      if (maze.cellWalls[y][x] & EAST_WALL)
        // API set walls for some direction

      if (maze.cellWalls[y][x] & SOUTH_WALL)
        // API set walls for some direction

      if (maze.cellWalls[y][x] & WEST_WALL)
        // API set walls for some direction

    }
  }
}

void updateMousePos(Coord* pos, Direction dir) {
  //depending on the mouse direction, increment position by one
  if (dir == NORTH)
    // increment in some direction
  if (dir == SOUTH)
    // increment in some direction
  if (dir == WEST)
    // increment in some direction
  if (dir == EAST)
    // increment in some direction
}

void floodFill(Maze* maze, bool to_start) { // function to be called everytime you move into a new cell
    
}

/*
OLD TEST CODE
// Testing enum Direction;
  int arr[4] = {NORTH, EAST, SOUTH, WEST};

  for(int i=0; i<4; i++){
    printf("%d\n", arr[i]);
  }
  
  return 0;

// Testing Max Cost initialization for all cells
// Initiallize Max Cost value for all cells, and Goal Cell values
  for(int i = 0; i < CELL_SIZE; i++){
    for(int j = 0; j < CELL_SIZE; j++){
      maze1.distances[i][j] = MAX_COST;
      printf("%d \n", maze1.distances[i][j]);
    };
    printf("\n");
  };

  for(int i = 8; i < 10; i++){
    for(int j = 8; j < 10; j++){
      maze1.distances[i][j] = 0;
    };
  };



// while functions with real micromouse
  while (!isGoalCell(mouse.pos)) {
    senseWalls();
    floodfill();
    chooseBestNeighbor();
    moveMouse();
}

  // , and add them to the queue
  for(Coord goal : goalCells){
    printf("Goal Cell: (%d, %d) = %d\n", goal.x, goal.y, maze1.distances[goal.x][goal.y]);
    //q.push(goal);       // Enqueue goal cells
    //tail++;            // Increment tail for each goal cell added
    //printf("Tail Size: %d\n", tail);
  }

  // Print the distance grid
  printf("Floodfill Distance Map:\n");
  for(int i = 0; i < mazeSize; i++){
    for(int j = 0; j < mazeSize; j++){
      printf("%3d\t", maze1.distances[i][j]); // Print distance with tab spacing
    };
    printf("\n\n");
  };
  
  
*/
