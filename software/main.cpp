#include <iostream>
#include <queue>
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


//Global Values
int MAX_COST = 255;
int CELL_SIZE = 16;


int main(){
  // Creating Maze Structure as Maze
  Maze maze1;

  //Creating mouse and it's parameters
  Cell mouse;
  mouse.pos = [0][0];
  mouse.dir = NORTH;

  int head, tail = 0;

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

  //Queue Iteration
  /*
  while(tail - head > 0){
    cur
  }
  */

  // Starting Position, with Walls on Left and Bottom side (Lower LH start)
  int cellWalls = 0b0011;

  // Wall values
  bool northWall = cellWalls & 1000;
  bool eastWall  = cellWalls & 0100;
  bool southWall = cellWalls & 0010;
  bool westWall  = cellWalls & 0001;

  // Mouse Actions
  /*

  function floodfill(maze, goalCells):
	Q = empty queue
	distances = 2D array initialized to infinity
	for cell in goalCells:
		enqueue(Q, cell)
		distances[cell] = 0	
	while Q is not empty:
		currentCell = dequeue(Q)
		for each neighbor of currentCell that is not blocked:
			if distance[neighbor] > distance[currentCell] + 1:
				distance[neighbor] = distance[currentCell] + 1
				enqueue(Q, neighbor)	
			

  PseudoCODE!!
  while(tail - head < 0)

  */
  
  
  return 1;
}

// Function Definitions
/*
Cell* getNeighborCell(){

};

*/

/*
OLD TEST CODE
//Testing enum Direction;
  int arr[4] = {NORTH, EAST, SOUTH, WEST};

  for(int i=0; i<4; i++){
    cout << arr[i] << endl;
  }
  
  return 0;
  */
  