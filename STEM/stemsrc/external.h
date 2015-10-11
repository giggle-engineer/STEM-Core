#define KEY_CLEAR	(10)
#define KEY_EXIT	(11)
#define PLAYER1		(1)
#define PLAYER2		(2)

int EXTKeyPressed(int Key,int Player);
void EXTSynchronise(unsigned char *ScreenData,int Scroll,unsigned char *HiRes,int SoundOn);

#ifdef MSDOS
void EXTInitialise(void);
#endif
#ifdef WINDOWS
void EXTInitialise(HANDLE hInst,HWND hWnd);
#endif

void EXTTerminate(void);

void _EXTSetColour(int IsWhite); /* Windows only */

