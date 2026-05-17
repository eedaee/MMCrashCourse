#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 32

// API Function Implementations
int getInteger(char* command) {
    printf("%s\n", command);     // Send command to simulator
    fflush(stdout);              // Ensure command is sent immediately
    char response[BUFFER_SIZE];
    if (fgets(response, BUFFER_SIZE, stdin) != NULL) {  // Read response from simulator
        return atoi(response);                          // Convert response to integer and return
    }
    return -1;                                          // Return -1 if there was an error reading the response
}

int getBoolean(char* command) {
    printf("%s\n", command);     // Send command to simulator
    fflush(stdout);              // Ensure command is sent immediately  
    char response[BUFFER_SIZE];
    if (fgets(response, BUFFER_SIZE, stdin) != NULL) {         // Read response from simulator
        int value = (strcmp(response, "true\n") == 0) ? 1 : 0; // Convert "true"/"false" to 1/0
        return value;                                          // Return bool as an integer
    }
    return -1;                                                 // Return -1 if there was an error reading the response
}

int getAck(char* command) {
    printf("%s\n", command);     // Send command to simulator
    fflush(stdout);              // Ensure command is sent immediately
    char response[BUFFER_SIZE];
    if (fgets(response, BUFFER_SIZE, stdin) != NULL) {  // Read response from simulator
        int success = (strcmp(response, "ack\n") == 0); // Check if response is "ack" 
        return success;                                 // Return 1 if successful, 0 otherwise
    }
    return -1;                                          // Return -1 if there was an error reading the response
}

//-------------------------------- Additional Helper Functions for Simulator Communication --------------------------------
// Maze Dimension Functions
int mazeWidth(){
    return getInteger("mazeWidth");
}

int mazeHeight(){
    return getInteger("mazeHeight");
}


// Wall Sensing Functions
bool wallFront(){
    return getBoolean("wallFront");
}

bool wallRight(){
    return getBoolean("wallRight");
}

bool wallLeft(){
    return getBoolean("wallLeft");
}

bool wallBack(){
    return getBoolean("wallBack");
}


// Action Functions
int moveForward(){
    return getAck("moveForward");
}

void turnRight(){
    getAck("turnRight");
}

void turnLeft(){
    getAck("turnLeft");
}

// Maze Configuration Functions
void setWall(int x, int y, char direction){
    printf("setWall %d %d %c\n", x, y, direction);
    fflush(stdout);
}

void clearWall(int x, int y, char direction){
    printf("clearWall %d %d %c\n", x, y, direction);
    fflush(stdout);
}

// Cell Display Functions
void setColor(int x, int y, char color){
    printf("setColor %d %d %c\n", x, y, color);
    fflush(stdout);
}

void clearColor(int x, int y){
    printf("clearColor %d %d\n", x, y);
    fflush(stdout);
}

void clearAllColor(){
    printf("clearAllColor\n");
    fflush(stdout);
}

// Text Display Functions
void setText(int x, int y, char* str){
    printf("setText %d %d %s\n", x, y, str);
    fflush(stdout);
}

void clearText(int x, int y){
    printf("clearText %d %d\n", x, y);
    fflush(stdout);
}

void clearAllText(){
    printf("clearAllText\n");
    fflush(stdout);
}

// Reset Functions
int wasReset(){
    return getBoolean("wasReset");
}

void ackReset(){
    getAck("ackReset");
}

// Debug Function
void debug_log(char* text){
    fprintf(stderr, "debug_log %s\n", text);
    fflush(stderr);
}

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