/************************************************************************/
/************************************************************************/
/*																		*/
/*						RCA Studio II Emulator in 'C'					*/
/*																		*/
/************************************************************************/
/************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WINDOWS
#include <windows.h>
#endif

#include "1802.h"						/* Header file for simulator */
#include "external.h"

int	IntToDisplay = 29;					/* IRQ time -> Display time */
int ScanLines = 128;					/* No of display scan lines */
int CyclesFlyBack = 6;					/* Cycles in the flyback */
int EF1PulseTime = 14;					/* Pulse length of EF1 */
int Granularity = 64;					/* Blocks of code in done in one */
int CycPerFrame;						/* Cycles per frame (no DMA) */
int ExecTime;							/* Remaining cycles for program */
int VideoOn = 1;						/* Video on ? */
int CurrentKeySelected = 0;				/* Key Selected in OUT 2 */
int SoundCounter = 0;					/* No of cycles sound is on */
int HighPitch,LowPitch;					/* Pitch variation */
int PitchScale;							/* Pitch scaling */

static void ReadST2(char *FileName);

/************************************************************************/
/*							Set up timing								*/
/************************************************************************/

static void TimingSetup(void)
{
long Cycles;
ScanLines = ReadIni("ScanLines",128);	/* Read setup values */
IntToDisplay = ReadIni("IntToDisplay",29);
CyclesFlyBack = ReadIni("CyclesFlyBack",6);
EF1PulseTime = ReadIni("EF1Pulse",14);
Granularity = ReadIni("Granularity",64);
HighPitch = ReadIni("HighPitch",625);
LowPitch = ReadIni("LowPitch",312);
PitchScale = ReadIni("PitchScale",40);

Cycles = ReadIni("Clock",1780);

Cycles = Cycles * 1000L;				/* Pulses per second */
Cycles = Cycles / 8;					/* CPU Cycles per second */
Cycles = Cycles / 60;					/* CPU Cycles per frame */
CycPerFrame = (int)Cycles;
Cycles = Cycles - IntToDisplay;			/* Adjust for display stuff */
Cycles = Cycles - CyclesFlyBack * ScanLines;
Cycles = Cycles - EF1PulseTime;
Cycles = Cycles - 8 * ScanLines;		/* Cycles used in DMA Clocking */
ExecTime = (int)Cycles;
}

/************************************************************************/
/*  				  Load ROM Image into memory						*/
/************************************************************************/

static int LoadROMImage(char *File,int Offset)
{
struct CPU1802 p;
FILE *f = fopen(File,"rb");				/* Open image file */
if (f != NULL)							/* If opened okay */
	{
	CPU1802_Status(&p);					/* Read in the image file */
	fread(p.Memory+Offset,1,8192,f);
	fclose(f);
	}
return(f != NULL);						/* Return true if loaded okay */
}

/************************************************************************/
/*						Initialise RAM Memory							*/
/************************************************************************/

static void InitialiseMemory(void)
{
int i;
struct CPU1802 p;
CPU1802_Status(&p);
for (i = 0x800;i < 0x1000;i++)			/* Fill the 512 byte blocks with 0 */
 p.Memory[i] = (i & 0x200) ? 0xFF:0x00;	/* or 0xFF depending on type */
}

/************************************************************************/
/*							Update EF3 and EF4							*/
/************************************************************************/

static void UpdateEF34(int Key)
{
if (Key < 10)							/* Only 0-9 are valid */
	{
	CPU1802_SetFlag(2,EXTKeyPressed(Key,PLAYER1) ? 1 : 0);
	CPU1802_SetFlag(3,EXTKeyPressed(Key,PLAYER2) ? 1 : 0);
	}
else 									/* Other values set EF3,EF4 to 0 */
	{
	CPU1802_SetFlag(2,0);
	CPU1802_SetFlag(3,0);
	}
}

/************************************************************************/
/*	  Execute a given number of cycles, then update flags and so on		*/
/************************************************************************/

static void _Execute(int Cycles)
{
CPU1802_Exec(Cycles);        			/* Do those cycles */
UpdateEF34(CurrentKeySelected);			/* Update EFlags 3 and 4 */
}

/************************************************************************/
/*				Frame stuff, called at every interrupt					*/
/************************************************************************/

static void Synchronise(void)
{
struct CPU1802 p;
unsigned char *Display;
int Freq;

CPU1802_Status(&p);						/* Get information */
Display = p.Memory+0x900;
if (VideoOn == 0) Display = NULL;		/* Screen off */

if (p.Q)								/* Sound on ? */
	{
	SoundCounter++;						/* Track how long */
	Freq = HighPitch -					/* Work out variable pitch */
			SoundCounter * PitchScale;
	if (Freq < LowPitch)
			Freq = LowPitch;			/* Limit it at standard / 2 */
	}
else
	Freq = SoundCounter = 0;			/* No sound, new sound */

EXTSynchronise(Display,					/* Update screen and sound */
			   (int)(p.R[0x0B] & 0xFF),
			   (unsigned char *)NULL,
			   Freq);
}

/************************************************************************/
/*						Handle IN instructions							*/
/************************************************************************/

int _1802_InHandler(int Port)
{
if (Port & 1) VideoOn = 1;				/* Input on Port 1 video on */
return(0xFF);
}

/************************************************************************/
/*						Handle OUT instructions							*/
/************************************************************************/


void _1802_OutHandler(int Port,int Data)
{
if (Port & 0x02)						/* Uses N2 for decoding */
	{
	CurrentKeySelected = Data & 15;		/* Update the 'latch' */
	UpdateEF34(CurrentKeySelected);		/* Update EFLAGS 3 and 4 */
	}
}

/************************************************************************/
/*							  Hard Reset								*/
/************************************************************************/

void ResetCPU(char *Cartridge)
{
struct CPU1802 p;
int n;
TimingSetup();							/* Calculate timing values */
InitialiseMemory();						/* Initialise memory */
LoadROMImage("/Users/hachidorii/Downloads/studio2.rom",0);			/* Load the BIOS ROM */
ReadST2(Cartridge);						/* Load the cartridge */
CPU1802_Reset();						/* Reset the processor */
VideoOn = 0;       						/* Video off */
CPU1802_Status(&p);
n = ReadIni("BootBeep",24);				/* Extend the boot beep */
if (n != 0) p.Memory[0x301] = n;
}

/************************************************************************/
/*							Do a whole frame							*/
/************************************************************************/

void CompleteFrame(char *Cartridge)
{
int i;
Synchronise();							/* Synchronise Display */

if (VideoOn)							/* Standard frame */
	{
	CPU1802_Interrupt();				/* Raise an interrupt */
	CPU1802_SetFlag(0,0);				/* EF1 = 0 */
	_Execute(IntToDisplay);				/* 29 cycles to display start */
	_Execute(CyclesFlyBack*ScanLines);
	CPU1802_SetFlag(0,1);				/* EF1 = 1 */
	_Execute(EF1PulseTime);				/* 14 Cycles with EF1 = 1 */
	CPU1802_SetFlag(0,0);				/* EF1 = 0 */

	for (i = 0;							/* Do in blocks, toggling EF1 */
		 i < ExecTime / Granularity;i++)
			{
			_Execute(Granularity);		/* Do a block */
			CPU1802_SetFlag(0,i & 1);	/* Toggle the EFlag */
			}
	}
else									/* Video on, straight cycles */
	{
	_Execute(CycPerFrame);
	}
if (EXTKeyPressed(KEY_CLEAR,PLAYER1)) ResetCPU(Cartridge);
}

/************************************************************************/
/*						Read the STEM.INI file							*/
/************************************************************************/

int ReadIni(char *Name,int Default)
{
FILE *f;
char Buffer[128],NameX[32];
f = fopen("stem.ini","r");     			/* Open file */
if (f == NULL) return(Default);			/* No file, use default */
strcpy(NameX,Name);strcat(NameX,"=");	/* Make '<NAME>=' string */
while (fgets(Buffer,sizeof(Buffer),f) != NULL)
	{									/* Scan file, if matches update */
	if (strncmp(NameX,Buffer,strlen(NameX)) == 0)
					Default = atoi(Buffer+strlen(NameX));
	}
fclose(f);								/* Close Stem.Ini */
return(Default);
}

/************************************************************************/
/*							Load a .ST2 format file						*/
/************************************************************************/

static void ReadST2(char *FileName)
{
char Header[128];
int i,Addr;
struct CPU1802 p;
unsigned char *Code;
FILE *f = fopen(FileName,"rb");			/* Open the file */
if (f == NULL) return;					/* Exit if file not found */
fread(Header,1,128,f);					/* Read in the header */

for (i = 1;i <= Header[4];i++)			/* Read in the code */
	{
	fseek(f,(long)i*256,SEEK_SET);		/* Move to the start of the page */
	Addr = ((int)(Header[i+63])) * 256;	/* Get an address */
	CPU1802_Status(&p);           		/* Find where memory is */
	Code = p.Memory;
	fread(Code+Addr,1,256,f);			/* Read in the code */
	}
fclose(f);
}
