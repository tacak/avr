#include <tchar.h>
#include <Windows.h>
#include <stdio.h>
#include "resource.h"

#define MYMSG_TRAY WM_APP + 1
#define BUFFER_LENGTH 4096

_TCHAR logFileName[MAX_PATH + 1];
HINSTANCE hInst;
BOOL bWatch = TRUE;

ATOM InitApp(HINSTANCE hInst);
BOOL InitInstance(HINSTANCE hInst, int nCmdShow);

int MakeRTrayMenu(HWND hWnd)
{
    HMENU hMenu, hSubMenu;
    POINT pt;

	if(bWatch){
		hMenu = LoadMenu(hInst, MAKEINTRESOURCE(RTRAYMENU1));
	}
	else{
		hMenu = LoadMenu(hInst, MAKEINTRESOURCE(RTRAYMENU2));
	}

    hSubMenu = GetSubMenu(hMenu, 0);
    GetCursorPos(&pt);
    SetForegroundWindow(hWnd);
    TrackPopupMenu(
        hSubMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
    return 0;
}

void MyTaskTray(HWND hWnd, PNOTIFYICONDATA lpni)
{
    HICON hIcon;

    hIcon = LoadIcon(hInst, MAKEINTRESOURCE(ON_ICON));
    lpni->cbSize = sizeof(NOTIFYICONDATA);
    lpni->hIcon = hIcon;
    lpni->hWnd = hWnd;
    lpni->uCallbackMessage = MYMSG_TRAY;
    lpni->uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    lpni->uID = 1;
	_tcscpy_s(lpni->szTip, 128, _T("AutoLocker"));
    
    BOOL ret = Shell_NotifyIcon(NIM_ADD, lpni);
    return;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static HANDLE h;
    static NOTIFYICONDATA ni;
	static DWORD AwayCnt = 0;

    switch (msg) {
    case WM_CREATE:
		MyTaskTray(hWnd, &ni);
		SetTimer(hWnd, 1, 1000, NULL);

		TCHAR ComPort[16];
		FILE *fin;
		_tfopen_s(&fin, _T("ComPort"), _T("r"));
		if(fin == NULL){
			_tcscpy_s(ComPort, 15, _T("COM4"));
		}
		else{
			_fgetts(ComPort, 15, fin);
			fclose(fin);
		}

		h = CreateFile(ComPort, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
		if(h == INVALID_HANDLE_VALUE){
			MessageBox(NULL, _T("COM ポートがオープンできませんでした"), _T("Error"), MB_OK);
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			return 0;
		}
		DCB dcb;
		GetCommState(h, &dcb);
		dcb.ByteSize = 8;
		dcb.BaudRate = 9600;
		dcb.fParity = 0;
		dcb.fOutxCtsFlow = 0;
		if(!SetCommState(h, &dcb)){
			return 0;
		}

		COMMTIMEOUTS cto;
		cto.ReadIntervalTimeout = 10;
		cto.ReadTotalTimeoutMultiplier = 0;
		cto.ReadTotalTimeoutConstant = 10;
		cto.WriteTotalTimeoutMultiplier = 0;
		cto.WriteTotalTimeoutConstant = 0;
		SetCommTimeouts(h, &cto);
		break;
	case WM_TIMER:
		DWORD nn;
		byte buf[16];
		ReadFile(h, buf, 16, &nn, 0);
		if(bWatch == TRUE && nn != 0){
			LockWorkStation();
		}
		break;
	case MYMSG_TRAY:
		switch (lp){
		case WM_RBUTTONDOWN:
		case WM_LBUTTONDOWN:
			MakeRTrayMenu(hWnd);
			break;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wp)){
		case IDM_ON:
			ni.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(ON_ICON));
			Shell_NotifyIcon(NIM_MODIFY, &ni);
			bWatch = TRUE;
			break;
		case IDM_OFF:
			ni.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(OFF_ICON));
			Shell_NotifyIcon(NIM_MODIFY, &ni);
			bWatch = FALSE;
			break;
        case IDM_END:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		}
		break;
	case WM_CLOSE:
		Shell_NotifyIcon(NIM_DELETE, &ni);
		KillTimer(hWnd, 1);
		CloseHandle(h);
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
        PostQuitMessage(0);
        break;
	default:
        return DefWindowProc(hWnd, msg, wp, lp);
	}

	return 0;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
    
    hInst = hInstance;
    if (!InitApp(hInstance))
        return FALSE;
    if (!InitInstance(hInstance, nCmdShow)) 
        return FALSE;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}

ATOM InitApp(HINSTANCE hInst)
{
    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;    //プロシージャ名
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;//インスタンス
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;    //メニュー名
    wc.lpszClassName = _T("AutoLocker_Class");
    wc.hIconSm = NULL;//アイコン
    return (RegisterClassEx(&wc));
}

//ウィンドウの生成
BOOL InitInstance(HINSTANCE hInst, int nCmdShow)
{
    HWND hWnd;

    hWnd = CreateWindow(_T("AutoLocker_Class"),
            _T("AutoLocker"), //タイトルバーにこの名前が表示されます
            WS_OVERLAPPEDWINDOW, //ウィンドウの種類
            CW_USEDEFAULT,    //Ｘ座標
            CW_USEDEFAULT,    //Ｙ座標
            300,    //幅
            250,    //高さ
            NULL, //親ウィンドウのハンドル、親を作るときはNULL
            NULL, //メニューハンドル、クラスメニューを使うときはNULL
            hInst, //インスタンスハンドル
            NULL);
    if(!hWnd)
        return FALSE;

    ShowWindow(hWnd, SW_HIDE);
	return TRUE;
}