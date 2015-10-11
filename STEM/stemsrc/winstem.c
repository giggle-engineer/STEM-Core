#include <windows.h>
#include <stdio.h>
#include <shellapi.h>
#include <commdlg.h>
#include <dir.h>
#include <string.h>
#include <ctype.h>
#include "1802.h"
#include "external.h"

#define SCALE (4)
#define WMU_SETTITLE  (WM_USER+0)

LRESULT CALLBACK MainWndProc(HWND hWnd,unsigned iMessage,WORD wParam,LONG lParam);

HWND hWndMain;
HANDLE hInstance;
char szCartridge[128];

int PASCAL WinMain(HANDLE hInst,HANDLE hPrev,LPSTR lpszCmdLine,int nCmdShow)
{
WNDCLASS wc;
MSG Msg;
BOOL QuitMsg;
int n;
hInstance = hInst;
if (hPrev == NULL)
	{
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)MainWndProc;
	wc.cbWndExtra = 0;
	wc.cbClsExtra = 0;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon(hInst,MAKEINTRESOURCE(900));
	wc.hCursor = LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground = GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = MAKEINTRESOURCE(999);
	wc.lpszClassName = "Studio2Class";
	RegisterClass(&wc);
	}

lstrcpy(szCartridge,lpszCmdLine);

hWndMain = CreateWindow("Studio2Class",
						"WinSTEM",
						WS_OVERLAPPEDWINDOW | WS_BORDER,
						CW_USEDEFAULT,CW_USEDEFAULT,
						128*SCALE,96*SCALE,
						NULL,
						NULL,
						hInst,NULL);

DragAcceptFiles(hWndMain,TRUE);

ShowWindow(hWndMain,nCmdShow);
EXTInitialise(hInst,hWndMain);
ResetCPU(szCartridge);
SendMessage(hWndMain,WMU_SETTITLE,0,0L);

QuitMsg = FALSE;
while (!QuitMsg)
	{
	if (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
		{
		if (Msg.message == WM_QUIT) QuitMsg = TRUE;
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
		}
	else
		{
		CompleteFrame(szCartridge);
		CompleteFrame(szCartridge);
		CompleteFrame(szCartridge);
		}
	}
EXTTerminate();
return(0);
}

void SetupDialog(HWND hWnd,OPENFILENAME *p)
{
p->lStructSize = sizeof(OPENFILENAME);
p->hwndOwner = hWnd;
p->hInstance = GetWindowWord(hWnd,GWW_HINSTANCE);
p->lpstrFilter = "RCA Studio 2\x0*.st2\x0\x0";
p->lpstrCustomFilter = NULL;
p->nFilterIndex = 1;
p->lpstrFile = szCartridge;
p->nMaxFile = sizeof(szCartridge);
p->lpstrFileTitle = NULL;
p->lpstrInitialDir = NULL;
p->lpstrTitle = "Load a Studio II Game";
p->Flags = OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
p->lpstrDefExt = "st2";
}

LRESULT CALLBACK MainWndProc(HWND hWnd,unsigned iMessage,WORD wParam,LONG lParam)
{
LONG lRet = 0L;
HMENU hMenu;
int   Scale;
RECT  rc,rcc;
OPENFILENAME ofn;
char szBuffer[128],szName[MAXFILE+10];

switch(iMessage)
	{
	case WM_COMMAND:
		switch(wParam)
			{
			case 101:	ResetCPU(szCartridge);
						break;
			case 103:	SendMessage(hWnd,WM_CLOSE,0,0L);
						break;
			case 102:	MessageBox(hWnd,"WinSTEM Studio II Emulator\n\nWritten by Paul Robson 2000","About...",MB_OK | MB_ICONINFORMATION);
						break;
			case 104:
			case 106:	hMenu = GetMenu(hWnd);
						CheckMenuItem(hMenu,104,MF_BYCOMMAND | ((wParam == 104) ? MF_CHECKED : MF_UNCHECKED));
						CheckMenuItem(hMenu,106,MF_BYCOMMAND | ((wParam == 106) ? MF_CHECKED : MF_UNCHECKED));
						_EXTSetColour(wParam == 104);
						break;
			case 108:
			case 109:	Scale = (wParam == 108) ? 4 : 1 ;
						GetWindowRect(hWnd,&rc);
						GetClientRect(hWnd,&rcc);

						SetWindowPos(hWnd,
									 NULL,
									 rc.left,
									 rc.top,
									 (rc.right-rc.left)-rcc.right+rcc.right*Scale/2,
									 (rc.bottom-rc.top)-rcc.bottom+rcc.bottom*Scale/2,
									 SWP_NOZORDER);
						break;
			case 105:
						SetupDialog(hWnd,&ofn);
						if (GetOpenFileName(&ofn) != 0)
							{
							ResetCPU(szCartridge);
							SendMessage(hWndMain,WMU_SETTITLE,0,0L);
							}
						break;
			}
		break;

	case WM_DROPFILES:
		DragQueryFile( (HANDLE)wParam,0,szCartridge,sizeof(szCartridge));
		ResetCPU(szCartridge);
		SendMessage(hWndMain,WMU_SETTITLE,0,0L);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WMU_SETTITLE:
		if (lstrlen(szCartridge) == 0)
			SetWindowText(hWnd,"WinStem");
		else
			{
			fnsplit(szCartridge,szBuffer,szBuffer,szName,szBuffer);
			strlwr(szName);
			szName[0] = toupper(szName[0]);
			lstrcat(szName," - WinStem");
			SetWindowText(hWnd,szName);
			}
		break;

	default:
		lRet = DefWindowProc(hWnd,iMessage,wParam,lParam);
		break;
	}
return(lRet);
}