
// libmylcd
// An LCD framebuffer library
// Michael McElligott
// okio@users.sourceforge.net

//  Copyright (c) 2005-2009  Michael McElligott
// 
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU LIBRARY GENERAL PUBLIC LICENSE
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU LIBRARY GENERAL PUBLIC LICENSE for details.



#include "../common.h"
#include "stc.h"







typedef struct{
	TFRAME *frame;
	int x;
	int y;
	int blockWidth;
	int blockHeight;
}TGAMEBLOCK;


	
static const int colourLookupTable[10] = {
	0xFF000000,
	0xFF00FFFF,
	0xFFFF0000,
	0xFF0000FF,
	0xFFFF6400,
	0xFF00FF00,
	0xFFFFFF00,
	0xFFFF00FF,
	127<<24 | 0x000000,
	127<<24 | 0xFFFFFF
};


static int colourLookup (const int idx)
{
	return colourLookupTable[idx];
}

int platformInit (StcGame *game)
{
//	printf("platformInit\n");
	
	game->tetris = my_calloc(1, sizeof(StcPlatform));
	if (game->tetris)
		return ERROR_NONE;
	else
		return ERROR_NO_MEMORY;
}

/* Clear resources used by platform */
void platformEnd (StcGame *game)
{
	my_free(game->tetris);
	//printf("platformEnd\n");
}

/* Read input device and notify game */
void platformReadInput (StcGame *game)
{
	//printf("platformReadInput\n");
}

int tetrisGameInput (TVLCPLAYER *vp, TTETRIS *tetris, const int key)
{
	
	StcGame *game = tetris->game;

	switch (key){
	  case 'a':
	  case 'A':
	  	gameOnKeyDown(game, EVENT_MOVE_LEFT);
	  	gameOnKeyUp(game, EVENT_MOVE_LEFT);
	  	break;
	  	  
	  case 'd':	
	  case 'D':
	  	gameOnKeyDown(game, EVENT_MOVE_RIGHT);
	  	gameOnKeyUp(game, EVENT_MOVE_RIGHT);
	  	break;

	  case 'w':
	  case 'W':
		gameOnKeyDown(game, EVENT_ROTATE_CCW);
		gameOnKeyUp(game, EVENT_ROTATE_CCW);
		break;
	  	  
	  case 'e':		  	  	
	  case 'E':
		gameOnKeyDown(game, EVENT_ROTATE_CW);
		gameOnKeyUp(game, EVENT_ROTATE_CW);
		break;

	  case 's':
	  case 'S':
		gameOnKeyDown(game, EVENT_MOVE_DOWN);
		gameOnKeyUp(game, EVENT_MOVE_DOWN);
		break;
	  	  			  	
	  case ' ':
		gameOnKeyDown(game, EVENT_DROP);
		gameOnKeyUp(game, EVENT_DROP);
		break;
	
	  case 'r': 
	  case 'R': 
		gameOnKeyDown(game, EVENT_RESTART);
		gameOnKeyUp(game, EVENT_RESTART);
		break;
			
	  case 'p': 
	  case 'P': 
		gameOnKeyDown(game, EVENT_PAUSE);
		gameOnKeyUp(game, EVENT_PAUSE);
		break;

	  case 13:
	  case 27:
	  case 'q':
	  case 'Q':
		page2SetPrevious(tetris);
		gameOnKeyDown(game, EVENT_QUIT);
		break; 
	}

	return 1;
}

static void drawCell (TGAMEBLOCK *gb, const int blockx, const int blocky, const int idx)
{
	const int x = (blockx * gb->blockWidth) + gb->x;
	const int y = (blocky * gb->blockHeight) + gb->y;
	lDrawRectangleFilled(gb->frame, x, y, x+gb->blockWidth-1, y+gb->blockHeight-1, colourLookup(idx));
}

void platformOnFilledRow (StcGame *game, int y)
{
	//drawCell(game->tetris->frame, 0, y, 8);
	//printf("row filled: %i\n", y);
}

void platformOnFilledRows (StcGame *game, int rowsFilled)
{
	//printf("total rows filled: %i\n", rowsFilled);
}

/* Render the state of the game */
void platformRenderGame (StcGame *game, const int posX, const int posY, const int blockWidth, const int blockHeight)
{
	//printf("platformRenderGame\n");

	TGAMEBLOCK gb;
	gb.frame = game->tetris->frame;
	gb.x = posX;
	gb.y = posY;
	gb.blockWidth = blockWidth;
	gb.blockHeight = blockHeight;

	// dropped blocks
	for (int j = 0; j < BOARD_TILEMAP_HEIGHT; j++){
		for (int i = 0; i < BOARD_TILEMAP_WIDTH; i++){
			int cell = game->map[i][j];
			if (cell)
				drawCell(&gb, i, j, cell);
		}
	}

	// falling block
	for (int j = 0; j < TETROMINO_SIZE; j++) {
		for (int i = 0; i < TETROMINO_SIZE; i++) {
			int cell = game->fallingBlock.cells[i][j];
			if (cell)
				drawCell(&gb, game->fallingBlock.x + i, game->fallingBlock.y + j, cell);
		}
	}
	
	// shadow block
	for (int j = 0; j < TETROMINO_SIZE; j++) {
		for (int i = 0; i < TETROMINO_SIZE; i++) {
			if (game->fallingBlock.cells[i][j])
				drawCell(&gb, game->fallingBlock.x + i, game->fallingBlock.y + j + game->shadowGap, COLOR_WHITE);
		}
	}	

	// next block
	for (int j = 0; j < TETROMINO_SIZE; j++) {
		for (int i = 0; i < TETROMINO_SIZE; i++) {
			int cell = game->nextBlock.cells[i][j];
			if (cell)
				drawCell(&gb, game->nextBlock.x + i + 12 ,game->nextBlock.y + j + 9, cell);
		}
	}

	// left
	lDrawLine(gb.frame, posX-1, posY, posX-1, posY+(BOARD_TILEMAP_HEIGHT*gb.blockHeight)+1, 255<<24|COL_BLUE_SEA_TINT);
	lDrawLine(gb.frame, posX-2, posY, posX-2, posY+(BOARD_TILEMAP_HEIGHT*gb.blockHeight)+1, 255<<24|COL_BLUE_SEA_TINT);
	
	//right
	lDrawLine(gb.frame, posX+(BOARD_TILEMAP_WIDTH*gb.blockWidth)+1, posY, posX+(BOARD_TILEMAP_WIDTH*gb.blockWidth)+1, posY+(BOARD_TILEMAP_HEIGHT*gb.blockHeight)+1, 255<<24|COL_BLUE_SEA_TINT);
	lDrawLine(gb.frame, posX+(BOARD_TILEMAP_WIDTH*gb.blockWidth)+2, posY, posX+(BOARD_TILEMAP_WIDTH*gb.blockWidth)+2, posY+(BOARD_TILEMAP_HEIGHT*gb.blockHeight)+1, 255<<24|COL_BLUE_SEA_TINT);
	
	//bottom
	lDrawLine(gb.frame, posX-1, posY+(BOARD_TILEMAP_HEIGHT*gb.blockHeight)+1, posX+(BOARD_TILEMAP_WIDTH*gb.blockWidth)+1, posY+(BOARD_TILEMAP_HEIGHT*gb.blockHeight)+1, 255<<24|COL_BLUE_SEA_TINT);
	lDrawLine(gb.frame, posX-1, posY+(BOARD_TILEMAP_HEIGHT*gb.blockHeight)+2, posX+(BOARD_TILEMAP_WIDTH*gb.blockWidth)+1, posY+(BOARD_TILEMAP_HEIGHT*gb.blockHeight)+2, 255<<24|COL_BLUE_SEA_TINT);

	lSetForegroundColour(gb.frame->hw, 255<<24|COL_WHITE);
	lSetBackgroundColour(gb.frame->hw, 0x00000000);

	int x = gb.frame->width * 0.02;
	int y = gb.frame->height * 0.06;
	if (x < 4) x = 4;

	
	lSetRenderEffect(gb.frame->hw, LTR_BLUR5);
	lSetFilterAttribute(gb.frame->hw, LTR_BLUR5, LTRA_BLUR_COLOUR, COL_PURPLE_GLOW);
	lPrintf(gb.frame, x, y   , TETRIS_STATS_FONT, LPRT_CPY, "Score: %i", game->stats.score);
	lPrintf(gb.frame, x, y+38, TETRIS_STATS_FONT, LPRT_CPY, "Lines: %i", game->stats.lines);
	lPrintf(gb.frame, x, y+(38*2), TETRIS_STATS_FONT, LPRT_CPY, "Level: %i", game->stats.level);
	
	//lPrintf(gb.frame, x, y+(40*4), TETRIS_KEYS_FONT, LPRT_CPY, "%s", "Keys:");
	
	lSetFilterAttribute(gb.frame->hw, LTR_BLUR5, LTRA_BLUR_COLOUR, COL_GREEN_TINT);
	y = y+(36*5)+14;
	x += 4;
	const int lineHeight = 34;
	lPrintf(gb.frame, x, y,        TETRIS_KEYS_FONT, LPRT_CPY, "%s", "Left:");
	lPrintf(gb.frame, x, y+lineHeight,     TETRIS_KEYS_FONT, LPRT_CPY, "%s", "Right:");
	lPrintf(gb.frame, x, y+(lineHeight*2), TETRIS_KEYS_FONT, LPRT_CPY, "%s", "Rotate:");
	lPrintf(gb.frame, x, y+(lineHeight*3), TETRIS_KEYS_FONT, LPRT_CPY, "%s", "Down:");
	lPrintf(gb.frame, x, y+(lineHeight*4), TETRIS_KEYS_FONT, LPRT_CPY, "%s", "Drop:");
	lPrintf(gb.frame, x, y+(lineHeight*5), TETRIS_KEYS_FONT, LPRT_CPY, "%s", "Restart:");
	lPrintf(gb.frame, x, y+(lineHeight*6), TETRIS_KEYS_FONT, LPRT_CPY, "%s", "Pause:");


	lSetFilterAttribute(gb.frame->hw, LTR_BLUR5, LTRA_BLUR_COLOUR, COL_BLUE_SEA_TINT);
	x += 150;
	lPrintf(gb.frame, x, y,        TETRIS_KEYS_FONT, LPRT_CPY, "%s", "A");
	lPrintf(gb.frame, x, y+lineHeight,     TETRIS_KEYS_FONT, LPRT_CPY, "%s", "D");
	lPrintf(gb.frame, x, y+(lineHeight*2), TETRIS_KEYS_FONT, LPRT_CPY, "%s", "W");
	lPrintf(gb.frame, x, y+(lineHeight*3), TETRIS_KEYS_FONT, LPRT_CPY, "%s", "S");
	lPrintf(gb.frame, x, y+(lineHeight*4), TETRIS_KEYS_FONT, LPRT_CPY, "%s", "Space");
	lPrintf(gb.frame, x, y+(lineHeight*5), TETRIS_KEYS_FONT, LPRT_CPY, "%s", "R");
	lPrintf(gb.frame, x, y+(lineHeight*6), TETRIS_KEYS_FONT, LPRT_CPY, "%s", "P");
	
	lSetRenderEffect(gb.frame->hw, LTR_DEFAULT);
}


/* Return the current system time in milliseconds */
long platformGetSystemTime (void)
{
	return /*timeGetTime();*/ getTickCount();
}

/* Initialize the random number generator */
void platformSeedRandom (long seed)
{
	srand(seed);
}

/* Return a random positive integer number */
int platformRandom (void)
{
	return rand();
}

int tetrisGameInit (TTETRIS *tetris, TFRAME *frame)
{
	tetris->game = my_calloc(1, sizeof(StcGame));
	if (!tetris->game) return 0;

	
	int err = gameInit(tetris->game);
	if (err != ERROR_NONE){
		//printf("stc init failed: %i\n", err);
	}
	
	tetris->game->tetris->frame = frame;
	return 1;
}

void tetrisGameDraw (TTETRIS *tetris, TFRAME *frame, const int x, const int y)
{
	gameUpdate(tetris->game, x, y, tetris->blockWidth, tetris->blockHeight);
	//tetris->game->stateChanged = 0;
}

void tetrisGameClose (TTETRIS *tetris)
{	
	gameEnd(tetris->game);
	my_free(tetris->game);
	tetris->game = NULL;
}
