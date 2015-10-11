/*

		Converts the file 1802.DEF into appropriate code

*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

static int RipHex(char **pp)
{
int c,n = 0;
while (isxdigit(**pp))
	{
	c = *(*pp)++;c = toupper(c);
	c = c - 48;if (c > 9) c = c - 7;
	n = n * 16 + c;
	}
return(n);
}

static void GenerateCodeLine(int ID,char *Desc)
{
printf("case 0x%02x: ",ID);
while (*Desc != '\0')
	{
	if (*Desc == '*')
		{
		Desc++;
		printf("%d",RipHex(&Desc) & ID);
		}
	else
		printf("%c",*Desc++);
	}
printf(";break;\n");
}
static void Generate(char *Line)
{
int Start,End,i;
Start = End = RipHex(&Line);			/* Get start opcode, default end */
if (*Line == '-')						/* if range a-b */
	{
	Line++;End = RipHex(&Line);			/* Get the end */
	}
while (*Line == ' ') Line++;			/* Remove leading spaces */
while (*Line != '\0' &&					/* Remove trailing spaces */
		Line[strlen(Line)-1] == ' ') Line[strlen(Line)-1] = '\0';
for (i = Start;i <= End;i++)			/* Generate all the CASE statements */
			GenerateCodeLine(i,Line);
}

int main()
{
FILE *f;
char Buffer[128],*p;
f = fopen("1802.def","r");				/* Open the definition file */
while (fgets(Buffer,					/* Work through the file */
		sizeof(Buffer),f) != NULL)
	{
	p = strstr(Buffer,"//");			/* Remove comments */
	if (p != NULL) *p = '\0';
	p = Buffer;							/* Remove control characters */
	while (*p != '\0') if (*p < ' ') *p++ = ' '; else p++;
	while (Buffer[0] == ' ')			/* Strip spaces */
				strcpy(Buffer,Buffer+1);
	if (strlen(Buffer) != 0)			/* Generate if relevant data */
					Generate(Buffer);
	}
fclose(f);
return 0;
}