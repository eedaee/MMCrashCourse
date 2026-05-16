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

enum Direction {
  NORTH, 
  EAST, 
  SOUTH, 
  WEST
};

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
int MAX_COST = 255;
int CELL_SIZE = 16;

const int NORTH_WALL = 0b1000;
const int EAST_WALL  = 0b0100;
const int SOUTH_WALL = 0b0010;
const int WEST_WALL  = 0b0001;

// Functions for Simulator / Real Mouse
// Differentiate between accessible cells and cells blocked by walls
//Cell* getNeighborCell();

// Returns best accessible cell for the mouse to move to
//Cell getBestCell();

// Direction functions to return direction after step rotation
//Direction cwStep();
//Direction ccwStep();

//Sets a Certain cell position as target cell
//void setGoalCell();

// Simulator Specific Functions
//void rotate();
//void move();
//void updateSim();

int main(){
  // Creating Maze Structure as Maze
  Maze maze1;

  // Creating mouse and it's parameters
  Cell mouse;
  mouse.pos = {15,0}; // Starting Position, with Walls on Left and Bottom side (Lower LH start)
  mouse.dir = NORTH;
  mouse.blocked = false;

  // Initiallize Max Cost value for all cells, and Goal Cell values
  for(int i = 0; i < CELL_SIZE; i++){
    for(int j = 0; j < CELL_SIZE; j++){
      maze1.distances[i][j] = MAX_COST; // Initialize all cells with Max Cost value (255)
      maze1.cellWalls[i][j] = 0b0000; // No walls in any direction (N, E, S, W)
    };
  };

  //Adding boundary walls so mouse cannot go out of bounds
  for(int i = 0; i < CELL_SIZE; i++){             // Using CELL_SIZE=16 to set walls
    maze1.cellWalls[i][0]  = NORTH_WALL;          // South wall for bottom row
    maze1.cellWalls[i][CELL_SIZE-1] = SOUTH_WALL; // North wall for top row
    maze1.cellWalls[0][i]  = WEST_WALL;           // West wall for left column
    maze1.cellWalls[CELL_SIZE-1][i] = EAST_WALL;  // East wall for right column
  };
  
  // Queue, head,and tail initialization
  queue<Coord> q;
  int head, tail = 0;

  //Set Goal Cell (Center 2x2 Square)
  vector<Coord> goalCells = {{7,7}, {7,8}, {8,7}, {8,8}};

  // Initialize Goal Cell values to 0, and add them to the queue
  for(Coord goal : goalCells){
    maze1.distances[goal.x][goal.y] = 0; // Set distance of goal cells to 0
    q.push(goal);                        // Enqueue goal cells
  }

  // Direction Movement Arrays
  // Represents changes in x + y coords for each direction (N, E, S, W)
  int dx[4] = {0, 1, 0, -1};
  int dy[4] = {1, 0, -1, 0};

  //FloodFill Algo main loop
  while(!q.empty()){
    Coord currentCell = q.front(); // Dequeue the front cell
    q.pop();

    // Check each of the 4 directions (N, E, S, W)
    for(int dir = 0; dir < 4; dir++){
      int newX = currentCell.x + (dir == EAST) - (dir == WEST); // Calculate new x coordinate
      int newY = currentCell.y + (dir == SOUTH) - (dir == NORTH); // Calculate new y coordinate

      // Check if the new cell is within bounds and not blocked by a wall
      if(newX >= 0 && newX < CELL_SIZE && newY >= 0 && newY < CELL_SIZE &&
         !(maze1.cellWalls[currentCell.x][currentCell.y] & (1 << (3 - dir)))) { // Check for wall in the direction
        if(maze1.distances[newX][newY] > maze1.distances[currentCell.x][currentCell.y] + 1){
          maze1.distances[newX][newY] = maze1.distances[currentCell.x][currentCell.y] + 1; // Update distance
          q.push({newX, newY}); // Enqueue the new cell
          tail++;
        }
      }
    }
  }

  // Print the distance grid
  cout << "Floodfill Distance Map:" << endl;
  for(int i = 0; i < CELL_SIZE; i++){
    for(int j = 0; j < CELL_SIZE; j++){
      cout << setw(3) << maze1.distances[i][j] << "\t"; // Print distance with tab spacing
    };
    cout << "\n\n";
  };
  
  
  return 1;
}

// Function Definitions
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


*/



/*
Summary of Maze Functions

int mazeWidth();
int mazeHeight();

bool wallFront(int numHalfSteps = 1);
bool wallRight(int numHalfSteps = 1);
bool wallLeft(int numHalfSteps = 1);
bool wallBack(int numHalfSteps = 1);

// Both of these commands can result in "crash"
void moveForward(int distance = 1);
void moveForwardHalf(int numHalfSteps = 1);

void turnRight();
void turnLeft();
void turnRight45();
void turnLeft45();

void setWall(int x, int y, char direction);
void clearWall(int x, int y, char direction);

void setColor(int x, int y, char color);
void clearColor(int x, int y);
void clearAllColor();

void setText(int x, int y, string text);
void clearText(int x, int y);
void clearAllText();

bool wasReset();
void ackReset();

int/float getStat(string stat);

*/