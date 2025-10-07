// background.h

#ifndef BACKGROUND_H
#define BACKGROUND_H
#include "vga.h"

// Images
extern unsigned char test1[RES_Y][RES_X];
extern unsigned char Bliss[RES_Y][RES_X];
extern unsigned char KTH[RES_Y][RES_X];
extern unsigned char Icecream[RES_Y][RES_X];

// Sprites
extern const unsigned char arrowSprite[20][20];
extern const unsigned char handSprite[20][20];

// Backgrounds

// Stylized
extern const unsigned char mainMenuStyle[RES_Y][RES_X];
extern const unsigned char processMenuStyle[RES_Y][RES_X];
extern const unsigned char uploadMenuStyle[RES_Y][RES_X];

// Original
/*
extern const unsigned char mainMenu[RES_Y][RES_X];
extern const unsigned char ProcessMenu[RES_Y][RES_X];
extern const unsigned char UploadMenu[RES_Y][RES_X];
*/

#endif