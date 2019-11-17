/* ========================================================================== */
/*                          STC - SIMPLE TETRIS CLONE                         */
/* -------------------------------------------------------------------------- */
/*   Platform interface definition.                                           */
/*                                                                            */
/*   Copyright (c) 2010 Laurens Rodriguez Oscanoa.                            */
/*   This code is licensed under the MIT license:                             */
/*   http://www.opensource.org/licenses/mit-license.php                       */
/* -------------------------------------------------------------------------- */

#ifndef STC_SRC_PLATFORM_H_
#define STC_SRC_PLATFORM_H_

#include "mylcd.h"

/* Game forward declaration */
typedef struct StcGame StcGame;

/* Initializes platform */
extern int platformInit(StcGame *game);

/* Clear resources used by platform */
extern void platformEnd(StcGame *game);

/* Read input device and notify game */
extern void platformReadInput(StcGame *game);

/* Render the state of the game */
extern void platformRenderGame(StcGame *game, const int x, const int y, const int blockWidth, const int blockHeight);

/* Return the current system time in milliseconds */
extern long platformGetSystemTime(void);

/* Initialize the random number generator */
extern void platformSeedRandom(long seed);

/* Return a random positive integer number */
extern int platformRandom(void);

extern void platformOnFilledRow (StcGame *game, int y_row);
extern void platformOnFilledRows (StcGame *game, int rowsFilled);


/*
 * Data structure that is going to hold platform dependent
 * information about our game. It's defined in the platform header.
 */


#include "../common.h"

typedef struct {
	TFRAME *frame;
}StcPlatform;


/*
int tetrisInput (TVLCPLAYER *vp, TTETRIS *tetris, const int key); 
void tetrisClose (TTETRIS *tetris);
void tetrisDraw (TTETRIS *tetris, TFRAME *frame);
int tetrisInit (TTETRIS *tetris, TFRAME *frame);*/

#endif /* STC_SRC_PLATFORM_H_ */
