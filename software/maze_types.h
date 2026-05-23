// maze_types.h
#pragma once

#include <vector>
#include <queue>
#include <string>
#include <iostream>
#include <cstring>

#define COMP_SIZE 16
#define SIM_SIZE  5
#define MAX_COST  255

using namespace std;

// !!! __CHANGE THIS VARIABLE__ to switch between 16x16 maze (COMP_SIZE) and 5x5 maze (SIM_SIZE)
const int mazeSize = COMP_SIZE; 

// -------------------------------------------------------
// Logging
// -------------------------------------------------------
inline void log(const string& text) { cerr << text; }

// -------------------------------------------------------
// Direction
// -------------------------------------------------------
enum Direction { NORTH,  EAST,   SOUTH,   WEST };

// Functions to get the name of a direction, or the initial of the direction (for API)
inline const char* getDirectionName(Direction d) {
  switch (d) {
    case NORTH: return "NORTH"; case EAST:  return "EAST";
    case SOUTH: return "SOUTH"; case WEST:  return "WEST";
    default:    return "UNKNOWN";
  }
}

inline const char getDirInitial(int d) {
  switch (d) {
    case NORTH: return 'n'; case EAST:  return 'e';
    case SOUTH: return 's'; case WEST:  return 'w';
    default: return '?';
  }
}

// -------------------------------------------------------
// Wall Bitmasks
// -------------------------------------------------------
enum wallBit{
  NORTH_WALL = 0b1000,     EAST_WALL  = 0b0100,
  SOUTH_WALL = 0b0010,     WEST_WALL  = 0b0001
};

inline int directionBitmask(int d) {
    switch (d) {
        case NORTH: return NORTH_WALL; case EAST:  return EAST_WALL;
        case SOUTH: return SOUTH_WALL; case WEST:  return WEST_WALL;
        default: return 0;
    }
}

// -------------------------------------------------------
// Structs
// -------------------------------------------------------
struct Coord{
  int x, y;
  bool operator==(const Coord& other) const {
    return x == other.x && y == other.y;
  }
};

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

// -------------------------------------------------------
// Phase Enum
// -------------------------------------------------------
enum RunPhase {
  PHASE_INIT,
  PHASE_EXPLORE_TO_GOAL,
  PHASE_RETURN_TO_START,
  PHASE_EXPLORE_FULL,
  PHASE_SPEEDRUN,
  PHASE_DONE
};

inline const char* getPhaseName(RunPhase p) {
  switch (p) {
    case PHASE_INIT:           return "INIT";
    case PHASE_EXPLORE_TO_GOAL:return "EXPLORE_TO_GOAL";
    case PHASE_RETURN_TO_START:return "RETURN_TO_START";
    case PHASE_EXPLORE_FULL:   return "EXPLORE_FULL";
    case PHASE_SPEEDRUN:       return "SPEEDRUN";
    case PHASE_DONE:           return "DONE";
    default:                   return "UNKNOWN";
  }
}

