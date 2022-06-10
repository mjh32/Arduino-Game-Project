#include <EEPROM.h>
#include <AberLED.h>
#define S_INVALID -1
#define S_START 0
#define S_PLAYING 1
#define S_DEAD 2
#define S_PAUSE 3
int state = -1; //All above is setting up states and importing libraries
int pPos[] = {0,3}; //Player Position
const int gameMap[][3] = {{0,0,2},{1,0,2},{2,0,2},{5,0,2},{6,0,2},{0,1,2},{2,1,2},{5,1,2},{7,1,2},{0,2,2},{1,2,2},{5,2,2},{7,2,2},{0,3,2},{2,3,2},{5,3,2},{6,3,2},{4,4,3},{3,5,3},{5,5,3},{4,6,3}}; //The start screen
unsigned long lastMoveTime = 0; //Used for updateLCD()
int blocks[8][8]; //The blocks array of the game
bool flash = false; //Used to flash a LED on start, pause and end states
unsigned char score; //the current score
unsigned char highScore; //the EEPROM score
int count = 0; //counts how many times updateLCD() is called in a single play state
float interval; //the speed that the game updates
int maxRan = 10; //the chances of a red dot spawning on a single LED

void setup() { //meeting any perquisites needed for the rest of the code to execute
  AberLED.begin();
  state = 0;
  randomSeed(analogRead(5));
  lastMoveTime = millis();
  clearAllBlocks();
}

/*void gotoState(int s){
  state = s;
  return millis();
}*/

bool playerHit(){ //detects if player has hit a red dot
  return blocks[0][pPos[1]] == 1;
}

bool playerPower(){ //detects if player has hit a green dot
  return blocks[0][pPos[1]] == 2;
}

void clearAllBlocks(){ //sets the entire blocks array to be empty
  for(int x=0;x<8;x++){
    for(int y=0;y<8;y++){
      blocks[x][y]=0;
    }
  }
}

/*void clearBlocks(){
  for(int y=0;y<8;y++){
    blocks[7][y]=0;
  }
  for(int y=0;y<8;y++){
    blocks[-1][y]=0;
  }
}*/

String bitTranslation(unsigned char num){ //translates an unsigned char to bits
  int theNums[] = {128,64,32,16,8,4,2,1};
  String result;
  for(int i=0;i<8;i++){
    if(num>=theNums[i]){
      num = num - theNums[i];
      result = result + '1';
    }
    else{
      result = result + '0';
    }
  }
  return result;
}

void writing(int x, int y, bool special, int luck){ //adds red and green dots to the blocks array
  if(special){
    if(luck==4) blocks[x][y]=2;
    else blocks[x][y]=1;
  }
  else blocks[x][y]=0;
}

void generation(){ //randomly decides if dots are going to spawn
  static int luck = 0;
  for(int y=0;y<8;y++){
    if(random(0,maxRan)==0){
      luck++; //luck counts the number of reds in order to give the right amount of greens
      writing(7,y,true,luck);
      if(luck==4)luck=0;
    }
    else writing(7,y,false,luck);
  }
}

void scrolling(){ //increments all the dots forward by one
  for(int x=0;x<8;x++){
    for(int y=0;y<8;y++){
      if(x!=0){
        blocks[x-1][y] = blocks[x][y];
      }
    }
  }
}

void renderBlocks(){ //uses the AberLED.set() function to write the contents of the blocks variable to screen
  for(int x=0;x<8;x++){
    for(int y=0;y<8;y++){
      switch(blocks[x][y]){
        case 0:
          break;
        case 1:
          AberLED.set(x,y,2);
          break;
        case 2:
          AberLED.set(x,y,1);
          break;
      }
    }
  }
}

void getInput(){ //registers valid inputs within the current state and takes appropriate action
  switch(state){
    case S_START:
      if(AberLED.getButtonDown(5)) state = 1;
      break;
    case S_PLAYING:
      if(AberLED.getButtonDown(1)&&pPos[1]>0) pPos[1]=pPos[1]-1;
      if(AberLED.getButtonDown(2)&&pPos[1]<7) pPos[1]=pPos[1]+1;
      if(AberLED.getButtonDown(5)) state = 3;
      break;
    case S_DEAD:
      if(AberLED.getButtonDown(5)){
        clearAllBlocks();
        state=0;
      }
      break;
    case S_PAUSE:
      if(AberLED.getButtonDown(5)) state = 1;
      break;
  }
}

void updateLCD(){ //calls functions and sets variables each interval appropriate for the current state
  unsigned long elapsedTime = millis() - lastMoveTime;
  if(elapsedTime > interval){
    lastMoveTime = millis();
    switch(state){
      case S_START: //sets all the variables ready for the playing state
        interval = 250;
        score = 0;
        highScore = 0;
        flash = !flash;
        maxRan = 10;
        count = 0;
        break;
      case S_PLAYING: //generates the blocks and scrolls, sets the interval speed and increments the score
        scrolling();
        generation();
        if(interval<1){
          if(EEPROM.read(0)<score) EEPROM.write(0,score);
          highScore = EEPROM.read(0);
          state = 2;
          break;
        }
        if(playerHit()) interval = interval - 50;
        if(playerPower()&&interval<250) interval = interval + 25;
        interval = interval - (interval/256);
        count++;
        if(count==20||count==40||count==60){
          if(maxRan>2&&count==60)maxRan--;
          if(count==60)count=0;
          score++;
        }
        break;
      case S_DEAD:
        interval = 250;
        pPos[1] = 3;
        flash = !flash;
        break;
      case S_PAUSE:
        flash = !flash;
        break;
    }
  }
}

void render(){ //renders the displays for each state with functions helping out
  AberLED.clear();
  switch(state){
    case S_START: //uses the constant array to render the start screen
      for(int i=0;i<21;i++){
        AberLED.set(gameMap[i][0],gameMap[i][1],gameMap[i][2]);
      }
      if(flash) AberLED.set(2,6,1);
      break;
    case S_PLAYING: //displays the dots and player
      renderBlocks();
      AberLED.set(pPos[0],pPos[1],3);
      break;
    case S_PAUSE: //displays the dots and player
      renderBlocks();
      if(flash)AberLED.set(pPos[0],pPos[1],3);
      break;
    case S_DEAD: //displays the score and high score in binary
      String points;
      points = bitTranslation(score);
      for(int i=0;i<8;i++){
        if(points[i]=='1') AberLED.set(i,0,2);
      }
      AberLED.set(4,2,3);
      AberLED.set(3,3,3);
      AberLED.set(5,3,3);
      AberLED.set(4,4,3);
      if(flash) AberLED.set(2,4,1);
      String topPoints;
      topPoints = bitTranslation(highScore);
      if(score==highScore){
        if(!flash){
          for(int i;i<8;i++){
            if(topPoints[i]=='1') AberLED.set(i,7,2);
          }
        }
      }
      else{
        for(int i=0;i<8;i++){
          if(topPoints[i]=='1') AberLED.set(i,7,2);
        }
      }
      break;
  }
  AberLED.swap();
}

void loop() { //calls the three main functions
  getInput();
  updateLCD();
  render();
}
