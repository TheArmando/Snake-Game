#include "mylib.h"
#include "text.h"

unsigned short *videoBuffer = (unsigned short *)0x6000000;


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
