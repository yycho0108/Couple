#include <Windows.h>
#include "resource.h"
#include <vector>
#include <algorithm>
#include <random>
#include <functional>
#include <ctime>
#include <iterator>
#define CardSize 64
#define FLIPBACK 404
#define NUMSUIT 17

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK CardProc(HWND, UINT, WPARAM, LPARAM);
VOID CALLBACK FlipProc(HWND, UINT, UINT, DWORD);
inline void PrintCard(HDC& hdc, int num);


UINT WM_FLIP = RegisterWindowMessage(L"Flip");

// information it needs :

// 1 ) suit (NUMBER that it holds)
// 2 ) whether or not it is open.

BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
HWND OneOpen;
void InitializeBoard();

HINSTANCE g_hInst;
HBITMAP Suite[NUMSUIT];
LPTSTR MainTitle = L"Couple";
LPTSTR CardTitle = L"Card";

HWND hMainWnd;
int Width, Height;
int NumPair;
std::vector<std::vector<int>> Board;
std::vector<std::vector<HWND>> Cards;

ATOM myRegister(LPTSTR& ClassName, WNDPROC Proc)
{
	WNDCLASS WndClass;
	WndClass.cbClsExtra = Proc==CardProc; //1 if Proc == CardProc
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = g_hInst;
	WndClass.lpfnWndProc = Proc;
	WndClass.lpszClassName = ClassName;
	WndClass.lpszMenuName = Proc == CardProc ? NULL : MAKEINTRESOURCE(IDR_MENU1);
	WndClass.style = CS_VREDRAW | CS_HREDRAW;
	return RegisterClass(&WndClass);
}
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	for (int i = 0; i < NUMSUIT; i++)
	{
		Suite[i] = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP1 + i));
	}

	g_hInst = hInstance;

	myRegister(MainTitle, WndProc);
	myRegister(CardTitle, CardProc);

	hMainWnd = CreateWindow(MainTitle, MainTitle, WS_OVERLAPPEDWINDOW^WS_THICKFRAME, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	ShowWindow(hMainWnd, nCmdShow);

	MSG msg;
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	switch (iMsg)
	{
	case WM_CREATE:
		hMainWnd = hWnd;
		DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), hMainWnd, DlgProc);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_FILE_NEWGAME:
			DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), hMainWnd, DlgProc);
			break;
		case ID_FILE_EXIT:
			PostQuitMessage(0);
			break;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, iMsg, wParam, lParam);
	}
	return 0;
}

BOOL CALLBACK DlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemInt(hDlg, IDC_EDIT1, Width?Width:4, FALSE);
		SetDlgItemInt(hDlg, IDC_EDIT2, Height?Height:4, FALSE);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			BOOL checkw, checkh;
			Width = GetDlgItemInt(hDlg, IDC_EDIT1, &checkw, FALSE);
			Height = GetDlgItemInt(hDlg, IDC_EDIT2, &checkh, FALSE);
			NumPair = Width*Height / 2;
			if (checkw&&checkh&&!((Width*Height) % 2))
			{
				InitializeBoard();
				EndDialog(hDlg,IDOK);
			}
			else
			{
				PostQuitMessage(0);
				EndDialog(hDlg, IDCANCEL);
			}
			break;
		case IDCANCEL:
			PostQuitMessage(0);
			EndDialog(hDlg, IDCANCEL);
			break;
		}
	default:
		return FALSE;
	}
	return TRUE;
}

void InitializeBoard()
{
	int i = -1;
	std::vector<int> sequence;
	sequence.reserve(Width*Height);
	std::generate_n(std::back_inserter(sequence), Width*Height, [&i](){return ++i / 2; });//0,0,1,1,2,2,3,3...
	std::random_shuffle(sequence.begin(), sequence.end(), [](int a){return std::bind(std::uniform_int_distribution <int > {0, a-1}, std::default_random_engine((unsigned int)time(0)))(); });

	//cleaning up
	for (auto &x : Cards)
	{
		for (auto &y : x)
		{
			DestroyWindow(y);
		}
	}

	Cards.clear();

	for (int i = 0; i < Height; i++)
	{
		Cards.push_back(std::vector<HWND>());
		Cards[i].reserve(Width);
		for (int j = 0; j < Width; j++)
		{
			Cards[i].push_back(CreateWindow(CardTitle, CardTitle, WS_CHILD | WS_VISIBLE | WS_BORDER, j*CardSize, i*CardSize, CardSize, CardSize, hMainWnd, NULL, g_hInst, NULL));
			ShowWindow(Cards[i][j], SW_SHOW);
			SetWindowLong(Cards[i][j], GWL_USERDATA, sequence.back()); //push in suits
			sequence.pop_back();
		}
	}

	RECT R = { 0, 0, CardSize*Width, CardSize*Height };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW^WS_THICKFRAME, TRUE);
	SetWindowPos(hMainWnd, NULL, 0, 0, R.right - R.left, R.bottom - R.top, SWP_NOMOVE);

	for (auto& x : Cards)
	{
		for (auto&y : x)
		{
			EnableWindow(y, FALSE);
		}
	}

	SetTimer(hMainWnd, FLIPBACK, 2000, FlipProc);
}

LRESULT CALLBACK CardProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	TCHAR strbuf[20];
	PAINTSTRUCT ps;

	switch (iMsg)
	{

	case WM_PAINT:
	{
		hdc = BeginPaint(hWnd, &ps);
		if (!IsWindowEnabled(hWnd))
		{
			PrintCard(hdc, GetWindowLong(hWnd, GWL_USERDATA) + 1);
			//PAINT... SUIT
			//wsprintf(strbuf, L"%d", );
			//TextOut(hdc, 0, 0, strbuf, lstrlen(strbuf));
		}// else PAINT BACK OF CARD.
		else
		{
			PrintCard(hdc, 0);
		}
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_TIMER:
		SendMessage(hWnd, WM_FLIP, 0, 0);
		KillTimer(hWnd, 0);
		break;
	case WM_LBUTTONDOWN:
	{
		SendMessage(hWnd, WM_FLIP, 0, 0);

		if (OneOpen)
		{
			if (GetWindowLong(OneOpen, GWL_USERDATA) == GetWindowLong(hWnd, GWL_USERDATA))
			{
				if (!(--NumPair))
				{
					if (MessageBox(hMainWnd, L"CONTINUE?", L"CLEAR!",MB_YESNO) == IDYES)
					{
						DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), hMainWnd, DlgProc);
					}
					else PostQuitMessage(0);
				}
			}
			else
			{
				SetTimer(hWnd, 0, 1000, NULL);
				SetTimer(OneOpen, 0, 1000, NULL);
			}
			OneOpen = NULL;
		}
		else OneOpen = hWnd;

		break;
	}
	case WM_DESTROY:
	{
		break;
	}
	default:
		if (iMsg == WM_FLIP)
		{
			EnableWindow(hWnd, !IsWindowEnabled(hWnd));
			InvalidateRect(hWnd, NULL, true);
			break;
		}
		else return DefWindowProc(hWnd, iMsg, wParam, lParam);
	}
	return 0;
}

VOID CALLBACK FlipProc(HWND hWnd, UINT iMsg, UINT CallerId, DWORD dwTime)
{
	for (auto&x : Cards)
	{
		for (auto&y : x)
		{
			SendMessage(y, WM_FLIP, 0, 0);
		}
	}
	KillTimer(hWnd, CallerId);
}

inline void PrintCard(HDC& hdc, int num)
{
	HDC MemDC = CreateCompatibleDC(hdc);
	HBITMAP OldBit = (HBITMAP)SelectObject(MemDC, Suite[num]);
	BitBlt(hdc, 0, 0, CardSize, CardSize, MemDC, 0, 0, SRCCOPY);
	SelectObject(MemDC, OldBit);
	DeleteDC(MemDC);
}