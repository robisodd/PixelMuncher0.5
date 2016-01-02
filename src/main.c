/*
v0.3: Ditched fluff (menus, animated backgrounds, etc.) to actually release this
      Spectres move randomly
      





TODO:
      Acceleromter should only be enabled if using it
      Save battery by only drawing what is being updated:
        Erase old sprites by draw background on top of where sprites were
        Draw sprites at new position
        Maybe B&W only -- Color draws entire screen no matter what

*/
#include "main.h"
  
#define UPDATE_MS 30 // Refresh rate in milliseconds (about 32fps)
//#define UPDATE_MS 100 // Refresh rate in milliseconds (about 32fps)
  
#define ZOOM 5       // Number of pixels per map square
// Map SHOULD be:
// Currently: 28w x 31h
// Original:  28w x 36h
// Should be: 32w x 36h
#define MAP_W 28
#define MAP_H 31
//#define BOARD_H 29 // technically 31 with top/bottom border, but can leave those out
  
// Pixel Offset to display board on screen
#define BOARD_X 2
#define BOARD_Y 13

// =========================================================================================================== //

#define Cherries   0
#define Strawberry 1
#define Peach      2
#define Apple      3
#define Grapes     4
#define Spaceship  5
#define Bell       6
#define Key        7
//uint32_t bonuspoints[8] = {100, 300, 500, 700, 1000, 2000, 3000, 5000};
//0:Cherries, 1:Strawberry, 2:Peach, 3:Apple, 4:Grapes, 5:Spaceship, 6:Bell, 7:Key
// ============================
//            Bonus
// ============================
//  Index   Points     Name
//    0       100    Cherries  
//    1       300    Strawberry
//    2       500    Peach
//    3       700    Apple
//    4      1000    Grapes
//    5      2000    Spaceship
//    6      3000    Bell
//    7      5000    Key
// ============================

  
// =========================================================================================================== //
int32_t abs32(int32_t x);
// =========================================================================================================== //
typedef struct XYStruct {
  int32_t x;
  int32_t y;
} XYStruct;
// =========================================================================================================== //
typedef struct SpectreStruct {
  XYStruct pos;        // in pixels, not tile -- center pixel
  XYStruct dir;        // direction of movement (always -1, 0 or 1)
  XYStruct targettile; // tile, not pixel
  uint8_t  speed;      // speed multlipier for direction of movement
  //uint8_t face;      // 0=Left, 1=Up, 2=Right, 3=Down
  int16_t  facing;     // Eyes Direction Facing (from 0 - TRIG_MAX_ANGLE)
  //uint32_t frame;    // Animation frame. 0=Skirt, 1=Skirt
  uint8_t  color;      // Spectre color, hard coded and overridden if scared
  uint8_t  mode;       //
  uint8_t  animate;
  uint8_t   enqueued_mode;
  //Modes:
  // Bunker
  // Leaving Bunker
  // Entering Bunker
  // Patrol
  // Attack
  // Cruise Elroy
  // Scared (Blue)
  // Dead/Eyes
} SpectreStruct;

#define ModeEyes   0
#define ModeBunker 1
#define ModePatrol 2
#define ModeAttack 3
#define ModeCruise 4
#define ModeScared 5
#define ModeInvisible 6
//SpectreStruct spectre[4];

// PlayerStruct are things about the person playing
//   which are the things that stay with "Player 1" and "Player 2" etc
//   including: initials, current dots remaining, score, lives, current level
typedef struct PlayerStruct {
  uint32_t score;
   uint8_t lives;
   uint8_t level;
   uint8_t dots[31];   // dots left
   uint8_t totaldots;  // current number of dots on the board
  // name/initials?
} PlayerStruct;

// MuncherStruct is about the character.
//   Properties reset between each player and/or single player's death
//   including: x,y pos, speed, mouth frame
typedef struct MuncherStruct {
  XYStruct pos;    // in pixels, not tile.  ~center pixel of sprite
  XYStruct dir;    // direction of movement (always -1, 0 or 1)
   uint8_t speed;  // speed multlipier for direction of movement
  //uint8_t face;  // 0=Left, 1=Up, 2=Right, 3=Down
   int16_t facing; // Eater Direction Facing (from 0 - TRIG_MAX_ANGLE)
  uint32_t frame;  // Animation frame. Mouth: 0=Closed, 1=Open, 2=Wide, 3=Open
//   uint8_t  mode;   // Alive, Dead
} MuncherStruct;

#define ModeDead   0
#define ModeAlive  1

// NOTE: Probably should change LevelStruct to a function which figures the data instead of a lookup table
// ... actually, make function which fills LevelStruct currentlevel
typedef struct LevelStruct {
  uint8_t bonus_sprite;
  //uint16_t bonus_points;  // points seem to be linked to sprite
  
  uint8_t bluemode_time;
  uint8_t bluemode_flashes;
  uint8_t muncher_normal_speed;
  uint8_t muncher_bluemode_speed;
  uint8_t spectre_normal_speed;
  uint8_t spectre_bluemode_speed;
  uint8_t spectre_tunnel_speed;
  uint8_t spectre_cruise1_speed;
  uint8_t spectre_cruise1_dots;
  uint8_t spectre_cruise2_speed;
  uint8_t spectre_cruise2_dots;
  
  uint8_t attackpatrol[8];
} LevelStruct;
// AttackPatrol Schedule: (all in Seconds)
// =========================================================
//  Mode       Level 1     Levels 2–4    Levels 5+
// =========================================================
// Scatter         7             7            5
//  Chase         20            20           20
// Scatter         7             7            5
//  Chase         20            20           20
// Scatter         5             5            5
//  Chase         20          1033         1037
// Scatter         5          1/60         1/60
//  Chase     indefinite   indefinite   indefinite
// =========================================================




#define AccelerometerControl   0 // Accelerometer Movement (default)
#define ULDRButtonControl      1 // Up/Down/Back/Select for Up/Down/Left/Right
#define LRButtonControl        2 // Up/Down for CounterClockwise/Clockwise rotation, Select to reverse direction

#define AccelerometerTolerance 10 // How far to tilt watch before it's recognized as joystick input
  
#define spectre0_color 0b11110000
#define spectre1_color 0b11001111
#define spectre2_color 0b11110111
#define spectre3_color 0b11111000;
#define scared_body_color 0b11010111
#define scared_face_color 0b11111111
#define spectre_eye_color 0b11111111
#define spectre_pupil_color 0b11000011

#define muncher_color 0b11111100

#define player_text_color 0b11011111
#define ready_color 0b11111100
#define game_over_color 0b11110000
#define score_color 0b11111111

  #define board_color 0b11000011
  #define background_color 0b11000000
  #define door_color 0b11111010
  
// ------------------------------------------------------------------------ //
//  Make all functions globally accessable
// ------------------------------------------------------------------------ //
void update_movement_via_joystick();
void game_click_config_provider(void *context);

void   init_board();
uint8_t getmap(int32_t x, int32_t y);
void   setmap(int32_t x, int32_t y, int8_t data);
uint8_t getlevelspeed(uint8_t level);

void load_graphics();
void unload_graphics();

void draw_background_fb(uint8_t *fb);
void draw_dots_fb(uint8_t *fb);
void draw_muncher_fb(uint8_t *fb);
void draw_spectres(uint8_t *fb);

//void draw_sprite8(uint8_t *fb, uint8_t *font, int16_t start_x, int16_t start_y, uint8_t color, uint8_t spr);
void draw_font8(uint8_t *fb, int32_t x, int32_t y, uint8_t color, uint8_t chr);
void draw_font8_text(uint8_t *fb, int16_t x, int16_t y, uint8_t color, char *str); // str points to zero-terminated string
void draw_actor(uint8_t *fb, int32_t x, int32_t y, uint8_t color, uint8_t spr);
void draw_pupils(uint8_t *fb, int32_t x, int32_t y, uint8_t color, uint8_t facing);
void draw_sprite(uint8_t *fb, int32_t x, int32_t y, uint8_t color, uint8_t spr);

void build_shadow_table();
void fill_rect(uint8_t *screen, GRect rect, uint8_t color);

void set_pattern(uint8_t *data);
void fill_framebuffer_with_pattern(uint8_t *screen, uint8_t *data);
void modify_pattern(uint8_t *data, int8_t x_offset, int8_t y_offset, uint8_t invert);
void create_pattern_layer();
void destroy_pattern_layer();


void init_player();
void init_muncher();
void move_muncher();
void init_spectres();
void move_spectres();
void check_collisions();
void muncher_eat_dots();

void start_game();
void init_game();
void destroy_game();
// ------------------------------------------------------------------------ //

Window *main_window;
Layer *root_layer;










uint16_t totalpellets;
  
// ------------------------------------------------------------------------ //
//  Helper Functions
// ------------------------------------------------------------------------ //
int32_t abs32(int32_t x) {return (x ^ (x >> 31)) - (x >> 31);}

Layer *game_layer;
AppTimer* looptimer;
uint8_t animate = 0;
uint32_t hi_score = 0;
#define ModeNewLifeBegin 0  // Just like ModeLevelBegin, but subtracts a life after showing "PLAYER ONE" "READY!"
#define ModeLevelBegin   1  // Very beginning, show "player 1" "ready", no sprites, no animation, pause for 16 counts
#define ModeRoundBegin   2  // Show board, ready, spectres, muncher (and reduce 1 life), pause for 16 counts, change mode to Playing
#define ModePlaying      3  // Most common mode
#define ModeDying        4  // Pause, go through death sequence, check GameOver, else run init (which will change mode to ModeStarting)
#define ModeWinning      5  // Pause, erase sprites, flash board, then run init (which will change mode to ModeStarting)
#define ModeGameOver     6  // Dead, no lives left.  Only back button to exit
  
uint8_t game_mode;// = ModeLevelBegin;  // should be set by init
uint8_t mode_counter;// = 0;         // Used for timing of starting, death and winning
bool animating;// = true;  // whether animations are happening or not (will get rid of this variable upon refinements later)
// uint8_t death_frame = 0;  // obsolete, replaced by mode_counter

PlayerStruct player;
SpectreStruct spectre[4];
MuncherStruct muncher;
//LevelStruct level[21];
LevelStruct currentlevel;
uint8_t levelplayerspeed;         // probably replace with level[min(player.level,21)].playerspeed

uint8_t map[MAP_W * MAP_H];  // int8 means cells can be from -127 to 128

//TODO: does face_to_dir need to be 32bits?
const int32_t face_to_dir[2][4] = {{1, 0, -1, 0},{0, -1, 0, 1}}; // X then Y

uint32_t bonuspoints[8] = {100, 300, 500, 700, 1000, 2000, 3000, 5000}; // 0:Cherries, 1:Strawberry, 2:Peach, 3:Apple, 4:Grapes, 5:Spaceship, 6:Bell, 7:Key

void set_mode(uint8_t mode) {
  game_mode = mode;
  mode_counter = 0;
}

void init_player() {
  player.score = 0;
  player.level = 1;
  player.lives = 3;
  for(uint8_t i=0; i<31; i++)
    player.dots[i] = 0; // all dots = uneaten
  //name="Player " & Number
}

void add_points(uint8_t points_to_add) {
  player.score += points_to_add;
  //if(player[current_player].score > hi_score) { }
  // Calculate if monsters come out of their pen
}

void init_muncher() {
  muncher.pos.x   = (14<<6);    // start halfway between 13&14;
  muncher.pos.y   = (23<<6)+32; // start in middle of 23;
  muncher.facing  = 2;          // Facing Left. 
                      // if using angles, then TRIG_MAX_ANGLE/4, or 1<<13; (up so counterclockwise button goes left, clockwise goes right)
  muncher.frame   =  0;        // Full Circle Sprite
  muncher.dir.x   = -1;        // moving left
  muncher.dir.y   =  0;        // not moving vertically
//   muncher.mode    = ModeAlive; // Currently alive, but not for long... Muahahaha!
}

void move_muncher() {
  //TODO: tend toward the middle based on speed
  if(muncher.dir.x != 0) {     // if moving horizontally
    if(getmap(muncher.pos.x+(32*muncher.dir.x), muncher.pos.y)<128) { // if not running into a wall  
      muncher.pos.x += (muncher.speed*muncher.dir.x);
      if(muncher.pos.x<-63)     muncher.pos.x += (32<<6);  // tunnel left wraparound, appear right
      if(muncher.pos.x>(32<<6)) muncher.pos.x -= (32<<6);  // tunnel right wraparound, appear left
//       muncher.frame=(muncher.frame+1)&3;
      muncher.frame=(muncher.frame+4)&15;
      muncher.pos.y = ((muncher.pos.y>>6)<<6)+32; // tend toward tile center (currently insta-moves to center)
    } else { // will hit a wall
      muncher.pos.x = ((muncher.pos.x>>6)<<6)+32; // finish moving toward wall, stop at tile center
      muncher.dir.x = 0;
    }
  } else if(muncher.dir.y !=0) {  // (not moving horizontally) if moving vertically
    if(getmap(muncher.pos.x, muncher.pos.y+(32*muncher.dir.y))<128) { // if not running into a wall  
      muncher.pos.y += (muncher.speed*muncher.dir.y);
//       muncher.frame=(muncher.frame+1)&3;
      muncher.frame=(muncher.frame+4)&15;
      muncher.pos.x = ((muncher.pos.x>>6)<<6)+32; // tend toward tile center
    } else { // hit a wall
      muncher.pos.y = ((muncher.pos.y>>6)<<6)+32; // finish moving toward wall, stop at tile center
      muncher.dir.y = 0;
    }
  } else { // (not moving horizontally, not moving vertically)
    
  }
}

void muncher_eat_dots() {
  //====================================//
  // Eat Dots, Update Score, Slow Speed
  //====================================//
  uint8_t tile = getmap(muncher.pos.x, muncher.pos.y);
  if(tile&8) {                                     // if on a dot
    if(tile&16) {                                    // if dot is a super dot
      muncher.speed -= 6;                              // slow down due to eating superdot
      add_points(50);                                  // add 50 points to score
      player.totaldots--;
      // [TODO] start CHASE mode
      //for spectre = 1 to 4
      // enqueue mode: scared
      //for(uint8_t i=0; i<4; ++i)
      //  spectre[i].mode=ModeScared;
      // need to have a bluemode flag or a bunker blue mode.
    } else {                                         // else it's a regular dot
      muncher.speed -= 2;                              // slow down due to eating dot
      add_points(10);                                  // add 10 points to score
    }
    setmap(muncher.pos.x, muncher.pos.y, tile&(~24));// remove dot from board
    player.totaldots--;
  }
}

  

void init_spectres() {
  // facing and direction are different as eyes point to where it WILL go
  for(uint8_t i=0; i<4; ++i) {
    spectre[i].pos.x  = (14<<6);    // start halfway between 13&14;
    spectre[i].pos.y  = (14<<6)+32; // start in middle of 14;
    spectre[i].dir.x  = 0;
    spectre[i].dir.y  = 0;
    spectre[i].speed  = 1;
    spectre[i].enqueued_mode = spectre[i].mode   = ModeBunker;
  }
  spectre[0].enqueued_mode = spectre[0].mode   = ModePatrol;
  
  spectre[0].pos.y  = (11<<6)+32; // start in middle of 11;
  spectre[1].pos.x  = (12<<6);    // start halfway between 11&12;
  spectre[3].pos.x  = (16<<6);    // start halfway between 15&16;

  spectre[0].facing = 2;          // Facing Left
  spectre[1].facing = 1;          // Facing Up
  spectre[2].facing = 3;          // Facing Down
  spectre[3].facing = 1;          // Facing Up
  
  spectre[0].dir.x  = -1; // left
  spectre[1].dir.y  = -1; // Up
  spectre[2].dir.y  =  1; // Down
  spectre[3].dir.y  = -1; // Up
  
  spectre[0].color = spectre0_color;
  spectre[1].color = spectre1_color;
  spectre[2].color = spectre2_color;
  spectre[3].color = spectre3_color;
  
  
  //spectre[].targettile.x = 0;
  spectre[0].targettile.x   = (23<<6);
  spectre[0].targettile.y   = (1<<6);
  spectre[1].targettile.x   = (23<<6);
  spectre[1].targettile.y   = (1<<6);
  spectre[2].targettile.x   = (23<<6);
  spectre[2].targettile.y   = (1<<6);
  spectre[3].targettile.x   = (23<<6);
  spectre[3].targettile.y   = (1<<6);

    
  //Enemy Init:
	//Start Positions
	//Target Coordinates for every mode (attack, regroup, scare)

  //Modes:
  // Bunker
  // Patrol
  // Attack
  // Cruise Elroy
  // Scared (Blue)
  // Dead/Eyes
  
}

void move_spectres() {
  for(uint8_t i=0; i<4; ++i) {
    spectre[i].speed=15;
    



    
    // check mode
    // check dot count
    // set speed accordingly for position
    // set CRUISE accordingly

    //check collision
//0-31 = before, 32-63=after
// if previous center dot was before center square and will be after
// then check next whole square (after moving)

    
    /*
    
    Center Position = BottomLeft
    OOO|OOOO
    OOO|OOOO
    OOO|OOOO
    OOO|OOOO
    ---+----
    OOO|OOOO
    OOO|OOOO
    OOO|OOOO
    
    
Whenever a ghost enters a new tile, it looks ahead to the next tile that it will reach, and makes a decision about which direction it will turn when it gets there.
ghosts may never choose to reverse their direction of travel.
there is one exception to this rule, whenever ghosts change from Chase or Scatter to any other mode, they are forced to reverse direction as soon as they enter the next tile.
(when the ghosts leave Frightened mode they do not change direction)
This forced direction-reversal instruction is also applied to any ghosts still inside the ghost house,
so a ghost that hasn’t yet entered the maze by the time the first mode switch occurs will exit the ghost house with a “reverse direction as soon as you can” instruction already pending.
This causes them to move left as usual for a very short time, but they will almost immediately reverse direction and go to the right instead.
Ghosts typically move to the left once they get outside, but if the system changes modes one or more times when a ghost is inside, that ghost will move to the right instead of the left upon leaving the house

twice as fast (same speed as the disembodied eyes 

Turbo Mode
Each game has an alternate mode called Turbo (a.k.a. speedy mode). This is a popular hardware modification of the game found in many of the original arcade cabinets. In this mode, Pac-Man travels about twice as fast (same speed as the disembodied eyes of the ghosts) and is not slowed down when eating pellets.
source: http://pacman.shaunew.com/

  //Modes:
  // Bunker
  // Leaving Bunker
  // Entering Bunker
  // Patrol
  // Attack
  // Cruise Elroy
  // Scared (Blue)
  // Dead/Eyes
  
fill speedbucket with speed based on mode and position
while(speedbucket>0) {
  OldPosition=Position
  Position += XYSpeed                     // (1 pixel movement)
  speedbucket--;
  If(tile(OldPosition)<>tile(position)) { // if entered new tile
    Position += XYSpeed*speedbucket       // Finish Off speedbucket
    If(NewMode<>Mode) {                   // If Mode Changed
      If(Mode==Chase or Scatter){  //chase-to-scatter, chase-to-frightened, scatter-to-chase, and scatter-to-frightened
        Facing=Reversed
      }
      Mode=NewMode
      

    } else {
    
    }
    Based on mode, do the following:
    Facing = CalculateNewDecisionXY(TargetTile)
    Update Facing
  }
  
  if(position=middle) {
    XYspeed=Facling
  }
}



Move(Direction) {
    //If(Facing Left or Right and Y=Center) DirY=0
    //If(Facing Up   or Down  and X=Center) DirX=0
    //add DirX
    //add DirY

    If(Facing Left or Right) {
      If(Y<>Center) {
        PosY += DirY  // If goes beyond center, take remainder and add it to X*DirX
      } else {
        DirY=0  // maybe not necessary
        PosX += DirX
      }
    } else {
      If(X<>Center) {
        PosX += DirX  // If goes beyond center, take remainder and add it to Y*DirY
      } else {
        DirX=0  // maybe not necessary
        PosY += DirY
      }
    }
}

PositionXY = Move(DirectionXY)
If(InANewTile){
  If(ReverseQueue) {
    DirectionXY = Backwards(DirectionXY)
  } else {
    DirectionXY = DecisionXY
  }
  Mode = CalculateMode(board.spectremode)
  DecisionXY = CalculateNewDecisionXY(TargetTile)
  Pupils = DecisionXY
}

TargetTile XY: Not px but tile (/64) target
Position XY: 64px/tile position
DirectionXY: Direction currently traveling
             If X<>center, add DirX
             If Y<>center, add DirY
DecisionXY: Upon entering new tile, DirectionXY=DecisionXY, then calculate new DecisionXY based on TargetTile
            Pupils = DecisionXY
Reverse Queue:  Upon entering new tile, if ReverseQueue=1, Throw out DecisionXY, DirectionXY=-DirectionXY, calculate new DecisionXY based on TargetTile
                If system changes modes while inside bunker, move right instead of left after leaving
Mode Change Queue: (Changes modes only upon entering new tile)

    
    */
    
     //if ((spectre[i].pos.x>>5)&1)
    if(spectre[i].mode==ModeBunker) {
      if(getmap(spectre[i].pos.x+(64*spectre[i].dir.x), spectre[i].pos.y+(64*spectre[i].dir.y))<128) { // if not running into a wall  
        spectre[i].pos.x += (spectre[i].speed*spectre[i].dir.x);
        spectre[i].pos.y += (spectre[i].speed*spectre[i].dir.y);
      } else { // will hit a wall
          //spectre[i].pos.y -= ((spectre[i].dir.y*64*1)/5);
          //spectre[i].pos.y -= (spectre[i].dir.y*32);
          spectre[i].facing = (spectre[i].facing  + 2) & 3; // reverse face
          spectre[i].dir.y *= -1;  // reverse direction
          //spectre[i].dir.y = 0;
        } 
    } else {
      if(getmap(spectre[i].pos.x+(33*spectre[i].dir.x), spectre[i].pos.y+(33*spectre[i].dir.y))<128) { // if not running into a wall  
        spectre[i].pos.x += (spectre[i].speed*spectre[i].dir.x);
        spectre[i].pos.y += (spectre[i].speed*spectre[i].dir.y);
        if(spectre[i].pos.x<-63)     spectre[i].pos.x += (32<<6); // tunnel left wraparound
        if(spectre[i].pos.x>(32<<6)) spectre[i].pos.x -= (32<<6); // tunnel right wraparound
        //spectre[i].frame=(spectre[i].frame+1)&3;
        
        if(spectre[i].mode != ModeBunker) {
          if(spectre[i].dir.x == 0)    // (not moving horizontally) if moving vertically
            spectre[i].pos.x = (spectre[i].pos.x&(~63))+32; // tend toward tile center
          if(spectre[i].dir.y == 0)     // if moving horizontally
            spectre[i].pos.y = (spectre[i].pos.y&(~63))+32; // tend toward tile center
        }
      } else { // will hit a wall
//         if(spectre[i].mode==ModeBunker) {
//           //spectre[i].pos.x -= (spectre[i].dir.x*32);
//           //spectre[i].facing = (spectre[i].facing  + 2) & 3;
//           //spectre[i].dir.x = 0;//*= -1;
          
//           //spectre[i].pos.y -= ((spectre[i].dir.y*64*1)/5);
//           //spectre[i].pos.y -= (spectre[i].dir.y*32);
//           spectre[i].facing = (spectre[i].facing  + 2) & 3; // reverse face
//           spectre[i].dir.y *= -1;  // reverse direction
//           //spectre[i].dir.y = 0;        
//         }
        if(spectre[i].mode==ModePatrol) {
          spectre[i].facing = (spectre[i].facing + (rand()%3))&3;
          spectre[i].dir.x = face_to_dir[0][spectre[i].facing];
          spectre[i].dir.y = face_to_dir[1][spectre[i].facing];
          //spectre[i].dir.x = 0;//*= -1;
        }
      }
    }

    
//     if(spectre[i].dir.x != 0) {     // if moving horizontally
//       if(getmap(spectre[i].pos.x+(33*spectre[i].dir.x), spectre[i].pos.y)>=0) { // if not running into a wall  
//         spectre[i].pos.x += (spectre[i].speed*spectre[i].dir.x);
//         if(spectre[i].pos.x<-63)     spectre[i].pos.x += (32<<6); // tunnel left wraparound
//         if(spectre[i].pos.x>(32<<6)) spectre[i].pos.x -= (32<<6); // tunnel right wraparound
//         //spectre[i].frame=(spectre[i].frame+1)&3;
//         //spectre[i].pos.y = ((spectre[i].pos.y>>6)<<6)+32; // tend toward tile center
//         spectre[i].pos.y = (spectre[i].pos.y&(~63))+32; // tend toward tile center
//       } else { // will hit a wall
//         spectre[i].pos.x = (spectre[i].pos.x&(~63))+32; // finish moving toward wall, stop at tile center
//         if(spectre[i].mode==ModeBunker) {
//           //spectre[i].pos.x -= (spectre[i].dir.x*32);
//           //spectre[i].facing = (spectre[i].facing  + 2) & 3;
//           //spectre[i].dir.x = 0;//*= -1;
//         }
//         if(spectre[i].mode==ModePatrol) {
//           spectre[i].facing = (spectre[i].facing + (rand()%3))&3;
//           spectre[i].dir.x = face_to_dir[0][spectre[i].facing];
//           spectre[i].dir.y = face_to_dir[1][spectre[i].facing];
//           //spectre[i].dir.x = 0;//*= -1;
//         }
//       }
//     } else if(spectre[i].dir.y !=0) {  // (not moving horizontally) if moving vertically
//       if(getmap(spectre[i].pos.x, spectre[i].pos.y+(33*spectre[i].dir.y))>=0) { // if not running into a wall  
//         spectre[i].pos.y += (spectre[i].speed*spectre[i].dir.y);
//         //spectre[i].frame=(spectre[i].frame+1)&3;
//         if(spectre[i].mode != ModeBunker)
//           spectre[i].pos.x = ((spectre[i].pos.x>>6)<<6)+32; // tend toward tile center
//       } else { // hit a wall
//         spectre[i].pos.y = ((spectre[i].pos.y>>6)<<6)+32; // finish moving toward wall, stop at tile center
//         if(spectre[i].mode==ModeBunker) {
//           //spectre[i].pos.y -= ((spectre[i].dir.y*64*1)/5);
//           //spectre[i].pos.y -= (spectre[i].dir.y*32);
//           spectre[i].facing = (spectre[i].facing  + 2) & 3; // reverse face
//           spectre[i].dir.y *= -1;  // reverse direction
//           //spectre[i].dir.y = 0;
//         }
        
//         if(spectre[i].mode==ModePatrol) {
//           spectre[i].facing = (spectre[i].facing + (rand()%3))&3;
//           spectre[i].dir.x = face_to_dir[0][spectre[i].facing];
//           spectre[i].dir.y = face_to_dir[1][spectre[i].facing];
//           //spectre[i].dir.x = 0;//*= -1;
//         }

        
//       }
//     } else { } // (not moving horizontally, not moving vertically)
    
  }
}

// =========================================================================================================== //
//  Map and Level Variables
// ======================= //
//TODO:
// Init Map and Init Dots
// initial_dots will be just 36(?) 32bit ints, each bit will be 1 or 0 and the two 2s (superdots) will be done in function
// initial_map will also be just 1bit, converted to -1 or 0 in MAP[]
// player[].dots[] will be like initial_dots and use same function to copy to MAP[]

// static int8_t boardlayout[] =
// {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
//  -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,
//  -1, 1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,
//  -1, 2,-1, 0, 0,-1, 1,-1, 0, 0, 0,-1, 1,-1,
//  -1, 1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,
//  -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//  -1, 1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,
//  -1, 1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,
//  -1, 1, 1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1,-1,
//  -1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 0,-1,
//   0, 0, 0, 0, 0,-1, 1,-1,-1,-1,-1,-1, 0,-1,
//   0, 0, 0, 0, 0,-1, 1,-1,-1, 0, 0, 0, 0, 0,
//   0, 0, 0, 0, 0,-1, 1,-1,-1, 0,-1,-1,-1,-1,
//  -1,-1,-1,-1,-1,-1, 1,-1,-1, 0,-1, 0, 0, 0,
//   0, 0, 0, 0, 0, 0, 1, 0, 0, 0,-1, 0, 0, 0,
//  -1,-1,-1,-1,-1,-1, 1,-1,-1, 0,-1, 0, 0, 0,
//   0, 0, 0, 0, 0,-1, 1,-1,-1, 0,-1,-1,-1,-1,
//   0, 0, 0, 0, 0,-1, 1,-1,-1, 0, 0, 0, 0, 0,
//   0, 0, 0, 0, 0,-1, 1,-1,-1, 0,-1,-1,-1,-1,
//  -1,-1,-1,-1,-1,-1, 1,-1,-1, 0,-1,-1,-1,-1,
//  -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,
//  -1, 1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,
//  -1, 1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,
//  -1, 2, 1, 1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 0,
//  -1,-1,-1, 1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,
//  -1,-1,-1, 1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,
//  -1, 1, 1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1,-1,
//  -1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,
//  -1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,
//  -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
//-1 = Impassable
// 0 = Blank
// 1 = Pellet
// 2 = Power Pellet

//00=impass 01=blank 10=dot 11=bigdot
uint32_t BoardLayout[31] = {
//00000000000000000000000000000000
//00000000000000000000000000000000
//00000000000000000000000000000000
//00000000000000000000000000000000 <- top of board
0b0000000000000000000000000000,
0b0010101010101010101010101000,
0b0010000000000010000000001000,
0b0010000101010010000101001100,
0b0010000000000010000000001000,
0b1010101010101010101010101000,
0b0000000010000010000000001000,
0b0000000010000010000000001000,
0b0010101010000010101010101000,
0b0001000000000010000000000000,
0b0001000000000010000101010101,
0b0101010101000010000101010101,
0b0000000001000010000101010101,
0b0101010001000010000000000000,
0b0101010001010110010101010101,
0b0101010001000010000000000000,
0b0000000001000010000101010101,
0b0101010101000010000101010101,
0b0000000001000010000101010101,
0b0000000001000010000000000000,
0b0010101010101010101010101000,
0b0010000000000010000000001000,
0b0010000000000010000000001000,
0b0110101010101010000010101100,
0b0000000010000010000010000000,
0b0000000010000010000010000000,
0b0010101010000010101010101000,
0b0010000000000000000000001000,
0b0010000000000000000000001000,
0b1010101010101010101010101000,
0b0000000000000000000000000000
//00000000000000000000000000000000 <- bottom of board
//00000000000000000000000000000000
//00000000000000000000000000000000
};



/*  Updated Map Data:
Map is now folded in half
bit 76543210
    xxxxxxxx = uint8_t
bit 0: 
bit 1: 
bit 2: 
bit 3: pellet
bit 4: power pellet
bit 5: spectre can't go up
bit 6: spectre goes slowly
bit 7: impassable (>127 means impassable)

Maybe bits 012 will be for 3D floor/ceiling squaretype

When bit 7 is on, the other bits change meaning:
bits 0-6: squaretype [0-127] (for 3D or 2D)
AND/OR bits show which tiles to build map out of (straight, corner, etc)

  1
  2631
  84268421
0b10000xxx = 128 = Impassible/Wall
0b11000xxx = 192 = Door(Impassible/Slow)
0b00000xxx =   0 = Normal Blank
0b00001xxx =   8 = Normal Dot
0b00011xxx =  24 = Normal BigDot
0b01000xxx =  64 = Slow Blank
0b01001xxx =  72 = Slow Dot
0b00100xxx =  32 = Can't go Up Blank
0b00101xxx =  40 = Can't go Up Dot
*/

static int8_t boardlayout[] = {
  128,128,128,128,128,128,128,128,128,128,128,128,128,128,
  128,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,128,
  128,  8,128,128,128,128,  8,128,128,128,128,128,  8,128,
  128, 24,128,  0,  0,128,  8,128,  0,  0,  0,128,  8,128,
  128,  8,128,128,128,128,  8,128,128,128,128,128,  8,128,
  128,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
  128,  8,128,128,128,128,  8,128,128,  8,128,128,128,128,
  128,  8,128,128,128,128,  8,128,128,  8,128,128,128,128,
  128,  8,  8,  8,  8,  8,  8,128,128,  8,  8,  8,  8,128,
  128,128,128,128,128,128,  8,128,128,128,128,128,  0,128,
    0,  0,  0,  0,  0,128,  8,128,128,128,128,128,  0,128,
    0,  0,  0,  0,  0,128,  8,128,128,  0,  0, 32, 32, 32,
    0,  0,  0,  0,  0,128,  8,128,128,  0,128,128,128,192,
  128,128,128,128,128,128,  8,128,128,  0,128, 64, 64, 64,
   64, 64, 64, 64, 64,  0,  8,  0,  0,  0,128, 64, 64, 64,
  128,128,128,128,128,128,  8,128,128,  0,128, 64, 64, 64,
    0,  0,  0,  0,  0,128,  8,128,128,  0,128,128,128,128,
    0,  0,  0,  0,  0,128,  8,128,128,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,128,  8,128,128,  0,128,128,128,128,
  128,128,128,128,128,128,  8,128,128,  0,128,128,128,128,
  128,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,128,
  128,  8,128,128,128,128,  8,128,128,128,128,128,  8,128,
  128,  8,128,128,128,128,  8,128,128,128,128,128,  8,128,
  128, 24,  8,  8,128,128,  8,  8,  8,  8,  8, 40, 40, 32,
  128,128,128,  8,128,128,  8,128,128,  8,128,128,128,128,
  128,128,128,  8,128,128,  8,128,128,  8,128,128,128,128,
  128,  8,  8,  8,  8,  8,  8,128,128,  8,  8,  8,  8,128,
  128,  8,128,128,128,128,128,128,128,128,128,128,  8,128,
  128,  8,128,128,128,128,128,128,128,128,128,128,  8,128,
  128,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
  128,128,128,128,128,128,128,128,128,128,128,128,128,128
};


void save_dots(uint8_t *dots) {
  // obsolete until fixed to use the new boardlayout[]
//   uint8_t bit = 0; // because 8 bits, can't have more than 256 total dots

//   // convert -1,0,1,2 to 0,1,2,3
//   for(uint16_t i=0; i<MAP_H*MAP_W; i++)
//     map[i]++;

//   for(uint16_t y=0, row=0; y<MAP_H*MAP_W; y+=MAP_W, row++)
//     for(uint16_t x=0; x<(MAP_W/2); x++) {  // map_w has to <= 32, else bitshift below breaks
//       if(((BoardLayout[row] >> (x*2))&3) > 1) { // if dot is supposed to exist
//         if(map[y+x]>1) {
//           //dots[(bit>>3)] &= 1(bit&7))&1
//         }
//         if(map[y + MAP_W - 1 - x]>1) {
          
//         }
//       }
//       map[y+x]               = (BoardLayout[row] >> (x*2))&3;
//       map[y + MAP_W - 1 - x] = (BoardLayout[row] >> (x*2))&3; // right 16 tiles = left side horizontally flipped
//       if(map[y+x] > 1) {  // 2(0b10) = dot, 3(0b11) = bigdot
//         if((dots[(bit>>3)]>>(bit&7))&1) // 1 = eaten
//           map[y+x] = 1;                 // 1 = blank
//         bit++;
//         if((dots[(bit>>3)]>>(bit&7))&1) // 1 = eaten
//           map[y + MAP_W - 1 - x] = 1;   // 1 = blank
//         bit++;
//       }
//     }

}

uint8_t load_dots(uint8_t *dots) {
 // note: in dots[], each bit is 0 if dot still exists or 1 if already eaten
 // init player[].dots[] all to 0 to signify full board (0 = dot there in dots[])
  uint8_t totaldots=0;
  uint8_t bit = 0; // because 8 bits, can't have more than 256 total dots
  for(uint16_t y=0, y2=0; y<MAP_H*MAP_W; y+=MAP_W, y2+=(MAP_W/2))
    for(uint16_t x=0; x<(MAP_W/2); x++) {  // map_w has to <= 32, else bitshift below breaks
      uint16_t pos = y + x;
      map[pos]               = boardlayout[y2 + x];
      map[y + MAP_W - 1 - x] = boardlayout[y2 + x]; // right 16 tiles = left side horizontally flipped
      if((map[pos] & 24)>0) {  // if map pos was a dot or bigdot
        if((dots[(bit>>3)]>>(bit&7))&1)  // if that dot was eaten (when dots[]=1, it means it was eaten)
          map[pos] &= ~24;               // remove dot
        else
          totaldots++;
        bit++;
        if((dots[(bit>>3)]>>(bit&7))&1)  // if that dot was eaten (when dots[]=1, it means it was eaten)
          map[y + MAP_W - 1 - x] &= ~24; // remove dot
        else
          totaldots++;
        bit++;
      }
    }
  return totaldots;
}

void init_board() {
//   for(uint16_t i=0; i<MAP_W*MAP_H; i++) map[i] = boardlayout[i];

  player.totaldots = load_dots((uint8_t*)&player.dots);
  
/*  
  for(uint16_t y=0; y<MAP_H*MAP_W; y+=MAP_W)
   for(uint16_t x=0; x<(MAP_W/2); x++) {
     map[y+x] = boardlayout[(y/2)+x];
     map[y + MAP_W - 1 - x] = boardlayout[(y/2)+x];
   }
*/
  levelplayerspeed = 19;  // Default 100% speed -- level speed will replace this
  
}

uint8_t getlevelspeed(uint8_t level) {
  return levelplayerspeed;
}

uint8_t getmap(int32_t x, int32_t y) {
  x>>=6; y>>=6;
  if(y==14 && (x<0 || x>=MAP_W)) return 0;
  return (x<0 || x>=MAP_W || y<0 || y>=MAP_H) ? -1 : map[(y * MAP_W) + x];
}

void setmap(int32_t x, int32_t y, int8_t data) {
  x>>=6; y>>=6;
  if(x>=0 && x<MAP_W && y>=0 && y<MAP_H)
    map[(y * MAP_W) + x]=data;
}

// =========================================================================================================== //
//  Control Input Options
// ======================= //
bool up_button_depressed = false; // Whether Pebble's   Up   button is being held
bool dn_button_depressed = false; // Whether Pebble's  Down  button is being held
bool sl_button_depressed = false; // Whether Pebble's Select button is being held
bool bk_button_depressed = false; // Whether Pebble's  Back  button is being held

//uint16_t currently_at;
uint16_t   turning_at = 32*32;  // Stops turning  left every frame when  UP  is held
//uint16_t     right_at = 32*32;  // Stops turning right every frame when DOWN is held

//uint8_t control_mode = AccelerometerControl;
//uint8_t control_mode = ULDRButtonControl;
uint8_t control_mode = LRButtonControl;


// ------------------------------------------------------------------------ //
//  Button Pushing
// ------------------------------------------------------------------------ //
void up_push_in_handler(ClickRecognizerRef recognizer, void *context) {up_button_depressed = true;  }  //   UP   button was pushed in
void up_release_handler(ClickRecognizerRef recognizer, void *context) {up_button_depressed = false; }  //   UP   button was released
void dn_push_in_handler(ClickRecognizerRef recognizer, void *context) {dn_button_depressed = true;  }  //  DOWN  button was pushed in
void dn_release_handler(ClickRecognizerRef recognizer, void *context) {dn_button_depressed = false; }  //  DOWN  button was released
void sl_push_in_handler(ClickRecognizerRef recognizer, void *context) {sl_button_depressed = true;  }  // SELECT button was pushed in
void sl_release_handler(ClickRecognizerRef recognizer, void *context) {sl_button_depressed = false; }  // SELECT button was released
void bk_click_handler  (ClickRecognizerRef recognizer, void *context) {bk_button_depressed = true;  }  //  BACK  button was clicked (BACK doesn't support raw)

void game_click_config_provider(void *context) {
  window_raw_click_subscribe(BUTTON_ID_UP, up_push_in_handler, up_release_handler, context);
  window_raw_click_subscribe(BUTTON_ID_DOWN, dn_push_in_handler, dn_release_handler, context);
  window_raw_click_subscribe(BUTTON_ID_SELECT, sl_push_in_handler, sl_release_handler, context);
  window_single_click_subscribe(BUTTON_ID_BACK, bk_click_handler);
}


// ------------------------------------------------------------------------ //
//  Get Joystick, Update Eater
// ------------------------------------------------------------------------ //
/* TODO:
         Use ARCTAN2 to figure X&Y Joystick by XZ and YZ accelerometer
         Maybe subtle movement = centered joystick position
*/
void update_movement_via_joystick() {
  AccelData accel;
  XYStruct testspeed, testfacing;
  
  testspeed.x = 0; testspeed.y = 0;  testfacing.x = 0;  testfacing.y = 0;
  switch(control_mode) {
    case AccelerometerControl:
      if(bk_button_depressed) window_stack_pop_all(true); // back = quit (TODO: Menu)
//       if(muncher.mode != ModeDead) {
        accel_service_peek(&accel); // Read accelerometer
        accel.x>>=3; accel.y>>=3;
             if(accel.x<-AccelerometerTolerance) {if(getmap(muncher.pos.x+(64*-1), muncher.pos.y+(64* 0))<128) {testspeed.x =-1; testfacing.x = 2;}} // Left
        else if(accel.x> AccelerometerTolerance) {if(getmap(muncher.pos.x+(64* 1), muncher.pos.y+(64* 0))<128) {testspeed.x = 1; testfacing.x = 0;}} // Right
             if(accel.y<-AccelerometerTolerance) {if(getmap(muncher.pos.x+(64* 0), muncher.pos.y+(64* 1))<128) {testspeed.y = 1; testfacing.y = 3;}} // Down
        else if(accel.y> AccelerometerTolerance) {if(getmap(muncher.pos.x+(64* 0), muncher.pos.y+(64*-1))<128) {testspeed.y =-1; testfacing.y = 1;}} // Up

        if((abs32(accel.x)>abs32(accel.y) || testspeed.y==0) && testspeed.x!=0) {
          muncher.dir.x = testspeed.x;
          muncher.dir.y = 0;
          muncher.facing  = testfacing.x;
        } else if(testspeed.y!=0) {
          muncher.dir.x = 0;
          muncher.dir.y = testspeed.y;
          muncher.facing  = testfacing.y;
        }
//       }
    break;
      

    
    //TODO: Unfinished ULDR - doesn't test if wall is there
    //TODO: Hold Up and Down together for a few frames = menu
    case ULDRButtonControl:
      //if(bk_button_depressed && !sl_button_depressed) {if(getmap(muncher.pos.x+(64*-1), muncher.pos.y+(64* 0))>=0) {testspeed.x =-1; testfacing.x = 2;}} // Left
      //if(sl_button_depressed && !bk_button_depressed) {if(getmap(muncher.pos.x+(64* 1), muncher.pos.y+(64* 0))>=0) {testspeed.x = 1; testfacing.x = 0;}} // Right
      //if(dn_button_depressed && !up_button_depressed) {if(getmap(muncher.pos.x+(64* 0), muncher.pos.y+(64* 1))>=0) {testspeed.y = 1; testfacing.y = 3;}} // Down
      //if(up_button_depressed && !dn_button_depressed) {if(getmap(muncher.pos.x+(64* 0), muncher.pos.y+(64*-1))>=0) {testspeed.y =-1; testfacing.y = 1;}} // Up
//       if(muncher.mode != ModeDead) {
        if(bk_button_depressed && !sl_button_depressed) {if(getmap(muncher.pos.x+(64*-1), muncher.pos.y+(64* 0))<128) {muncher.dir.y=0;muncher.dir.x=-1; muncher.facing=2;}} // Left
        if(sl_button_depressed && !bk_button_depressed) {if(getmap(muncher.pos.x+(64* 1), muncher.pos.y+(64* 0))<128) {muncher.dir.y=0;muncher.dir.x= 1; muncher.facing=0;}} // Right
        if(dn_button_depressed && !up_button_depressed) {if(getmap(muncher.pos.x+(64* 0), muncher.pos.y+(64* 1))<128) {muncher.dir.x=0;muncher.dir.y= 1; muncher.facing=3;}} // Down
        if(up_button_depressed && !dn_button_depressed) {if(getmap(muncher.pos.x+(64* 0), muncher.pos.y+(64*-1))<128) {muncher.dir.x=0;muncher.dir.y=-1; muncher.facing=1;}} // Up    
//       }
    break;
    
    case LRButtonControl:         // Up/Down = CounterClockwise/Clockwise, Select = Reverse Direction (back brings up menu)
      //if pushing SL
      //  reverse
      //else
      //  if can go ccw and pushing UP then
      //    if can go cw and pushing DN then
      //      don't move
      //    else
      //      go ccw(up)
      //  else
      //    if can go cw(down) and pushing down
      //      go cw(down)
  
      if(bk_button_depressed) window_stack_pop_all(true); // back = quit (TODO: Menu)
//       if(muncher.mode != ModeDead) {
        if(sl_button_depressed) {            // Reverse
           sl_button_depressed = false;       // Stop reversing
           muncher.facing = (muncher.facing+2)&3;
           muncher.dir.x = face_to_dir[0][muncher.facing];
           muncher.dir.y = face_to_dir[1][muncher.facing];
         } else if(((muncher.pos.x>>6) * (muncher.pos.y>>6)) != turning_at) {
           turning_at = 32*32;
           testfacing.x = (muncher.facing+1)&3; // Left Turn
           testfacing.y = (muncher.facing+3)&3; // Right Turn

           if(getmap(muncher.pos.x+(64*face_to_dir[0][testfacing.x]), muncher.pos.y+(64*face_to_dir[1][testfacing.x]))<128 && up_button_depressed) {
             if(getmap(muncher.pos.x+(64*face_to_dir[0][testfacing.y]), muncher.pos.y+(64*face_to_dir[1][testfacing.y]))<128 && dn_button_depressed) {
             } else {
               //go ccw(up)
               muncher.facing = testfacing.x;
               muncher.dir.x = face_to_dir[0][muncher.facing];
               muncher.dir.y = face_to_dir[1][muncher.facing];
               turning_at = ((muncher.pos.x>>6) * (muncher.pos.y>>6));
             }
           } else {
             if(getmap(muncher.pos.x+(64*face_to_dir[0][testfacing.y]), muncher.pos.y+(64*face_to_dir[1][testfacing.y]))<128 && dn_button_depressed) {
               //go cw(down)
               muncher.facing = testfacing.y;
               muncher.dir.x = face_to_dir[0][muncher.facing];
               muncher.dir.y = face_to_dir[1][muncher.facing];
               turning_at = ((muncher.pos.x>>6) * (muncher.pos.y>>6));
             }
           }
        }
//       }
    break;
  }
  bk_button_depressed = false; // since it's click, not raw, resetting bk variable until next depression
}
//   testspeed.x = 0; testspeed.y = 0;
//   if(joystickmode) {
//     if(abs32(accel.x)>abs32(accel.y)) {
//       if(accel.x<0) {testspeed.x =-1; testfacing = 2;} // Left
//       else          {testspeed.x = 1; testfacing = 0;} // Right
//     } else {
//       if(accel.y<0) {testspeed.y = 1; testfacing = 3;} // Down
//       else          {testspeed.y =-1; testfacing = 1;} // Up
//     }
//   } else { // button mode
//     if(sl_button_depressed && player[currentplayer].facing != 0) {testspeed.x = 1; testfacing = 0;} // Right
//     if(up_button_depressed && player[currentplayer].facing != 1) {testspeed.y =-1; testfacing = 1;} // Up
//     if(bk_button_depressed && player[currentplayer].facing != 2) {testspeed.x =-1; testfacing = 2;} // Left
//     if(dn_button_depressed && player[currentplayer].facing != 3) {testspeed.y = 1; testfacing = 3;} // Down
//   }
  
//   if(getmap(pos.x+(64*testspeed.x), pos.y+(64*testspeed.y))>=0) {  // if trying to turn and you can turn
//     player[currentplayer].speed.x=testspeed.x;
//     player[currentplayer].speed.y=testspeed.y;
//     player[currentplayer].facing = testfacing;
//   }
  

void check_collisions() {
  for(uint8_t i=0; i<4; ++i) {
    if(((spectre[i].pos.x>>6)==(muncher.pos.x>>6)) && ((spectre[i].pos.y>>6)==(muncher.pos.y>>6))) {
      set_mode(ModeDying);
//       muncher.mode = ModeDead;
    }
    
  }
  // Check Fruit Collision
  
}


//AccelData accel;// = (AccelData) { .x = 0, .y = 0, .z = 0 };
// ------------------------------------------------------------------------ //
//  Main Loop Functions
// ------------------------------------------------------------------------ //
static void gameloop(void *data) {
  switch(game_mode) {
    case ModeNewLifeBegin:// Just like ModeLevelBegin, but subtracts a life
      animating = false;
      if(bk_button_depressed) window_stack_pop_all(true); // back = quit (TODO: Menu)
      mode_counter++;
      if(mode_counter>16*4) {
        player.lives--;
        set_mode(ModeRoundBegin);
      }
    break;
    
    case ModeLevelBegin:// Very beginning, show "player 1" "ready", no sprites, no animation, pause for 16 counts
      animating = false;
      if(bk_button_depressed) window_stack_pop_all(true); // back = quit (TODO: Menu)
      mode_counter++;
      if(mode_counter>16*4) {
        set_mode(ModeRoundBegin);
      }
    break;
    
    case ModeRoundBegin: // Show board, ready, spectres, muncher (and reduce 1 life), pause for 16 counts, change mode to Playing
      animating = false;
      if(bk_button_depressed) window_stack_pop_all(true); // back = quit (TODO: Menu)
      mode_counter++;
      if(mode_counter>16*4) {
        animating = true;
        set_mode(ModePlaying);
      }
    break;
    
    case ModeGameOver: // Dead, no lives left.  Only back button to exit
      if(bk_button_depressed) window_stack_pop_all(true); // back = quit (TODO: Menu)
    break;
    
    case ModeWinning:  // Pause, erase sprites, flash board, then run init (which will change mode to ModeStarting)
    if(bk_button_depressed) window_stack_pop_all(true); // back = quit (TODO: Menu)
    // todo: flash board, init and begin new level
    break;
    
    case ModeDying:  // Pause, go through death sequence, check GameOver, else run init (which will change mode to ModeStarting)
      if(bk_button_depressed) window_stack_pop_all(true); // back = quit (TODO: Menu)
      mode_counter++;
      if(muncher.frame<19) {  // if death animation has not yet started
        if(mode_counter==30) {  // 30 frame pause before death starts
          muncher.frame = 19;                      // start death animation at frame 0
          muncher.facing = 0;                      // set this to 0 so the math works in the rendering function
          for(uint8_t i=0; i<4; ++i)
            spectre[i].mode = ModeInvisible;       // Remove spectres
          mode_counter=0;
        }
      } else {  // else, death animation has already started -- step through death animation sequence
        if(muncher.frame<27) {   // if still going through animation
          if((mode_counter&3)==0 && mode_counter>30)  // increase animation every 4 frames (pause for 32 at first frame)
            muncher.frame++;
          if(muncher.frame==27)   // if reached the exploding frame
            mode_counter=0;        //   reset the counter
        } else {
          if(mode_counter==8)    // wait 8 frames at the explosion, then make muncher blank
            muncher.frame=253;  // -3 (= 32-3 = 29 = blank)
          if(mode_counter==50) { // wait 50 frames after muncher went blank, then start new round (if enough lives are available)
            if(player.lives==0) {
              set_mode(ModeGameOver);                      // Game Over
            } else {                                       // Else start next round (keep dots and score the same) TODO: Player Two?
              player.lives--;
              init_muncher();
              init_spectres();
              set_mode(ModeRoundBegin);
            }
          }
        }
      }
    break;

    case ModePlaying:  // Most common mode
      muncher.speed = getlevelspeed(player.level);
      muncher_eat_dots();  // Reduce muncher speed if a dot is eaten
      update_movement_via_joystick();
      move_muncher();
      move_spectres();
      check_collisions();
    break;
    
    default:
      // error, bad game mode
    break;
  }
 

  layer_mark_dirty(game_layer);  // Schedule redraw of screen
  looptimer = app_timer_register(UPDATE_MS, gameloop, NULL); // Finished. Wait UPDATE_MS then loop
}




//====================================//
static void game_layer_update(Layer *me, GContext *ctx) {
  char text[10];
  //uint8_t *framebuffer = (uint8_t*)*(uint32_t*)ctx;
  GBitmap* framebuffer_bmp = graphics_capture_frame_buffer(ctx);
  if(framebuffer_bmp) {
    uint8_t *framebuffer = gbitmap_get_data(framebuffer_bmp);
  
    draw_background_fb(framebuffer);
    draw_dots_fb(framebuffer);
    if(player.lives>0) draw_sprite(framebuffer, 4,     3, muncher_color, 38);  // Lives
    if(player.lives>1) draw_sprite(framebuffer, 4+8,   3, muncher_color, 38);  // Lives
    if(player.lives>2) draw_sprite(framebuffer, 4+8+8, 3, muncher_color, 38);  // Lives
    if(player.lives>3) draw_sprite(framebuffer, 4+8+8+8, 3, muncher_color, 38);  // Lives
    if(player.lives>4) draw_sprite(framebuffer, 4+8+8+8+8, 3, muncher_color, 38);  // Lives
    if(player.lives>5) draw_sprite(framebuffer, 4+8+8+8+8+8, 3, muncher_color, 38);  // Lives
    snprintf(text, sizeof(text), "%06ld", (long int)player.score);
    draw_font8_text(framebuffer, 8*6, 2, score_color, text);
    
    bool drawmuncher = false;
    bool drawspectres = false;
    switch(game_mode) {
      case ModeLevelBegin:
      case ModeNewLifeBegin:
        draw_font8_text(framebuffer, 8*4, 66, player_text_color, "PLAYER ONE");
        draw_font8_text(framebuffer, 8*6+1, 96, ready_color, "READY!");  // yellow
      break;
      
      case ModeRoundBegin:
        drawmuncher=true;
        drawspectres = true;
        draw_font8_text(framebuffer, 8*6+1, 96, ready_color, "READY!");  // yellow
      break;
      
      case ModeGameOver:
        draw_font8_text(framebuffer, 8*4, 96, game_over_color, "GAME  OVER");
      break;
      
      default:
        drawmuncher = true;
        drawspectres = true;
      break;
    }
    
    if(drawmuncher)
      draw_actor(framebuffer, muncher.pos.x, muncher.pos.y, muncher_color, 32 + muncher.frame + muncher.facing); // change "muncher.frame>>0" to ">>?" for slower mouth flapping
    

    // Draw Spectres
    if(drawspectres)
     for(uint8_t i=0; i<4; ++i) {
       if(spectre[i].mode!=ModeInvisible) {
         if(spectre[i].mode==ModeScared) {
           draw_actor(framebuffer, spectre[i].pos.x, spectre[i].pos.y, scared_body_color, 17 + ((spectre[i].animate>>2)&1));              // spectre scared body (animate skirt)
           draw_actor(framebuffer, spectre[i].pos.x, spectre[i].pos.y, scared_face_color, 19);              // spectre scared face
         } else {
           draw_actor(framebuffer, spectre[i].pos.x, spectre[i].pos.y, spectre[i].color, 17 + ((spectre[i].animate>>2)&1));              // spectre body (animate skirt)
           #ifdef PBL_COLOR
             draw_actor( framebuffer, spectre[i].pos.x, spectre[i].pos.y, spectre_eye_color, 12 + ((spectre[i].facing)&3));      // spectre eyes
             draw_pupils(framebuffer, spectre[i].pos.x, spectre[i].pos.y, spectre_pupil_color,      ((spectre[i].facing)&3));      // spectre pupils
           #else
             // draw b&w pupils
           #endif
         }
         spectre[i].animate++;  // Only do this most of the time
         // Don't animate while mode is numbers (while being eaten) or while not moving.
         // in fact, move spectre.animate++ to the movespectre function
       }
     }
    
    
    graphics_release_frame_buffer(ctx, framebuffer_bmp);
  }
  if(animating) animate++;  // to animate flashing bigdots
}
  
// ------------------------------------------------------------------------ //
//  Main Functions
// ------------------------------------------------------------------------ //

// animation off
//player 1
// ready
// 4 counts
// ghosts
//ready
//4 counts
// start, animation on
// gameover = no animations


// winning flash gets rid of ghost door
// ghost animation stops (but dots flash) during eating pause
// new round = ready (no anim)
void init_round() {
  init_muncher();
  init_spectres();
  animating = false;
}

void init_level() {
  init_board();
  init_round();
  animating = false;
  animate = 255;  // animation is frozen, but keep big dots lit
}

void init_game() {
  game_layer = root_layer;// create a new layer here //window_get_root_layer(window);
  
  layer_set_update_proc(game_layer, game_layer_update);
  
  load_graphics();
  init_player();
  init_level();
  set_mode(ModeNewLifeBegin);
  animating = false;
  
  window_set_click_config_provider(main_window, game_click_config_provider);
}

void start_game() {
  looptimer = app_timer_register(UPDATE_MS, gameloop, NULL); // Finished. Wait UPDATE_MS then loop
}

void destroy_game() {
  //if(looptimer) app_timer_cancel(looptimer);
  accel_data_service_unsubscribe();
  //TODO: destroy game layer
  //destroy_graphics();
//  for (uint8_t i=0; i<4; i++) for (uint8_t j=0; j<4; j++) gbitmap_destroy(playersprite[i][j]);
//  gbitmap_destroy(background);
}












// ================================================================================================================================================ //
//  Graphical Rendering Functions
// ================================================================================================================================================ //
GBitmap *font8_bmp; uint8_t *font8;
GBitmap *background_bmp; uint8_t *background;
GBitmap *font_sprites_bmp; uint8_t *font_sprites;

// ------------------------------------------------------------------------------------------------------------------------------------------------ //
void load_graphics() {
          font8_bmp = gbitmap_create_with_resource(RESOURCE_ID_FONT8);
     background_bmp = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND);
   font_sprites_bmp = gbitmap_create_with_resource(RESOURCE_ID_FONT_SPRITES);

              font8 = gbitmap_get_data(font8_bmp);
         background = gbitmap_get_data(background_bmp);
       font_sprites = gbitmap_get_data(font_sprites_bmp);
}
// ------------------------------------------------------------------------------------------------------------------------------------------------ //
void unload_graphics() {
  gbitmap_destroy(font8_bmp);
  gbitmap_destroy(background_bmp);
  gbitmap_destroy(font_sprites_bmp);
}
// ------------------------------------------------------------------------------------------------------------------------------------------------ //


#ifdef PBL_BW
// ------------------------------------------------------------------------------------------------------------------------------------------------ //
//  Black and White Drawing Functions
// ------------------------------------------------------------------------ //
void draw_background_fb(uint8_t *fb) {
 for (uint16_t i=0; i<BOARD_Y*5; i++) ((uint32_t*)fb)[i] = 0; // Top rows black
 for (uint16_t i=0, j=BOARD_Y*5; i<155*5; i++,j++) ((uint32_t*)fb)[j] = ((uint32_t*)background)[i]; // Draw Background Image -- equivilant to: graphics_draw_bitmap_in_rect(ctx, background, GRect(0,BOARD_Y,144,155));
}
// ------------------------------------------------------------------------------------------------------------------------------------------------ //

#define   pset(x, y) fb[((y)*20) + ((x) >> 3)] |=  (1 << ((x)&7))
#define pclear(x, y) fb[((y)*20) + ((x) >> 3)] &= ~(1 << ((x)&7))
//uint8_t xaddr = (x*5+BOARD_X+2) >> 3;
//uint8_t xbit  = (x*5+BOARD_X+2) & 7;
//fb[((y*5+BOARD_Y+2)*20) + xaddr] |= 1 << xbit;

void draw_dots_fb(uint8_t *fb) {
  for(uint16_t y=0, i=0; y<MAP_H; y++)
    for(uint16_t x=0; x<MAP_W; x++, i++)
      if(map[i]&16) {      // big dot
        if(((animate>>2)&1) == 1) {
          uint16_t px=x*5+BOARD_X, py=y*5+BOARD_Y;
          pset(px+2, py+0);
          pset(px+1, py+1);
          pset(px+2, py+1);
          pset(px+3, py+1);
          pset(px+0, py+2);
          pset(px+1, py+2);
          pset(px+2, py+2);
          pset(px+3, py+2);
          pset(px+4, py+2);
          pset(px+1, py+3);
          pset(px+2, py+3);
          pset(px+3, py+3);
          pset(px+2, py+4);
        }
      } else if(map[i]&8) { // regular dot
        pset(x*5+BOARD_X+2, y*5+BOARD_Y+2);
      }
}
// ------------------------------------------------------------------------------------------------------------------------------------------------ //
void draw_pupils(uint8_t *fb, int32_t x, int32_t y, uint8_t color, uint8_t facing) {
  x = ((x>>6)*ZOOM) + (((x&63)*ZOOM)/64) + BOARD_X;
  y = ((y>>6)*ZOOM) + (((y&63)*ZOOM)/64) + BOARD_Y;
  switch(facing) {
    case 0:  //00
      pclear(x, y);
      pclear(x+3, y);
    break;
    case 1:  //01
      pclear(x-1, y-2);
      pclear(x+1, y-2);
    break;
    case 2:  //10
      pclear(x, y);
      pclear(x-3, y);
    break;
    case 3:  //11
      pclear(x-1, y+1);
      pclear(x+1, y+1);
    break;
  }
}
// ------------------------------------------------------------------------------------------------------------------------------------------------ //

void fill_rect(uint8_t *screen, GRect rect, uint8_t color) {
  uint8_t data[] = {170, 85};
  if((color&192)!=0b00000000) {    // if not clear
    if(color==0b11000000)      {data[0]=  0; data[1]=  0;} // opaque black
    else if(color==0b11111111) {data[0]=255; data[1]=255;} // opaque white
    else                       {data[0]=170; data[1]= 85;} // opaque grey

    rect.size.w  += rect.origin.x; rect.size.h  += rect.origin.y;                      // convert rect.size.w and rect.size.h to rect.x2 and rect.y2
    rect.size.w   = rect.size.w   < 0 ? 0 : rect.size.w   > 144 ? 144 : rect.size.w;   // make sure rect.x2 is within screen bounds
    rect.origin.x = rect.origin.x < 0 ? 0 : rect.origin.x > 144 ? 144 : rect.origin.x; // make sure rect.x1 is within screen bounds
    rect.size.h   = rect.size.h   < 0 ? 0 : rect.size.h   > 168 ? 168 : rect.size.h;   // make sure rect.y2 is within screen bounds
    rect.origin.y = rect.origin.y < 0 ? 0 : rect.origin.y > 168 ? 168 : rect.origin.y; // make sure rect.y1 is within screen bounds

    GPoint addr;
    addr.y = rect.origin.y*20;
    uint8_t l_mask = 255 << (rect.origin.x%8); // mask for the left side
    uint8_t r_mask = 255 << (rect.size.w%8);   // mask for the right side

    for(int16_t y=0; y<(rect.size.h-rect.origin.y); y++, addr.y+=20) {
      addr.x = rect.origin.x>>3;       // init X memory address
      if  (addr.x >= 0 && addr.x < 19) screen[addr.y + addr.x] = (data[y&1] & l_mask) + (screen[addr.y + addr.x] & ~l_mask); // fill left-side of row
      for(addr.x++; addr.x<(rect.size.w>>3); addr.x++)        if(addr.x >= 0 && addr.x < 19) screen[addr.y + addr.x] = data[y&1]; // fill middle of row
      if  (addr.x >= 0 && addr.x < 19) screen[addr.y + addr.x] = (screen[addr.y + addr.x] & r_mask) + (data[y&1] & ~r_mask); // fill right-side of row
    }
  }
}
// ------------------------------------------------------------------------------------------------------------------------------------------------ //
#endif

  
  
  
  
  
  
  
// ------------------------------------------------------------------------------------------------------------------------------------------------ //
//  Color Drawing Functions
// ------------------------------------------------------------------------ //  
#ifdef PBL_COLOR
// ================================================================ //
//   How to support transparencies and the alpha channel
// ================================================================ //
#define FULL_SHADOW 0b00111111 // 100% black
#define MORE_SHADOW 0b01111111 // 66%  dark
#define SOME_SHADOW 0b10111111 // 33%  shade
#define NONE_SHADOW 0b11111111 // full color

uint8_t shadowtable[] = {192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,  /* ------------------ */ \
                         192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,  /*      0% alpha      */ \
                         192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,  /*        Clear       */ \
                         192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,  /* ------------------ */ \

                         192,192,192,193,192,192,192,193,192,192,192,193,196,196,196,197,  /* ------------------ */ \
                         192,192,192,193,192,192,192,193,192,192,192,193,196,196,196,197,  /*     33% alpha      */ \
                         192,192,192,193,192,192,192,193,192,192,192,193,196,196,196,197,  /*    Transparent     */ \
                         208,208,208,209,208,208,208,209,208,208,208,209,212,212,212,213,  /* ------------------ */ \

                         192,192,193,194,192,192,193,194,196,196,197,198,200,200,201,202,  /* ------------------ */ \
                         192,192,193,194,192,192,193,194,196,196,197,198,200,200,201,202,  /*     66% alpha      */ \
                         208,208,209,210,208,208,209,210,212,212,213,214,216,216,217,218,  /*    Translucent     */ \
                         224,224,225,226,224,224,225,226,228,228,229,230,232,232,233,234,  /* ------------------ */ \

                         192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,  /* ------------------ */ \
                         208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,  /*    100% alpha      */ \
                         224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,  /*      Opaque        */ \
                         240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255}; /* ------------------ */

// ------------------------------------------------------------------------------------------------------------------------------------------------ //
void draw_background_fb(uint8_t *fb) {
  for(uint16_t i=0; i<BOARD_Y*144; ++i) ((uint32_t*)fb)[i] = 0b11000000; // Top rows black

  uint32_t fbpos=BOARD_Y*144;
  for(uint32_t j=0; j<155*144; ++j)
    fb[fbpos++] = ((background[j>>3] >> (7-(j&7))) & 1) ? board_color : background_color;  // draw board blue with black background

  //Spectre Door
  for(fbpos = (BOARD_Y+63)*144 + 67; fbpos<(BOARD_Y+63)*144 + 67 + 10; ++fbpos)
    fb[fbpos] = door_color;
}
// ------------------------------------------------------------------------------------------------------------------------------------------------ //
void draw_dots_fb(uint8_t *fb) {
  #define dotcolor 0b11111111
  uint32_t addr = ((BOARD_Y+2)*144)+BOARD_X+2;
  for(uint16_t y=0, i=0; y<MAP_H; ++y, addr+=(144*4) + (144-(5*MAP_W)))
    for(uint16_t x=0; x<MAP_W; ++x, ++i, addr+=5)  // +5 is because each square is 5 pixels apart
      if(map[i]&16) {        // big dot
        if((animate>>2)&1) {  // Big Dot Flashes (meaning draw or don't draw).  change >>2 to change flash rate
          fb[addr-144-144-1] = dotcolor;
          fb[addr-144-144] = dotcolor;
          fb[addr-144-144+1] = dotcolor;
          
          fb[addr-144-2] = dotcolor;
          fb[addr-144-1] = dotcolor;
          fb[addr-144] = dotcolor;
          fb[addr-144+1] = dotcolor;
          fb[addr-144+2] = dotcolor;
          
          fb[addr-2] = dotcolor;
          fb[addr-1] = dotcolor;
          fb[addr] = dotcolor;
          fb[addr+1] = dotcolor;
          fb[addr+2] = dotcolor;
          
          fb[addr+144-2] = dotcolor;
          fb[addr+144-1] = dotcolor;
          fb[addr+144] = dotcolor;
          fb[addr+144+1] = dotcolor;
          fb[addr+144+2] = dotcolor;
          
          fb[addr+144+144-1] = dotcolor;
          fb[addr+144+144] = dotcolor;
          fb[addr+144+144+1] = dotcolor;
        }
      } else if(map[i]&8) {  // regular dot
        fb[addr] = dotcolor;
      }
}
// ------------------------------------------------------------------------------------------------------------------------------------------------ //
void draw_pupils(uint8_t *fb, int32_t x, int32_t y, uint8_t color, uint8_t facing) {
  //NOTE: Will probably break when going out of screen bounds
  x=(((x>>6)*ZOOM) + (((x&63)*ZOOM)/64) + BOARD_X);
  y=(((y>>6)*ZOOM) + (((y&63)*ZOOM)/64) + BOARD_Y);
  uint32_t addr = (y*144)+x;
  if(facing&1) {
    addr += ((facing&2)?+144:-288);
    fb[addr - 1]=0;
    fb[addr + 1]=0;
  } else {
      fb[addr]=0;
      fb[addr + ((facing&2)?-3:3)]=0;
  }
//   switch(facing) {
//     case 0:  //00
//       fb[addr]=0;
//       fb[addr+3]=0;
//     break;
//     case 1:  //01
//       fb[addr-144-144-1]=0;
//       fb[addr-144-144+1]=0;
//     break;
//     case 2:  //10
//       fb[addr]=0;
//       fb[addr-3]=0;
//     break;
//     case 3:  //11
//       fb[addr+144-1]=0;
//       fb[addr+144+1]=0;
//     break;
//   }
}
// ------------------------------------------------------------------------------------------------------------------------------------------------ //
void fill_rect(uint8_t *screen, GRect rect, uint8_t color) {
  uint8_t bg_opacity = (~color)&0b11000000;
  rect.size.w  += rect.origin.x; rect.size.h  += rect.origin.y;                      // convert rect.size.w and rect.size.h to rect.x2 and rect.y2
  rect.size.w   = rect.size.w   < 0 ? 0 : rect.size.w   > 144 ? 144 : rect.size.w;   // make sure rect.x2 is within screen bounds
  rect.origin.x = rect.origin.x < 0 ? 0 : rect.origin.x > 144 ? 144 : rect.origin.x; // make sure rect.x1 is within screen bounds
  rect.size.h   = rect.size.h   < 0 ? 0 : rect.size.h   > 168 ? 168 : rect.size.h;   // make sure rect.y2 is within screen bounds
  rect.origin.y = rect.origin.y < 0 ? 0 : rect.origin.y > 168 ? 168 : rect.origin.y; // make sure rect.y1 is within screen bounds

  rect.origin.y*=144; rect.size.h*=144;
  for (uint16_t y_addr=rect.origin.y; y_addr<rect.size.h; y_addr+=144)
    for(uint16_t x_addr=rect.origin.x; x_addr<rect.size.w; x_addr++)
      screen[y_addr+x_addr] = shadowtable[bg_opacity + (screen[y_addr+x_addr]&63)] + color;
}
// ------------------------------------------------------------------------------------------------------------------------------------------------ //


#endif
// ------------------------------------------------------------------------------------------------------------------------------------------------ //
  
  
  
  
  
  
  
  
  
  
  
  
  
// void draw_sprite8(uint8_t *fb, int16_t start_x, int16_t start_y, uint8_t hflip, uint8_t vflip, uint8_t color, uint8_t spr) {  // uint8_t color isn't used in B&W
//   hflip = hflip ? 1 : 0;  vflip = vflip ? 1 : 0;  // makes sure hflip and vflip are either 1 or 0
//   uint16_t left   = (start_x <     0) ? (start_x >  -8) ?   0 - start_x : 8 : 0;
//   uint16_t right  = (start_x > 144-8) ? (start_x < 144) ? 144 - start_x : 0 : 8;
//   uint16_t top    = (start_y <     0) ? (start_y >  -8) ?   0 - start_y : 8 : 0;
//   uint16_t bottom = (start_y > 168-8) ? (start_y < 168) ? 168 - start_y : 0 : 8;
  
//   uint8_t    *row = font8+(spr&3) + ((spr&252)*8) + ((vflip?bottom:top)*4);
//   uint16_t y_addr = (start_y + top) * IF_COLORBW(144, 20);
  
//   for(uint16_t y=top; y<bottom; ++y) {
//     for(uint16_t x=left; x<right; ++x) {
//       #ifdef PBL_BW
//         //fb[y_addr + ((start_x+x) >> 3)] &= ~(1 << ((start_x+x)&7)); // Black Background (comment both out for clear background)
//         //fb[y_addr + ((start_x+x) >> 3)] |=  (1 << ((start_x+x)&7)); // White Background (comment both out for clear background)
//         //fb[y_addr + ((start_x+x) >> 3)] &= ~((((*row>>x)&1)) << ((start_x+x)&7)); // Black Pixel
//           fb[y_addr + ((start_x+x) >> 3)] |=  ((((*row>>x)&1)) << ((start_x+x)&7)); // White Pixel
//       #else
//         //if((*row>>x)&1)   // horizontally flipped
//         if((*row&(128>>x))) // normal
//           fb[y_addr + start_x + x] = color;
//           // else fb[y_addr + x] = background_color;
//       #endif
//     }
//     y_addr += IF_COLORBW(144, 20);
//     row += vflip ? -4 : 4;
//   }
// }


void draw_sprite8(uint8_t *fb, uint8_t *font, int16_t start_x, int16_t start_y, uint8_t color, uint8_t spr) {  // in B&W, color=0 is black, else white
  uint16_t left   = (start_x <     0) ? (start_x >  -8) ?   0 - start_x : 8 : 0;
  uint16_t right  = (start_x > 144-8) ? (start_x < 144) ? 144 - start_x : 0 : 8;
  uint16_t top    = (start_y <     0) ? (start_y >  -8) ?   0 - start_y : 8 : 0;
  uint16_t bottom = (start_y > 168-8) ? (start_y < 168) ? 168 - start_y : 0 : 8;
  uint8_t    *row = font + (spr&3) + ((spr&252)*8) + (top*4);
  uint16_t y_addr = (start_y + top) * IF_COLOR_BW(144, 20);
  
  for(uint16_t y=top; y<bottom; ++y) {
    for(uint16_t x=left; x<right; ++x) {
      #ifdef PBL_BW
        //fb[y_addr + ((start_x+x) >> 3)] &= ~(1 << ((start_x+x)&7)); // Black Background (comment both out for clear background)
        //fb[y_addr + ((start_x+x) >> 3)] |=  (1 << ((start_x+x)&7)); // White Background (comment both out for clear background)
        if(color)
          fb[y_addr + ((start_x+x) >> 3)] |=  ((((*row>>x)&1)) << ((start_x+x)&7)); // White Pixel
        else
          fb[y_addr + ((start_x+x) >> 3)] &= ~((((*row>>x)&1)) << ((start_x+x)&7)); // Black Pixel
      #else
        //if((*row>>x)&1)   // horizontally flipped
        if((*row&(128>>x))) // normal
          fb[y_addr + start_x + x] = color;
          // else fb[y_addr + x] = background_color;
      #endif
    }
    y_addr += IF_COLOR_BW(144, 20);
    row += 4;
  }
}

void draw_font8_text(uint8_t *fb, int16_t x, int16_t y, uint8_t color, char *str) { // str points to zero-terminated string
  uint8_t strpos=0;
  while(str[strpos]>0) {
    if(x>136) {x=0; y+=8;}  // wrap horizontally
    if(y>160) y=0;          // wrap vertically
    draw_sprite8(fb, font8, x, y, color, str[strpos]);
    x+=8; strpos++;
  }
}

void draw_font8(uint8_t *fb, int32_t x, int32_t y, uint8_t color, uint8_t spr) {
  draw_sprite8(fb, font8, x, y, color, spr);
}

void draw_sprite(uint8_t *fb, int32_t x, int32_t y, uint8_t color, uint8_t spr) {
  draw_sprite8(fb, font_sprites, x, y, color, spr);
}

// draw_actor is much like draw_sprite, but converts 64px-per-tile to 5px-per-tile
// also, actor x,y is the center of the sprite, draw_sprite puts x,y on the upper-left corner
void draw_actor(uint8_t *fb, int32_t x, int32_t y, uint8_t color, uint8_t spr) {
  draw_sprite8(fb, font_sprites, (((x>>6)*5) + (((x&63)*5)/64) + BOARD_X - 3), (((y>>6)*5) + (((y&63)*5)/64) + BOARD_Y - 3), color, spr);
  
  
  //IF_COLOR(fb[((((y>>6)*5) + (((y&63)*5)/64) + BOARD_Y)*144)+(((x>>6)*5) + (((x&63)*5)/64) + BOARD_X)]=0b11001100); // put green dot at center of actor (actor's x,y pos)
}

// ------------------------------------------------------------------------ //
//  Main Functions
// ------------------------------------------------------------------------ //
static void main_window_load(Window *window) {
  root_layer = window_get_root_layer(window);
  init_game();
  start_game();
}

static void main_window_unload(Window *window) {
  destroy_game();
}

static void main_window_appear(Window *window) {
  APP_LOG(APP_MSG_OK, "A Wild Main Window Appeared!");
}    
static void main_window_disappear(Window *window) {
  APP_LOG(APP_MSG_OK, "The Main Window Ran Away!");
}

static void init(void) {
  // Set up and push main window
  main_window = window_create();
  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
    .appear = main_window_appear,
    .disappear = main_window_disappear
  });
  
  //IF_SDK2(window_set_fullscreen(main_window, true));
  
  bool emulator = watch_info_get_model()==WATCH_INFO_MODEL_UNKNOWN;
  if(emulator)
    light_enable(true);
  
  //Set up other stuff
  srand(time(NULL));  // Seed randomizer
  accel_data_service_subscribe(0, NULL);  // We may be using the accelerometer TODO: only turn on acceleromter when in use
  
  //Begin
  window_stack_push(main_window, true); // Display window -- layer is now dirty.  Timer callback will be scheduled after dirty layer is written.  
}
  
static void deinit(void) {
  unload_graphics();
  accel_data_service_unsubscribe();
  window_destroy(main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}