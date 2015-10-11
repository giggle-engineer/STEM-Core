#define	READ(Addr)			(Memory[(Addr) & MEMMASK])
#define WRITE(Addr,Data) 	_1802_Write(Addr,Data)
#define FETCH				READ(R[P]++)

#define	DEC(Reg)			Reg = ((Reg)-1) & 0xFFFF;
#define	INC(Reg)			Reg = ((Reg)+1) & 0xFFFF;

#define SHORTBRANCH(Test)	{ Temp = FETCH; if (Test) R[P] = (R[P] & 0xFF00) | Temp; }
#define LONGBRANCH(Test) 	{ Cycles--;Temp = FETCH; Temp = (Temp << 8) | FETCH; if (Test) R[P] = Temp; }
#define LONGSKIP(Test) 		{ Cycles--;if (Test) R[P] = R[P]+2; }

#define	ADD(v1,v2,c) 		{ Temp = (v1)+(v2)+(c); D = Temp & 0xFF; DF = (Temp >> 8) & 1; }
#define SUB(v1,v2,c) 		ADD(v1,(v2)^0xFF,c)

#define	SHL					{ DF = (D >> 7) & 1;D = (D << 1) & 0xFE; }
#define SHR 				{ DF = (D & 1); D = (D >> 1) & 0x7F; }
#define	SHLC				{ Temp = DF; SHL; D = D | Temp; }
#define SHRC 				{ Temp = DF; SHR; D = D | (Temp << 7); }