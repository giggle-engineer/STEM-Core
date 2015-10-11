struct CPU1802
{
int D,DF,Q,IE,X,P,T,EF[4];
long R[16];
int MemorySize;
unsigned char *Memory;
};

void CPU1802_Reset(void);
void CPU1802_Status(struct CPU1802 *);
void CPU1802_SetFlag(int,int);
void CPU1802_Exec(int);
void CPU1802_Dump(FILE *);
void CPU1802_Interrupt(void);

void _1802_OutHandler(int,int);
int  _1802_InHandler(int);

void ResetCPU(char *Cartridge);
void CompleteFrame(char *Cartridge);
int  ReadIni(char *Name,int Default);