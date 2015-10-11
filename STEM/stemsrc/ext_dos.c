/************************************************************************/
/************************************************************************/
/*																		*/
/*						External Drivers : MSDOS						*/
/*																		*/
/*			This is both the VGA and Text versions of the emulator		*/
/*																		*/
/************************************************************************/
/************************************************************************/
/*
	Note : on some Turbo 'C's the getvect() and setvect() prototypes in
	"DOS.H" are commented out, and will need uncommenting. Why this is I
	have no idea : PR
*/

#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <mem.h>
#include <dos.h>
#include <time.h>
#include "external.h"					/* General includes */
#include "1802.h"

typedef void interrupt (*IntVec)(void);	/* Interrupt routine type */

unsigned char far *Screen;				/* Pointer to display */
unsigned char Scan;						/* Scan code (interrupt) */
unsigned char KeyTable[128];			/* Key press table (interrupt) */
IntVec	 OldIntVec;						/* Old keyboard interrupt */
unsigned char Pixels[64][32];			/* Last pixel value */
unsigned long Colour,FGR,BGR;			/* Colour stuff */
int		 Attrib;						/* Text Attribute */

#define EXTEND(v) (v | (v << 8) | (v << 16) | (v << 24))

/************************************************************************/
/*					Return Non-Zero if key is pressed					*/
/************************************************************************/

int KeyP1[10] = { 45,2,3,4,16,17,18,30,31,32 };
int KeyP2[10] = { 82,71,72,73,75,76,77,79,80,81 };

int EXTKeyPressed(int Key,int Player)
{
int Press = 0;
if (kbhit() != 0) getch();				/* Empty keyboard buffer */
switch(Key)
	{
	case KEY_EXIT:						/* EXIT (Escape) */
		Press = KeyTable[1];
		break;
	case KEY_CLEAR:						/* Clear (F1) */
		Press = KeyTable[59];
		break;
	default:							/* Handle Player Keys 1 & 2 */
		Press = KeyP1[Key];
		if (Player == PLAYER2) Press = KeyP2[Key];
		Press = KeyTable[Press];
		break;
	}
return((Press == 0) ? 0 : 1);
}

/************************************************************************/
/*					Update display (graphics mode)						*/
/************************************************************************/

static void ScreenGraphics(unsigned char *ScreenData,int Scroll)
{
int x,y,Byte,Pixel;
unsigned long *pScreen;


for (x = 0;x < 64;x++)          		/* Horizontal coordinates */
	for (y = 0;y < 32;y++)				/* Vertical coordinates */
	{									/* Get data address, then pixel */
	Byte = ((x / 8) + (y * 8) + Scroll) & 0xFF;
	Pixel = (ScreenData[Byte] & (128 >> (x & 7))) ? 1 : 0;
	if (Pixels[x][y] != Pixel)			/* If changed */
		{
		Pixels[x][y] = Pixel;			/* Update last pixel */
		pScreen = (unsigned long *)(Screen + x * 4 + 32 + y * 6 * 320 + 4 * 320);
		Colour = Pixel ? FGR : BGR;		/* Decide Colour */
		*pScreen = Colour;				/* Draw the pixels */
		*(pScreen+80) = Colour;
		*(pScreen+160) = Colour;
		*(pScreen+240) = Colour;
		*(pScreen+320) = Colour;
		*(pScreen+400) = Colour;
		}
	}
}

/************************************************************************/
/*						Update display (text mode)						*/
/************************************************************************/

unsigned int Graphic[] = { 32,223,220,219 };

static void ScreenText(char *ScreenData,int Scroll)
{
int c,Base,n,y,x,x1,Bit;

for (y = 0;y < 32;y = y + 2)			/* Go through two at a time */
	{
	for (x = 0;x < 8;x++)				/* Work the row */
		{
		n = y * 8 + x;					/* Byte offset in memory */
		Base = x * 16 +					/* Screen address */
					(y / 2) * 160 + 16 + 160*4;

		for (x1 = 0;x1 < 8;x1++)		/* Work the byte */
				{
				Bit = (128 >> x1);		/* Construct top and bottom */
				c = (ScreenData[(n+Scroll) & 0xFF] & Bit) ? 1 : 0;
				c = c + ((ScreenData[(n+Scroll+8) & 0xFF] & Bit) ? 2 : 0);
				Screen[Base++] = Graphic[c];
				Screen[Base++] = Attrib;
				}
		}
	}
}

/************************************************************************/
/*	Synchronise to clock, update screen and sound. Called every frame	*/
/************************************************************************/

int FrameCounter = 0;
clock_t LastTick;
int SoundFlags[3];

void EXTSynchronise(unsigned char *ScreenData,
						int Scroll,unsigned char *HiRes,int SoundOn)
{
clock_t t;
struct CPU1802 p;
int Freq;

if (ScreenData != NULL)					/* Is the screen on ? */
	{
	#ifdef GFX_VGA						/* If so use appropriate update */
	ScreenGraphics(ScreenData,Scroll);	/* routine */
	#else
	ScreenText(ScreenData,Scroll);
	#endif
	}

SoundFlags[FrameCounter++] = SoundOn;	/* Track sound for three frames */

if (FrameCounter == 3)					/* Every 3 frames. We can't sync */
	{									/* to 60Hz, so sync to 18.2Hz */
	FrameCounter = 0;
	do  								/* Wait for clock tick change */
		t = clock();
	while (t == LastTick);
	LastTick = t;

	if (SoundFlags[0] ||				/* Sound if any sound this time */
			SoundFlags[1] || SoundFlags[2])
		{
		Freq = SoundFlags[0];			/* Select frequency */
		if (SoundFlags[1]) Freq = SoundFlags[1];
		if (SoundFlags[2]) Freq = SoundFlags[2];
		sound(Freq);					/* And beep ! */
		}
	else        						/* No sound, sound off */
		{
		nosound();
		}
	}
}

/************************************************************************/
/*							Replacement INT9							*/
/************************************************************************/

void interrupt KeyboardInterrupt(void)
{
Scan = inportb(0x60);					/* Read info from keyboard */
if (Scan > 0x7F)						/* Bit 7 high, clear key */
	KeyTable[Scan & 0x7F] = 0;
else									/* Bit 7 low, set key */
	KeyTable[Scan] = 1;
outportb(0x20,0x20);					/* Reset keyboard driver */
(*OldIntVec)();							/* Call old routine */
}

/************************************************************************/
/*					  Initialise External Stuff							*/
/************************************************************************/

void EXTInitialise(void)
{
unsigned int i,j,Bgr;
union REGS r;
Bgr = ReadIni("DOSSurround",4);
#ifdef GFX_VGA							/* Graphic VGA Mode */
r.x.ax = 0x13;int86(0x10,&r,&r);        /* Switch to Mode 13h */
Screen = (unsigned char far *)(0xA000L << 16);
for (i = 0;i < 320*200;i++) Screen[i] = Bgr;
#else									/* 80 x 25 Text Mode */
Screen = (unsigned char far *)(0xB800L << 16);
for (i = 0;i < 160*25;i=i+2)			/* Clear the Screen */
	{ Screen[i] = 32;Screen[i+1] = 0x07+(Bgr << 4); }
#endif
for (i = 0;i < 128;i++) KeyTable[i] = 0;/* Zero the key table */
for (i = 0;i < 64;i++)      			/* Last value table = junk */
	for (j = 0;j < 32;j++)
		Pixels[i][j] = 0xAA;
OldIntVec = getvect(9);					/* Save the old interrupt routine */
setvect(9,KeyboardInterrupt);			/* Use the new interrupt routine */
FGR = ReadIni("DOSForeground",15);		/* Create colours */
BGR = ReadIni("DOSBackground",0);
FGR = EXTEND(FGR); BGR = EXTEND(BGR);
Attrib = ReadIni("DOSForeground",15)+	/* Display Attribute */
				(ReadIni("DOSBackground",0) << 4);
}

/************************************************************************/
/*					    Terminate External Stuff						*/
/************************************************************************/

void EXTTerminate(void)
{
int i;
union REGS r;
#ifdef GFX_VGA							/* Return to text screen */
r.x.ax = 0x03;int86(0x10,&r,&r);        /* Switch to Mode 03h */
#endif
textattr(0x7);
clrscr();								/* Delete screen */
setvect(9,OldIntVec);					/* Replace old interrupt routine */
nosound();
}
