typedef unsigned short u16;
typedef unsigned int u32;
typedef int bool;

#define true 1
#define false 2

#define HAS_SNAKE 1
#define HAS_FOOD  2
#define EMPTY     0

#include <stdlib.h>
#include "text.h"
#include "mainmenu.h"
#include "apple.h"
#include "gameover.h"


#define NUMOBJS 7

#define REG_DISPCTL *(unsigned short *)0x4000000
#define MODE3 3
#define BG2_ENABLE (1<<10)

#define SCANLINECOUNTER *(volatile unsigned short *)0x4000006

#define OFFSET(row, col, rowlen)  ((row)*(rowlen)+(col))

unsigned short *videoBuffer = (unsigned short *)0x6000000;

#define COLOR(r, g, b) ((r) | (g)<<5 | (b)<<10)
#define WHITE COLOR(31,31,31)
#define RED COLOR(31,0,0)
#define GREEN COLOR(0,31,0)
#define BLUE COLOR(0,0,31)
#define MAGENTA COLOR(31, 0, 31)
#define CYAN COLOR(0, 31, 31)
#define YELLOW COLOR(31, 31, 0)
#define GREY COLOR(25, 25, 25)
#define BLACK 0

// Buttons

#define BUTTON_A		(1<<0)
#define BUTTON_B		(1<<1)
#define BUTTON_SELECT	(1<<2)
#define BUTTON_START	(1<<3)
#define BUTTON_RIGHT	(1<<4)
#define BUTTON_LEFT		(1<<5)
#define BUTTON_UP		(1<<6)
#define BUTTON_DOWN		(1<<7)
#define BUTTON_R		(1<<8)
#define BUTTON_L		(1<<9)

#define KEY_DOWN_NOW(key)  (~(BUTTONS) & key)

#define BUTTONS *(volatile unsigned int *)0x4000130

/* DMA */

#define REG_DMA0SAD         *(volatile u32*)0x40000B0 		// source address
#define REG_DMA0DAD         *(volatile u32*)0x40000B4       // destination address
#define REG_DMA0CNT         *(volatile u32*)0x40000B8       // control register

// DMA channel 1 register definitions
#define REG_DMA1SAD         *(volatile u32*)0x40000BC 		// source address
#define REG_DMA1DAD         *(volatile u32*)0x40000C0       // destination address
#define REG_DMA1CNT         *(volatile u32*)0x40000C4       // control register

// DMA channel 2 register definitions
#define REG_DMA2SAD         *(volatile u32*)0x40000C8 		// source address
#define REG_DMA2DAD         *(volatile u32*)0x40000CC       // destination address
#define REG_DMA2CNT         *(volatile u32*)0x40000D0       // control register

// DMA channel 3 register definitions
#define REG_DMA3SAD         *(volatile u32*)0x40000D4 		// source address
#define REG_DMA3DAD         *(volatile u32*)0x40000D8       // destination address
#define REG_DMA3CNT         *(volatile u32*)0x40000DC       // control register

typedef struct
{
	const volatile void *src;
	volatile void *dst;
	unsigned int cnt;
} DMA_CONTROLLER;

#define DMA ((volatile DMA_CONTROLLER *) 0x040000B0)

// Defines
#define DMA_CHANNEL_0 0
#define DMA_CHANNEL_1 1
#define DMA_CHANNEL_2 2
#define DMA_CHANNEL_3 3

#define DMA_DESTINATION_INCREMENT (0 << 21)
#define DMA_DESTINATION_DECREMENT (1 << 21)
#define DMA_DESTINATION_FIXED (2 << 21)


#define DMA_SOURCE_INCREMENT (0 << 23)
#define DMA_SOURCE_DECREMENT (1 << 23)
#define DMA_SOURCE_FIXED (2 << 23)

#define DMA_REPEAT (1 << 25)

#define DMA_16 (0 << 26)
#define DMA_32 (1 << 26)

#define DMA_NOW (0 << 28)
#define DMA_AT_VBLANK (1 << 28)
#define DMA_AT_HBLANK (2 << 28)
#define DMA_AT_REFRESH (3 << 28)

#define DMA_IRQ (1 << 30)
#define DMA_ON (1 << 31)

#define BLOCK_SIZE 5
#define NUM_FOOD 3
#define FOOD_GONE -11

// Prototypes
void setPixel(int row, int col, unsigned short color);
void drawRect(int row, int col, int height, int width, unsigned short color);
//void delay(int n);


void waitForVblank();

typedef struct {
	int direction;
	int status;
} TILE;

typedef struct {
	int head_row;
	int head_col;
	int tail_row;
	int tail_col;
} SNAKE;

typedef struct {
	int row;
	int col;
} FOOD;

// State enum definition
enum GBAState {
	MAIN_MENU,
	GAME,
	GAME_OVER,
	NODRAW,
};

void drawChar(int row, int col, char ch, u16 color)
{
	for(int r=0; r<8; r++)
	{
		for(int c=0; c<6; c++)
		{
			if(fontdata_6x8[OFFSET(r, c, 6)+ch*48])
			{
				setPixel(row+r, col+c, color);
			}
		}
	}
}

void drawString(int row, int col, char *str, u16 color)
{
	while(*str)
	{
		drawChar(row, col, *str++, color);
		col += 6;

	}
}

void delay(volatile int time) {
	for (int i = 0; i < (time * 100); i++) {}
}

void drawImage3(int row, int col, int width, int height, const u16 *image) {
	for (int i = 0; i < height; i++) {
		REG_DMA3SAD = (u32) &image[OFFSET(i, 0, width)];
		REG_DMA3DAD = (u32) &videoBuffer[OFFSET(row + i, col, 240)];
		REG_DMA3CNT = width | DMA_ON;
	}
}

char string[4];

int main()
{
	REG_DISPCTL = MODE3 | BG2_ENABLE;

	enum GBAState state = MAIN_MENU;

	//int delta = 1; // speed in which movement occurs
	int direction = 0; // start going no where
	int snake_size = 0; // actual size - 1. or amount of snake bits + head
	int old_size = snake_size;
	int collided = 0;
	int keyPressed = 0;


	TILE board[32][48];
	// initilize an empty board
	for (int i = 0; i < 32; i++) {
		for (int j = 0; j < 48; j++) {
			board[i][j].direction = 0;
			board[i][j].status = 0;
		}
	}

	// create a snake and then put it on the board
	SNAKE snake;

	snake.head_row = 5;
	snake.head_col = 5;
	snake.tail_row = snake.head_row;
	snake.tail_col = snake.head_col;

	board[snake.head_row][snake.head_col].status = HAS_SNAKE;


	// Allocate some food
	FOOD food[NUM_FOOD];

	// initilize the food to be non-existent
	for (int i = 0; i < NUM_FOOD; i++) {
		food[i].row = FOOD_GONE;
		food[i].col = FOOD_GONE;
	}



	while (1) {

		if (KEY_DOWN_NOW(BUTTON_SELECT)) {
			state = MAIN_MENU;
			snake.head_row = 5;
			snake.head_col = 5;
			snake.tail_row = snake.head_row;
			snake.tail_col = snake.head_col;

			direction = 0; // start going no where
			snake_size = 0; // actual size - 1. or amount of snake bits + head
			old_size = snake_size;
			collided = 0;

			for (int i = 0; i < NUM_FOOD; i++) {
				food[i].row = FOOD_GONE;
				food[i].col = FOOD_GONE;
			}

			// initilize an empty board
			for (int i = 0; i < 32; i++) {
				for (int j = 0; j < 48; j++) {
					board[i][j].direction = 0;
					board[i][j].status = 0;
				}
			}

		}

		switch (state) {
		case MAIN_MENU:
			drawImage3(0, 0, 240, 160, mainmenu);
			if (KEY_DOWN_NOW(BUTTON_START) && keyPressed == 0) {
				state = GAME;
				drawRect(0, 0, 160, 240, BLACK);
				keyPressed = 1;
			}
			if (!KEY_DOWN_NOW(BUTTON_START)) {
				keyPressed = 0;
			}
			break;
		case GAME:
			//
			// MAIN GAME CODE
			//

			if (KEY_DOWN_NOW(BUTTON_UP)) {
				if (direction != 3) {
					direction = 1;
				}
			} else if (KEY_DOWN_NOW(BUTTON_LEFT)) {
				if (!direction != 4) {
					direction = 2;
				}
			} else if (KEY_DOWN_NOW(BUTTON_DOWN)) {
				if (!direction != 1) {
					direction = 3;
				}
			} else if (KEY_DOWN_NOW(BUTTON_RIGHT)) {
				if (!direction != 2) {
					direction = 4;
				}
			}

			/*if (KEY_DOWN_NOW(BUTTON_START)) {
				direction = 0;
			}*/


			// Check to see if the snake hits the edge of the map
			if (snake.head_row >= 32) {
				state = GAME_OVER;
				break;
			} else if (snake.head_row <= 0) {
				state = GAME_OVER;
				break;
			}
			if (snake.head_col >= 48) {
				state = GAME_OVER;
				break;
			} else if (snake.head_col <= 0) {
				state = GAME_OVER;
				break;
			}

			/*
			if (old_size != snake_size) {
				old_size = snake_size;
				//drawChar(120, 120, (char) (old_size + '0'), BLACK);
				drawRect(120, 120, 10, 10, BLACK);
			}
			drawChar(120, 120, (char) (snake_size+'0'), WHITE); */
			/*
			drawRect(130, 120, 30, 30, BLACK);
			drawChar(130, 132, (char) ((snake[snake_size].row % 10) + '0'), WHITE);
			drawChar(130, 123, (char) (((snake[snake_size].row % 100) / 10) + '0'), WHITE);
			drawChar(130, 118, (char) (((snake[snake_size].row % 1000) / 10) + '0'), WHITE);
			drawChar(140, 120, (char) (snake[snake_size].col + '0'), WHITE);
			*/

			waitForVblank();

			int tempRow = snake.tail_row;
			int tempCol = snake.tail_col;
			//int tempDirection = board[tempRow][tempCol].direction;
			for (int i = 0; i < snake_size; i++) { // this SHOULD never hit the head of the snake

				if (tempRow == snake.head_row  && tempCol == snake.head_col) {
					// end game
					state = GAME_OVER;
					break;
				}

				// move the tempRow and tempCol to the next part of the snake
				if (board[tempRow][tempCol].direction == 1) {
					tempRow--;
				} else if (board[tempRow][tempCol].direction == 2) {
					tempCol--;
				} else if (board[tempRow][tempCol].direction == 3) {
					tempRow++;
				} else if (board[tempRow][tempCol].direction == 4) {
					tempCol++;
				}


			}

			collided = 0;
			// check for collision first with food, second with the snake parts
			for (int i = 0; i < NUM_FOOD; i++) {
				/*if ((snake.head_row < food[i].row + BLOCK_SIZE) && (snake.head_row + BLOCK_SIZE > food[i].row)
					&& (snake.head_col < food[i].col + BLOCK_SIZE) &&
					(BLOCK_SIZE + snake.head_col > food[i].col)) {*/
				if (snake.head_row == food[i].row && snake.head_col == food[i].col) {

						// if snake head is ontop of food, undraw the food
						drawRect(food[i].row * 5, food[i].col * 5, BLOCK_SIZE, BLOCK_SIZE, GREEN);
						// TOOD: redraw the head??? na fam

						food[i].row = FOOD_GONE;
						food[i].col = FOOD_GONE;

						snake_size++;
						collided = 1;
						//drawRect(snake[snake_size].row, snake[snake_size].col, BLOCK_SIZE, BLOCK_SIZE, GREEN);
				}
			}

			// draw the food, if it hasn't been drawn
			for (int i = 0; i < NUM_FOOD; i++) {
				if (food[i].row == FOOD_GONE && food[i].col == FOOD_GONE) {
					food[i].row = rand() % 31;
					food[i].col = rand() % 47;
					//drawRect(food[i].row * 5, food[i].col * 5, BLOCK_SIZE, BLOCK_SIZE, YELLOW);
					drawImage3(food[i].row * 5, food[i].col * 5, 5, 5, apple);
				}
			}


			// put in the direction so that the tail knows where it's going
			board[snake.head_row][snake.head_col].direction = direction;

			if (collided == 0) {
				drawRect(snake.tail_row * 5, snake.tail_col * 5, BLOCK_SIZE, BLOCK_SIZE, BLACK);
				board[snake.tail_row][snake.tail_col].status = EMPTY;
				if (board[snake.tail_row][snake.tail_col].direction == 1) {
					board[snake.tail_row][snake.tail_col].direction = 0;
					snake.tail_row--;
				} else if (board[snake.tail_row][snake.tail_col].direction == 2) {
					board[snake.tail_row][snake.tail_col].direction = 0;
					snake.tail_col--;
				} else if (board[snake.tail_row][snake.tail_col].direction == 3) {
					board[snake.tail_row][snake.tail_col].direction = 0;
					snake.tail_row++;
				} else if (board[snake.tail_row][snake.tail_col].direction == 4) {
					board[snake.tail_row][snake.tail_col].direction = 0;
					snake.tail_col++;
				}
			}

			// draw the new head of the snake
			if (direction == 1) {
				snake.head_row--;
			} else if (direction == 2) {
				snake.head_col--;
			} else if (direction == 3) {
				snake.head_row++;
			} else if (direction == 4) {
				snake.head_col++;
			}
			board[snake.head_row][snake.head_col].status = HAS_SNAKE;

			drawRect(snake.head_row * 5, snake.head_col * 5, BLOCK_SIZE, BLOCK_SIZE, GREEN);
			delay(500);
			// END OF MAIN GAME CODE
			break;
		case GAME_OVER:
			drawImage3(0, 0, 240, 160, gameover);

			int j = snake_size;
			for (int i = 2; i >= 0; i--) {
				string[i] = j % 10 + '0';
				j = j / 10;
				/*if (i < 2) {
					if (j == 0) {
						string[i] = '0';
					}
				}*/
			}
			drawString(70, 105, string, WHITE);
			//drawChar(70, 120, (char) (snake_size+'0'), WHITE);


			state = NODRAW;
			break;
		case NODRAW:
			if (KEY_DOWN_NOW(BUTTON_START) && keyPressed == 0) {
				state = MAIN_MENU;

				snake.head_row = 5;
				snake.head_col = 5;
				snake.tail_row = snake.head_row;
				snake.tail_col = snake.head_col;

				direction = 0;  // start going no where
				snake_size = 0; // actual size - 1. or amount of snake bits + head
				old_size = snake_size;
				collided = 0;

				for (int i = 0; i < NUM_FOOD; i++) {
					food[i].row = FOOD_GONE;
					food[i].col = FOOD_GONE;
				}

				// initilize an empty board
				for (int i = 0; i < 32; i++) {
					for (int j = 0; j < 48; j++) {
						board[i][j].direction = 0;
						board[i][j].status = 0;
					}
				}
				keyPressed = 1;
			}
			if (!KEY_DOWN_NOW(BUTTON_START)) {
				keyPressed = 0;
			}
			break;
		}
	}

}

void setPixel(int row, int col, unsigned short color)
{
	videoBuffer[OFFSET(row,col, 240)] = color;
}

void drawRect(int row, int col, int height, int width, volatile unsigned short color)
{
	for(int r=0; r<height; r++)
	{
		REG_DMA3SAD = (u32)&color;
		REG_DMA3DAD = (u32)(&videoBuffer[OFFSET(row+r, col, 240)]);
		REG_DMA3CNT = width | DMA_ON | DMA_SOURCE_FIXED;
	}
}



void waitForVblank()
{
	while(SCANLINECOUNTER > 160);
	while(SCANLINECOUNTER < 160);
}
