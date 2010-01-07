#define LEVEL 18
#define X 15
#define Y 18
#define EXIT_X 3
#define EXIT_Y 1
#define PLAYERS 3
#define BLOCKS 14
#define ROTATORS 2
#define BLOCKX 3
#define BLOCKY 3
#define HOLES 3
#define MAX_STEPS (130+6)
#define COMPRESSED_ALIGN_BITS 3
const char level[Y][X+1] = {
"###############",
"#  2 ##########",
"#    ##########",
"###  ## e #####",
"##  ##  e  ####",
"#OO## fffMM ###",
"#aaOcc gg M  ##",
"## bcc  hhii ##",
"### ccAA j    #",
"#### dA kk  # #",
"####  #   p##3#",
"####  ###np####",
"#####m###  ####",
"##### ####  ###",
"####  ##### ###",
"#### ###### ###",
"###4 ####1  ###",
"###############",
};
