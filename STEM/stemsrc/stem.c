/************************************************************************/
/************************************************************************/
/*																		*/
/*						RCA Studio II Emulator in 'C'					*/
/*																		*/
/************************************************************************/
/************************************************************************/

#include <stdio.h>
#include "1802.H"						/* Header file for simulator */
#include "external.h"					/* Port dependent stuff */

void main(int argc,char *Argv[])
{
char *Cartridge = "";
if (argc == 2) Cartridge = Argv[1];
EXTInitialise();						/* Set up all */
ResetCPU(Cartridge);					/* Reset the CPU */
while (EXTKeyPressed(KEY_EXIT,0) == 0)	/* Keep going until we stop */
						CompleteFrame(Cartridge);
EXTTerminate();							/* Tidy up */
}