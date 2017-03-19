#include <stdlib.h>
#include "text.h"
#include "mainmenu.h"
#include "apple.h"
#include "gameover.h"
#include "mylib.h"

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
