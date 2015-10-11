/************************************************************************/
/************************************************************************/
/*																		*/
/*						External Drivers : Windows						*/
/*																		*/
/************************************************************************/
/************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "external.h"					/* General includes */
#include "1802.h"

unsigned char Bits[64*32];

struct __BMInfo
{
BITMAPINFOHEADER bmi;
RGBQUAD bmiColours[256];
} BMInfo;

HWND hMainWindow;
int  FrameCount = 0;

/************************************************************************/
/*					Return Non-Zero if key is pressed					*/
/************************************************************************/

int Player1[10] = { 'X',
					'1','2','3',
					'Q','W','E',
					'A','S','D'};

int Player2[10] = { VK_NUMPAD0,
					VK_NUMPAD7,VK_NUMPAD8,VK_NUMPAD9,
					VK_NUMPAD4,VK_NUMPAD5,VK_NUMPAD6,
					VK_NUMPAD1,VK_NUMPAD2,VK_NUMPAD3 };

int EXTKeyPressed(int Key,int Player)
{
int Press = -1;
switch(Key)
	{
	case KEY_CLEAR:
		Press = VK_F1;
		break;
	case KEY_EXIT:
		break;
	default:
		Press = Player1[Key];
		if (Player == PLAYER2) Press = Player2[Key];
		break;
	}
if (Press >= 0)
	{
	Press = GetAsyncKeyState(Press) & 0x8000;
	return(Press == 0 ? 0 : 1);
	}
return(0);
}

/************************************************************************/
/*	Synchronise to clock, update screen and sound. Called every frame	*/
/************************************************************************/

DWORD dwLastTime = -1;

void EXTSynchronise(unsigned char *ScreenData,
						int Scroll,unsigned char *HiRes,int SoundOn)
{
int n,x,y,Byte;
RECT rc;
HDC hDC;
DWORD dwTime;
FrameCount = (FrameCount+1) % 3;

if (FrameCount == 0)
	{
	hDC = GetDC(hMainWindow);
	GetClientRect(hMainWindow,&rc);

	for (x = 0;x < 8;x++)
		for (y = 0;y < 32;y++)
			{
			Byte = ScreenData[(x+(31-y)*8+Scroll) & 0xFF];
			for (n = 0;n < 8;n++)
				Bits[x*8+y*64+n] = (Byte & (0x80 >> n)) ? 1 : 0;
			}

	StretchDIBits(hDC,
				  0,0,rc.right,rc.bottom,
				  0,0,64,32,
				  Bits,
				  (BITMAPINFO *)&BMInfo,
				  DIB_RGB_COLORS,
				  SRCCOPY);

	ReleaseDC(hMainWindow,hDC);
	do
		dwTime = GetTickCount() / 40;
	while (dwTime == dwLastTime);
    dwLastTime = dwTime;
	}
}

/************************************************************************/
/*					  Initialise External Stuff							*/
/************************************************************************/

void EXTInitialise(HANDLE hInst,HWND hWnd)
{
int i,n;
HDC hWinDC;

hMainWindow = hWnd;
hWinDC = GetDC(hWnd);

BMInfo.bmi.biSize = sizeof(BITMAPINFOHEADER);
BMInfo.bmi.biWidth = 64;
BMInfo.bmi.biHeight = 32;
BMInfo.bmi.biPlanes = 1;
BMInfo.bmi.biBitCount = 8;
BMInfo.bmi.biCompression = BI_RGB;
BMInfo.bmi.biSizeImage = 0;
BMInfo.bmi.biXPelsPerMeter = 0;
BMInfo.bmi.biYPelsPerMeter = 0;
BMInfo.bmi.biClrUsed = 2;
BMInfo.bmi.biClrImportant = 2;


for (i = 0;i < 2;i++)
	{
	n = i * 255;
	BMInfo.bmiColours[i].rgbRed = n;
	BMInfo.bmiColours[i].rgbGreen = n;
	BMInfo.bmiColours[i].rgbBlue = n;
	BMInfo.bmiColours[i].rgbReserved = 0;
	}

for (i = 0;i < 64*32;i++) Bits[i] = random(2);
ReleaseDC(hMainWindow,hWinDC);
}

/************************************************************************/
/*					    Terminate External Stuff						*/
/************************************************************************/

void EXTTerminate(void)
{
}

/************************************************************************/
/*								Set colour								*/
/************************************************************************/

void _EXTSetColour(int IsWhite)
{
int n = 0;
if (IsWhite != 0) n = 255;
BMInfo.bmiColours[1].rgbRed = n;
BMInfo.bmiColours[1].rgbBlue = n;
}

