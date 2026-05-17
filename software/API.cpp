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