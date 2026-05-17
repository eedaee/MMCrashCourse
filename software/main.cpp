#include <iostream>
#include <iomanip>
#include <queue>
#include <vector>
using namespace std;

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

// Cell and Maze Structures
struct Cell{
  Coord pos;
  Direction dir;
  bool blocked;
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

const int NORTH_WALL = 0b1000;
const int EAST_WALL  = 0b0100;
const int SOUTH_WALL = 0b0010;
const int WEST_WALL  = 0b0001;

void setGoalCell(int size, int &tail, Maze& maze, vector<Coord>& goalCells, queue<Coord>& q);

// Functions for Simulator / Real Mouse
// Differentiate between accessible cells and cells blocked by walls
//Cell* getNeighborCell();

// Returns best accessible cell for the mouse to move to
//Cell getBestCell();

// Direction functions to return direction after step rotation
//Direction cwStep();
//Direction ccwStep();

//Sets a Certain cell position as target cell


// Simulator Specific Functions
//void rotate();
//void move();
//void updateSim();

int main(){
  // Creating Maze Structure as Maze
  Maze maze1;
  int mazeSize = COMP_SIZE; // !!!Using COMP_SIZE (16) for the maze size, can be changed to SIM_SIZE (5) for testing
  vector<Coord> goalCells;  // Vector to store goal cell coordinates

  // Creating mouse and it's parameters
  Cell mouse;
  mouse.pos = {mazeSize - 1, 0};  // Starting Position, bottom-left corner of the maze
  mouse.dir = NORTH;              // Starting Direction, facing North
  mouse.blocked = false;          // No blocks at start 

  int moveCount = 0; // Counter for number of moves taken by the mouse
  int maxMoves = 300; // Maximum number of moves to prevent infinite loops in case of errors

  // Maze Initialization [Distances and Walls]
  // Initiallize Max Cost value for all cells, and Goal Cell values
  for(int i = 0; i < mazeSize; i++){
    for(int j = 0; j < mazeSize; j++){
      maze1.distances[i][j] = MAX_COST; // Initialize all cells with Max Cost value (255)
      maze1.cellWalls[i][j] = 0b0000; // No walls in any direction (N, E, S, W)
    };
  };

  //Adding boundary walls so mouse cannot go out of bounds
  for(int i = 0; i < mazeSize; i++){             // Using CELL_SIZE=16 to set walls
    maze1.cellWalls[i][0]  |= NORTH_WALL;          // South wall for bottom row
    maze1.cellWalls[i][mazeSize-1] |= SOUTH_WALL; // North wall for top row
    maze1.cellWalls[0][i]  |= WEST_WALL;           // West wall for left column
    maze1.cellWalls[mazeSize-1][i] |= EAST_WALL;  // East wall for right column
  };

  // Test to check if walls are set correctly
  if(maze1.cellWalls[0][0] == 0b1001 && maze1.cellWalls[0][15] == 0b0011 && maze1.cellWalls[15][0] == 0b1100 && maze1.cellWalls[15][15] == 0b0110){
    cout << "Walls initialized correctly\n" << endl;
  } else {
    cout << "Error setting walls.\n" << endl;
  }

  // Print initial state of the mouse
  cout << "Starting Micromouse Floodfill Simulation" << endl;
  cout << "Start Position: (" << mouse.pos.x << ", " << mouse.pos.y << ")" << endl;
  cout << "Start Direction: " << getDirectionName(mouse.dir) << endl;
  cout << endl;

  // -------------------------------------------------------------------------------
  // Queue, head,and tail initialization
  queue<Coord> q;
  int head, tail = 0;

  //Initialize Goal Cell, Set Goal Cell values, add them to queue
  setGoalCell(mazeSize, tail, maze1, goalCells, q);

  // Direction Movement Arrays
  // Represents changes in x + y coords for each direction (N, E, S, W)
  int dx[4] = {0, 1, 0, -1};
  int dy[4] = {1, 0, -1, 0};

  //FloodFill Algo main loop
  while(!q.empty()){
    Coord currentCell = q.front(); // Dequeue the front cell
    q.pop();                       // Increment head (included for clarity)
    
    // Check each of the 4 directions (N, E, S, W)
    for(int dir = 0; dir < 4; dir++){
      int newX = currentCell.x + (dir == EAST) - (dir == WEST); // Calculate new x coordinate
      int newY = currentCell.y + (dir == SOUTH) - (dir == NORTH); // Calculate new y coordinate

      // Check if the new cell is within bounds and not blocked by a wall
      if(newX >= 0 && newX < mazeSize && newY >= 0 && newY < mazeSize &&!(maze1.cellWalls[currentCell.x][currentCell.y] & (1 << (3 - dir)))) { 
        // Check for wall in the direction
        if(maze1.distances[newX][newY] > maze1.distances[currentCell.x][currentCell.y] + 1){
          maze1.distances[newX][newY] = maze1.distances[currentCell.x][currentCell.y] + 1; // Update distance
          q.push({newX, newY}); // Enqueue the new cell
          tail++;
        }
      }
    }
    // move mouse 
    

  }
  
 

  // Print the distance grid
  cout << "Floodfill Distance Map:" << endl;
  for(int i = 0; i < mazeSize; i++){
    for(int j = 0; j < mazeSize; j++){
      cout << setw(3) << maze1.distances[i][j] << "\t"; // Print distance with tab spacing
    };
    cout << "\n\n";
  };
  
  
  return 0;
  

}

// Function Definitions
void setGoalCell(int size, int &tail, Maze& maze, vector<Coord>& goalCells, queue<Coord>& q){
  switch(size){
   
    case SIM_SIZE:
      goalCells = {{2,2}};                         // Single center square for 5x5 maze
      for (Coord goal : goalCells) {               // Set distance of goal cell to 0, print and add to queue
        maze.distances[goal.x][goal.y] = 0;
        cout << "Goal Cell: (" << goal.x << ", " << goal.y << ") = " << maze.distances[goal.x][goal.y] << endl;
        q.push(goal);
        tail++;
      }
      cout << "Tail Size: " << tail << endl << endl;
    break;
    
    case COMP_SIZE:
      goalCells = {{7,7}, {7,8}, {8,7}, {8,8}};;  // Center 2x2 square for 16x16 maze
      for (Coord goal : goalCells) {               // Set distance of goal cell to 0, print and add to queue
        maze.distances[goal.x][goal.y] = 0;
        cout << "Goal Cells: (" << goal.x << ", " << goal.y << ") = " << maze.distances[goal.x][goal.y] << endl;
        q.push(goal);
        tail++;
      }
      cout << "Tail Size: " << tail << endl << endl;
    break;

      default:
        cout << "Invalid maze size. Please use SIM_SIZE (5) or COMP_SIZE (16)." << endl;
      break;
    }
};
/*
Cell* getNeighborCell(){

};

*/

/*
OLD TEST CODE
// Testing enum Direction;
  int arr[4] = {NORTH, EAST, SOUTH, WEST};

  for(int i=0; i<4; i++){
    cout << arr[i] << endl;
  }
  
  return 0;

// Testing Max Cost initialization for all cells
// Initiallize Max Cost value for all cells, and Goal Cell values
  for(int i = 0; i < CELL_SIZE; i++){
    for(int j = 0; j < CELL_SIZE; j++){
      maze1.distances[i][j] = MAX_COST;
      cout << maze1.distances[i][j] << " " << endl;
    };
    cout << "\n";
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
    cout << "Goal Cell: (" << goal.x << ", " << goal.y << ") = " << maze1.distances[goal.x][goal.y] << endl;
    //q.push(goal);       // Enqueue goal cells
    //tail++;            // Increment tail for each goal cell added
    //cout << "Tail Size: " << tail << endl;
  }

  
*/
