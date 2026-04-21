#include <iostream>
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
  
};


struct Maze{
  int distances[16][16];
  int cellWalls[16][16];
  Coord* goalPos;
};


int main(){
  
  
  
  return 0;
}

/*
OLD TEST CODE
  int arr[4] = {NORTH, EAST, SOUTH, WEST};

  for(int i=0; i<4; i++){
    cout << arr[i] << endl;
  }
  
  return 0;
  */
  